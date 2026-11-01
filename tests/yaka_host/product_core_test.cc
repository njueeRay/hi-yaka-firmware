#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "bounded_queue.h"
#include "capability_registry.h"
#include "product_controller.h"
#include "simulated_product_ports.h"

namespace {

using namespace hi_yaka;

int failures = 0;

#define EXPECT_TRUE(expression)                                                        \
    do {                                                                               \
        if (!(expression)) {                                                           \
            std::cerr << __FILE__ << ':' << __LINE__ << " expectation failed: "       \
                      << #expression << '\n';                                          \
            ++failures;                                                                \
        }                                                                              \
    } while (false)

#define EXPECT_EQ(actual, expected) EXPECT_TRUE((actual) == (expected))

ProductEvent Event(ProductEventType type, uint32_t correlation = 0) {
    ProductEvent event;
    event.type = type;
    event.source = EventSource::kTest;
    event.timestamp_ms = 10;
    event.correlation_id = correlation;
    return event;
}

void MakeAllCapabilitiesReady(CapabilityRegistry& registry) {
    for (std::size_t index = 0; index < kCapabilityCount; ++index) {
        registry.Update(static_cast<CapabilityId>(index), CapabilityState::kReady,
                        ErrorCode::kNone, 1);
    }
}

void Dispatch(ProductController& controller, ProductEvent event, uint64_t now_ms = 10) {
    EXPECT_TRUE(controller.Post(event) != QueuePushResult::kRejectedFull);
    const auto result = controller.ProcessNext(now_ms);
    EXPECT_TRUE(result == ProcessResult::kHandled || result == ProcessResult::kIgnoredDuplicate ||
                result == ProcessResult::kRejected || result == ProcessResult::kExpired);
}

struct Fixture {
    Fixture() : controller(simulated.Bind(), capabilities) {
        MakeAllCapabilitiesReady(capabilities);
        Dispatch(controller, Event(ProductEventType::kBootCompleted, 1));
    }

