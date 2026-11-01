#ifndef HI_YAKA_CAPABILITY_REGISTRY_H_
#define HI_YAKA_CAPABILITY_REGISTRY_H_

#include <array>

#include "product_types.h"

namespace hi_yaka {

struct CapabilityRecord {
    CapabilityState state{CapabilityState::kUnavailable};
    ErrorCode error{ErrorCode::kNone};
    uint64_t changed_at_ms{0};
};

class CapabilityRegistry {
public:
    bool Update(CapabilityId id, CapabilityState state, ErrorCode error, uint64_t changed_at_ms);
    CapabilityRecord Get(CapabilityId id) const;
    bool IsUsable(CapabilityId id) const;

private:
    static std::size_t Index(CapabilityId id);
    std::array<CapabilityRecord, kCapabilityCount> records_{};
};

}  // namespace hi_yaka

#endif  // HI_YAKA_CAPABILITY_REGISTRY_H_
