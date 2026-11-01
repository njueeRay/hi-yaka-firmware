#include "capability_registry.h"

namespace hi_yaka {

std::size_t CapabilityRegistry::Index(CapabilityId id) {
    const auto index = static_cast<std::size_t>(id);
    return index < kCapabilityCount ? index : 0;
}

bool CapabilityRegistry::Update(CapabilityId id, CapabilityState state, ErrorCode error,
                                uint64_t changed_at_ms) {
    if (id == CapabilityId::kCount) {
        return false;
    }
    auto& record = records_[Index(id)];
    const bool changed = record.state != state || record.error != error;
    record.state = state;
    record.error = error;
    if (changed) {
        record.changed_at_ms = changed_at_ms;
    }
    return changed;
}

CapabilityRecord CapabilityRegistry::Get(CapabilityId id) const {
    if (id == CapabilityId::kCount) {
        return {};
    }
    return records_[Index(id)];
}

bool CapabilityRegistry::IsUsable(CapabilityId id) const {
    const auto state = Get(id).state;
    return state == CapabilityState::kReady || state == CapabilityState::kDegraded;
}

}  // namespace hi_yaka
