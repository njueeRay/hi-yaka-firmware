#ifndef HI_YAKA_UPDATE_ASSURANCE_H_
#define HI_YAKA_UPDATE_ASSURANCE_H_

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace hi_yaka {

struct SemanticVersion {
    uint16_t major{0};
    uint16_t minor{0};
    uint16_t patch{0};

    static bool Parse(std::string_view text, SemanticVersion& result);
};

int Compare(const SemanticVersion& left, const SemanticVersion& right);

struct UpdateManifest {
    std::string_view board;
    std::string_view version;
    std::string_view minimum_installed_version;
    uint16_t minimum_hardware_revision{0};
    uint16_t maximum_hardware_revision{0};
    std::size_t image_size{0};
    std::string_view image_sha256;
    std::string_view signature;
};

class PackageSignatureVerifier {
public:
    virtual ~PackageSignatureVerifier() = default;
    virtual bool Verify(const UpdateManifest& manifest) const = 0;
};

enum class UpdateRejection : uint8_t {
    kNone,
    kWrongBoard,
    kInvalidVersion,
    kVersionNotNewer,
    kInstalledVersionTooOld,
    kHardwareIncompatible,
    kImageEmpty,
    kImageTooLarge,
    kInvalidDigest,
    kInvalidSignature,
};

struct UpdatePolicy {
    static constexpr std::string_view kBoard = "hi-yaka";
    static constexpr std::size_t kOtaSlotBytes = 0x3f0000;
    static constexpr std::size_t kSafetyMarginBytes = 0x40000;
    static constexpr std::size_t kMaximumImageBytes = kOtaSlotBytes - kSafetyMarginBytes;

    uint16_t hardware_revision{0};
    std::string_view installed_version;
};

UpdateRejection ValidateUpdateManifest(const UpdateManifest& manifest,
                                       const UpdatePolicy& policy,
                                       const PackageSignatureVerifier& verifier);

enum class UpdateAssurancePhase : uint8_t {
    kIdle,
    kAccepted,
    kDownloading,
    kVerified,
    kPendingReboot,
    kPendingConfirmation,
    kConfirmed,
    kRollbackRequested,
    kFailed,
};

enum class UpdateAssuranceFailure : uint8_t {
    kNone,
    kManifestRejected,
    kNetworkInterrupted,
    kInsufficientSpace,
    kDigestMismatch,
    kSignatureRejected,
    kConfigurationMigrationFailed,
    kSelfTestFailed,
    kConfirmationTimedOut,
};

class UpdateAssuranceStateMachine {
public:
    bool AcceptManifest(UpdateRejection rejection);
    bool BeginDownload();
    bool CompleteDownload(bool digest_matches, bool signature_valid);
    bool PrepareReboot(bool migration_succeeded);
    bool ObserveFirstBoot();
    bool ConfirmAfterSelfTest(bool self_test_succeeded);
    void FailDownload(UpdateAssuranceFailure failure);
    void ConfirmationTimedOut();

    UpdateAssurancePhase Phase() const { return phase_; }
    UpdateAssuranceFailure Failure() const { return failure_; }

private:
    void Fail(UpdateAssuranceFailure failure);

    UpdateAssurancePhase phase_{UpdateAssurancePhase::kIdle};
    UpdateAssuranceFailure failure_{UpdateAssuranceFailure::kNone};
};

class ConfigurationMigrationStore {
public:
    virtual ~ConfigurationMigrationStore() = default;
    virtual bool Snapshot() = 0;
    virtual bool Apply() = 0;
    virtual bool Validate() = 0;
    virtual bool Commit() = 0;
    virtual bool Restore() = 0;
};

class ConfigurationMigrationTransaction {
public:
    explicit ConfigurationMigrationTransaction(ConfigurationMigrationStore& store) : store_(store) {}
    bool Run();

private:
    ConfigurationMigrationStore& store_;
};

}  // namespace hi_yaka

#endif  // HI_YAKA_UPDATE_ASSURANCE_H_
