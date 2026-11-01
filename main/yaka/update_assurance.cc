#include "update_assurance.h"

#include <limits>

namespace hi_yaka {
namespace {

bool ParsePart(std::string_view text, std::size_t& cursor, uint16_t& value) {
    if (cursor >= text.size() || text[cursor] < '0' || text[cursor] > '9') {
        return false;
    }
    uint32_t parsed = 0;
    while (cursor < text.size() && text[cursor] >= '0' && text[cursor] <= '9') {
        parsed = parsed * 10U + static_cast<uint32_t>(text[cursor] - '0');
        if (parsed > std::numeric_limits<uint16_t>::max()) {
            return false;
        }
        ++cursor;
    }
    value = static_cast<uint16_t>(parsed);
    return true;
}

bool IsLowerHexSha256(std::string_view digest) {
    if (digest.size() != 64) {
        return false;
    }
    for (const char character : digest) {
        if (!((character >= '0' && character <= '9') ||
              (character >= 'a' && character <= 'f'))) {
            return false;
        }
    }
    return true;
}

}  // namespace

bool SemanticVersion::Parse(std::string_view text, SemanticVersion& result) {
    SemanticVersion parsed;
    std::size_t cursor = 0;
    if (!ParsePart(text, cursor, parsed.major) || cursor >= text.size() || text[cursor++] != '.' ||
        !ParsePart(text, cursor, parsed.minor) || cursor >= text.size() || text[cursor++] != '.' ||
        !ParsePart(text, cursor, parsed.patch) || cursor != text.size()) {
        return false;
    }
    result = parsed;
    return true;
}

int Compare(const SemanticVersion& left, const SemanticVersion& right) {
    if (left.major != right.major) return left.major < right.major ? -1 : 1;
    if (left.minor != right.minor) return left.minor < right.minor ? -1 : 1;
    if (left.patch != right.patch) return left.patch < right.patch ? -1 : 1;
    return 0;
}

UpdateRejection ValidateUpdateManifest(const UpdateManifest& manifest,
                                       const UpdatePolicy& policy,
                                       const PackageSignatureVerifier& verifier) {
    if (manifest.board != UpdatePolicy::kBoard) return UpdateRejection::kWrongBoard;
    SemanticVersion candidate;
    SemanticVersion installed;
    SemanticVersion minimum;
    if (!SemanticVersion::Parse(manifest.version, candidate) ||
        !SemanticVersion::Parse(policy.installed_version, installed) ||
        !SemanticVersion::Parse(manifest.minimum_installed_version, minimum)) {
        return UpdateRejection::kInvalidVersion;
    }
    if (Compare(candidate, installed) <= 0) return UpdateRejection::kVersionNotNewer;
    if (Compare(installed, minimum) < 0) return UpdateRejection::kInstalledVersionTooOld;
    if (policy.hardware_revision < manifest.minimum_hardware_revision ||
        policy.hardware_revision > manifest.maximum_hardware_revision) {
        return UpdateRejection::kHardwareIncompatible;
    }
    if (manifest.image_size == 0) return UpdateRejection::kImageEmpty;
    if (manifest.image_size > UpdatePolicy::kMaximumImageBytes) {
        return UpdateRejection::kImageTooLarge;
    }
    if (!IsLowerHexSha256(manifest.image_sha256)) return UpdateRejection::kInvalidDigest;
    if (manifest.signature.empty() || !verifier.Verify(manifest)) {
        return UpdateRejection::kInvalidSignature;
    }
    return UpdateRejection::kNone;
}

void UpdateAssuranceStateMachine::Fail(UpdateAssuranceFailure failure) {
    phase_ = UpdateAssurancePhase::kFailed;
    failure_ = failure;
}

bool UpdateAssuranceStateMachine::AcceptManifest(UpdateRejection rejection) {
    if (phase_ != UpdateAssurancePhase::kIdle) return false;
    if (rejection != UpdateRejection::kNone) {
        Fail(UpdateAssuranceFailure::kManifestRejected);
        return false;
    }
    phase_ = UpdateAssurancePhase::kAccepted;
    return true;
}

bool UpdateAssuranceStateMachine::BeginDownload() {
    if (phase_ != UpdateAssurancePhase::kAccepted) return false;
    phase_ = UpdateAssurancePhase::kDownloading;
    return true;
}

bool UpdateAssuranceStateMachine::CompleteDownload(bool digest_matches, bool signature_valid) {
    if (phase_ != UpdateAssurancePhase::kDownloading) return false;
    if (!digest_matches) {
        Fail(UpdateAssuranceFailure::kDigestMismatch);
        return false;
    }
    if (!signature_valid) {
        Fail(UpdateAssuranceFailure::kSignatureRejected);
        return false;
    }
    phase_ = UpdateAssurancePhase::kVerified;
    return true;
}

bool UpdateAssuranceStateMachine::PrepareReboot(bool migration_succeeded) {
    if (phase_ != UpdateAssurancePhase::kVerified) return false;
    if (!migration_succeeded) {
        Fail(UpdateAssuranceFailure::kConfigurationMigrationFailed);
        return false;
    }
    phase_ = UpdateAssurancePhase::kPendingReboot;
    return true;
}

bool UpdateAssuranceStateMachine::ObserveFirstBoot() {
    if (phase_ != UpdateAssurancePhase::kPendingReboot) return false;
    phase_ = UpdateAssurancePhase::kPendingConfirmation;
    return true;
}

bool UpdateAssuranceStateMachine::ConfirmAfterSelfTest(bool self_test_succeeded) {
    if (phase_ != UpdateAssurancePhase::kPendingConfirmation) return false;
    if (!self_test_succeeded) {
        phase_ = UpdateAssurancePhase::kRollbackRequested;
        failure_ = UpdateAssuranceFailure::kSelfTestFailed;
        return false;
    }
    phase_ = UpdateAssurancePhase::kConfirmed;
    return true;
}

void UpdateAssuranceStateMachine::FailDownload(UpdateAssuranceFailure failure) {
    if (phase_ == UpdateAssurancePhase::kDownloading &&
        (failure == UpdateAssuranceFailure::kNetworkInterrupted ||
         failure == UpdateAssuranceFailure::kInsufficientSpace)) {
        Fail(failure);
    }
}

void UpdateAssuranceStateMachine::ConfirmationTimedOut() {
    if (phase_ == UpdateAssurancePhase::kPendingConfirmation) {
        phase_ = UpdateAssurancePhase::kRollbackRequested;
        failure_ = UpdateAssuranceFailure::kConfirmationTimedOut;
    }
}

bool ConfigurationMigrationTransaction::Run() {
    if (!store_.Snapshot()) return false;
    if (!store_.Apply() || !store_.Validate() || !store_.Commit()) {
        store_.Restore();
        return false;
    }
    return true;
}

}  // namespace hi_yaka
