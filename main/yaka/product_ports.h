#ifndef HI_YAKA_PRODUCT_PORTS_H_
#define HI_YAKA_PRODUCT_PORTS_H_

#include "product_types.h"

namespace hi_yaka {

enum class FeedbackSignal : uint8_t {
    kAccepted,
    kCompleted,
    kCancelled,
    kWarning,
    kError,
};

class DisplayPort {
public:
    virtual ~DisplayPort() = default;
    virtual void Present(const ProductSnapshot& snapshot) = 0;
};

class InputPort {
public:
    virtual ~InputPort() = default;
    virtual void SetEnabled(bool enabled) = 0;
};

class AudioPort {
public:
    virtual ~AudioPort() = default;
    virtual bool BeginDialogue() = 0;
    virtual void CancelDialogue() = 0;
    virtual bool StartRecordingCapture() = 0;
    virtual void StopRecordingCapture() = 0;
};

class StoragePort {
public:
    virtual ~StoragePort() = default;
    virtual bool OpenRecording() = 0;
    virtual bool CommitRecording() = 0;
    virtual void AbortRecording() = 0;
};

class NetworkPort {
public:
    virtual ~NetworkPort() = default;
    virtual bool BeginProvisioning() = 0;
    virtual bool BeginDialogueSession() = 0;
    virtual void CancelOperation() = 0;
};

class NfcPort {
public:
    virtual ~NfcPort() = default;
    virtual bool BeginScan() = 0;
    virtual void CancelScan() = 0;
};

class ImuPort {
public:
    virtual ~ImuPort() = default;
    virtual void SetMonitoring(bool enabled) = 0;
};

class FeedbackPort {
public:
    virtual ~FeedbackPort() = default;
    virtual void Emit(FeedbackSignal signal) = 0;
};

class PowerPort {
public:
    virtual ~PowerPort() = default;
    virtual bool PrepareShutdown() = 0;
};

class UpdatePort {
public:
    virtual ~UpdatePort() = default;
    virtual bool BeginUpdate() = 0;
    virtual void CancelUpdate() = 0;
};

struct ProductPorts {
    DisplayPort* display{nullptr};
    InputPort* input{nullptr};
    AudioPort* audio{nullptr};
    StoragePort* storage{nullptr};
    NetworkPort* network{nullptr};
    NfcPort* nfc{nullptr};
    ImuPort* imu{nullptr};
    FeedbackPort* feedback{nullptr};
    PowerPort* power{nullptr};
    UpdatePort* update{nullptr};
};

}  // namespace hi_yaka

#endif  // HI_YAKA_PRODUCT_PORTS_H_
