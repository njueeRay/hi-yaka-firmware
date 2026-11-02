#ifndef HI_YAKA_BOARD_API_H_
#define HI_YAKA_BOARD_API_H_

#include "product_types.h"
#include "product_ports.h"
#include "recording_store.h"

namespace hi_yaka {

bool BoardCapabilityReady(CapabilityId capability);
void SetBoardInputEnabled(bool enabled);
RecordingStoragePortAdapter* BoardRecordingPort();
FeedbackPort* BoardFeedbackPort();

}  // namespace hi_yaka

#endif  // HI_YAKA_BOARD_API_H_