    CapabilityRegistry capabilities;
    SimulatedProductPorts simulated;
    ProductController controller;
};

void TestBoundedQueuePolicies() {
    BoundedQueue<int, 2> rejecting(QueueFullPolicy::kRejectNewest);
    EXPECT_EQ(rejecting.TryPush(1), QueuePushResult::kAccepted);
    EXPECT_EQ(rejecting.TryPush(2), QueuePushResult::kAccepted);
    EXPECT_EQ(rejecting.TryPush(3), QueuePushResult::kRejectedFull);
    EXPECT_EQ(rejecting.Metrics().peak_depth, 2U);
    EXPECT_EQ(rejecting.Metrics().rejected, 1U);

    BoundedQueue<int, 2> dropping(QueueFullPolicy::kDropOldest);
    dropping.TryPush(1);
    dropping.TryPush(2);
    EXPECT_EQ(dropping.TryPush(3), QueuePushResult::kAcceptedAfterDroppingOldest);
    int value = 0;
    EXPECT_TRUE(dropping.TryPop(value));
    EXPECT_EQ(value, 2);
    EXPECT_EQ(dropping.Metrics().dropped_oldest, 1U);

    AudioBlockQueue audio_blocks(QueueFullPolicy::kDropOldest);
    StorageWriteQueue storage_writes(QueueFullPolicy::kRejectNewest);
    EXPECT_EQ(audio_blocks.CapacityValue(), kAudioBlockQueueCapacity);
    EXPECT_EQ(storage_writes.CapacityValue(), kStorageWriteQueueCapacity);
}

void TestBoundedQueueSupportsConcurrentProducers() {
    BoundedQueue<int, 32> queue(QueueFullPolicy::kRejectNewest);
    std::vector<std::thread> producers;
    for (int producer = 0; producer < 4; ++producer) {
        producers.emplace_back([&queue, producer]() {
            for (int item = 0; item < 1000; ++item) {
                queue.TryPush(producer * 1000 + item);
            }
        });
    }
    for (auto& producer : producers) {
        producer.join();
    }
    const auto metrics = queue.Metrics();
    EXPECT_EQ(metrics.depth, 32U);
    EXPECT_EQ(metrics.accepted, 32U);
    EXPECT_EQ(metrics.rejected, 3968U);
}

void TestBootAndDuplicateCorrelationAreIdempotent() {
    Fixture fixture;
    EXPECT_EQ(fixture.controller.Snapshot().foreground, ForegroundMode::kHome);
    const auto generation = fixture.controller.Snapshot().generation;
    Dispatch(fixture.controller, Event(ProductEventType::kBootCompleted, 1));
    EXPECT_EQ(fixture.controller.Snapshot().generation, generation);
    EXPECT_EQ(fixture.controller.Metrics().duplicate_events, 1U);
}

void TestDialogueLifecycleAndCancel() {
    Fixture fixture;
    Dispatch(fixture.controller, Event(ProductEventType::kNetworkOnline, 2));
    Dispatch(fixture.controller, Event(ProductEventType::kDialogueRequested, 3));
    EXPECT_EQ(fixture.controller.Snapshot().foreground, ForegroundMode::kDialogue);
    EXPECT_EQ(fixture.controller.Snapshot().dialogue, DialoguePhase::kConnecting);
    Dispatch(fixture.controller, Event(ProductEventType::kDialogueListening, 4));
    Dispatch(fixture.controller, Event(ProductEventType::kDialogueSpeaking, 5));
    EXPECT_EQ(fixture.controller.Snapshot().dialogue, DialoguePhase::kSpeaking);
    Dispatch(fixture.controller, Event(ProductEventType::kCancel, 6));
    EXPECT_EQ(fixture.controller.Snapshot().foreground, ForegroundMode::kHome);
    EXPECT_EQ(fixture.simulated.calls.dialogue_cancel, 1U);
    EXPECT_EQ(fixture.simulated.calls.network_cancel, 1U);
}

void TestRecordingCommitAndCapabilityFailure() {
    Fixture fixture;
    Dispatch(fixture.controller, Event(ProductEventType::kRecordingRequested, 10));
    EXPECT_EQ(fixture.controller.Snapshot().foreground, ForegroundMode::kRecording);
    EXPECT_EQ(fixture.simulated.calls.storage_begin, 1U);
    EXPECT_EQ(fixture.simulated.calls.recording_begin, 1U);
    Dispatch(fixture.controller, Event(ProductEventType::kRecordingStopped, 11));
    EXPECT_EQ(fixture.controller.Snapshot().foreground, ForegroundMode::kHome);
    EXPECT_EQ(fixture.simulated.calls.storage_commit, 1U);

    fixture.capabilities.Update(CapabilityId::kStorage, CapabilityState::kFailed,
                                ErrorCode::kStorageUnavailable, 20);
    Dispatch(fixture.controller, Event(ProductEventType::kRecordingRequested, 12));
    EXPECT_EQ(fixture.controller.Snapshot().foreground, ForegroundMode::kError);
    EXPECT_EQ(fixture.controller.Snapshot().active_error, ErrorCode::kStorageUnavailable);
    Dispatch(fixture.controller, Event(ProductEventType::kRecover, 13));
    EXPECT_EQ(fixture.controller.Snapshot().foreground, ForegroundMode::kHome);
}

void TestNonCriticalCapabilityFailureDoesNotTrapUi() {
    Fixture fixture;
    auto event = Event(ProductEventType::kCapabilityChanged, 20);
    event.capability = CapabilityId::kImu;
    event.capability_state = CapabilityState::kFailed;
    event.error = ErrorCode::kCapabilityFailed;
    Dispatch(fixture.controller, event);
    EXPECT_EQ(fixture.controller.Snapshot().foreground, ForegroundMode::kHome);
    EXPECT_EQ(fixture.controller.Snapshot().active_error, ErrorCode::kNone);
}

void TestTimeoutAndExpiredEvents() {
    Fixture fixture;
    auto expired = Event(ProductEventType::kSettingsRequested, 30);
    expired.deadline_ms = 50;
    EXPECT_EQ(fixture.controller.Post(expired), QueuePushResult::kAccepted);
    EXPECT_EQ(fixture.controller.ProcessNext(51), ProcessResult::kExpired);
    EXPECT_EQ(fixture.controller.Snapshot().foreground, ForegroundMode::kHome);

    auto timeout = Event(ProductEventType::kOperationTimedOut, 31);
    timeout.error = ErrorCode::kOperationTimedOut;
    Dispatch(fixture.controller, timeout);
    EXPECT_EQ(fixture.controller.Snapshot().foreground, ForegroundMode::kError);
    Dispatch(fixture.controller, Event(ProductEventType::kRecover, 32));
    EXPECT_EQ(fixture.controller.Snapshot().foreground, ForegroundMode::kHome);
}

void TestUpdateRequiresConfirmation() {
    Fixture fixture;
    Dispatch(fixture.controller, Event(ProductEventType::kNetworkOnline, 40));
    Dispatch(fixture.controller, Event(ProductEventType::kUpdateRequested, 41));
    EXPECT_EQ(fixture.controller.Snapshot().update, UpdatePhase::kAwaitingConfirmation);
    EXPECT_EQ(fixture.simulated.calls.update_begin, 0U);
    Dispatch(fixture.controller, Event(ProductEventType::kUpdateConfirmed, 42));
    EXPECT_EQ(fixture.controller.Snapshot().update, UpdatePhase::kDownloading);
    EXPECT_EQ(fixture.simulated.calls.update_begin, 1U);
}

void TestShutdownIsIdempotent() {
    Fixture fixture;
    Dispatch(fixture.controller, Event(ProductEventType::kShutdownRequested, 50));
    EXPECT_EQ(fixture.controller.Snapshot().power, PowerPhase::kPreparingShutdown);
    Dispatch(fixture.controller, Event(ProductEventType::kShutdownRequested, 51));
    EXPECT_EQ(fixture.simulated.calls.shutdown_prepare, 1U);
    Dispatch(fixture.controller, Event(ProductEventType::kShutdownReady, 52));
    EXPECT_EQ(fixture.controller.Snapshot().power, PowerPhase::kWaitingForPowerOff);
}

}  // namespace

int main() {
    TestBoundedQueuePolicies();
    TestBoundedQueueSupportsConcurrentProducers();
    TestBootAndDuplicateCorrelationAreIdempotent();
    TestDialogueLifecycleAndCancel();
    TestRecordingCommitAndCapabilityFailure();
    TestNonCriticalCapabilityFailureDoesNotTrapUi();
    TestTimeoutAndExpiredEvents();
    TestUpdateRequiresConfirmation();
    TestShutdownIsIdempotent();

    if (failures != 0) {
        std::cerr << failures << " test expectation(s) failed\n";
        return EXIT_FAILURE;
    }
    std::cout << "HiYaKa product core host tests passed\n";
    return EXIT_SUCCESS;
}
