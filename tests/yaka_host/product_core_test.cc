#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "bounded_queue.h"
#include "business_flows.h"
#include "capability_registry.h"
#include "host_recording_backend.h"
#include "product_controller.h"
#include "recording_store.h"
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

void TestDialogueInterruptionReconnectAndTimeout() {
    Fixture fixture;
    Dispatch(fixture.controller, Event(ProductEventType::kNetworkOnline, 60));
    Dispatch(fixture.controller, Event(ProductEventType::kDialogueRequested, 61));
    Dispatch(fixture.controller, Event(ProductEventType::kDialogueSpeaking, 62));
    Dispatch(fixture.controller, Event(ProductEventType::kDialogueInterrupted, 63));
    EXPECT_EQ(fixture.controller.Snapshot().dialogue, DialoguePhase::kInterrupted);
    EXPECT_EQ(fixture.simulated.calls.dialogue_interrupt, 1U);
    Dispatch(fixture.controller, Event(ProductEventType::kDialogueListening, 64));
    Dispatch(fixture.controller, Event(ProductEventType::kNetworkOffline, 65));
    EXPECT_EQ(fixture.controller.Snapshot().foreground, ForegroundMode::kDialogue);
    EXPECT_EQ(fixture.controller.Snapshot().dialogue, DialoguePhase::kInterrupted);
    Dispatch(fixture.controller, Event(ProductEventType::kNetworkOnline, 66));
    EXPECT_EQ(fixture.controller.Snapshot().dialogue, DialoguePhase::kConnecting);

    DialogueDeadlineTracker deadlines;
    deadlines.Observe(fixture.controller.Snapshot(), 100);
    ProductEvent timeout;
    EXPECT_TRUE(!deadlines.Poll(15100, timeout));
    EXPECT_TRUE(deadlines.Poll(15101, timeout));
    Dispatch(fixture.controller, timeout, 15101);
    EXPECT_EQ(fixture.controller.Snapshot().foreground, ForegroundMode::kError);
    EXPECT_EQ(fixture.controller.Snapshot().active_error, ErrorCode::kOperationTimedOut);
    EXPECT_EQ(fixture.simulated.calls.dialogue_cancel, 1U);
    EXPECT_EQ(fixture.simulated.calls.network_cancel, 1U);
}

void TestConfigurableInputAndPageFlow() {
    const InputBinding bindings[] = {
        {PhysicalKey::kDown, KeyGesture::kPress, ForegroundMode::kHome,
         ProductAction::kFocusNext},
        {PhysicalKey::kConfirm, KeyGesture::kPress, ForegroundMode::kHome,
         ProductAction::kActivate},
        {PhysicalKey::kPrimary, KeyGesture::kLongPress, ForegroundMode::kHome,
         ProductAction::kStartRecording},
    };
    InputActionMap map;
    EXPECT_TRUE(map.Configure(bindings, 3));
    EXPECT_EQ(map.Resolve(PhysicalKey::kPrimary, KeyGesture::kLongPress,
                          ForegroundMode::kHome),
              ProductAction::kStartRecording);
    EXPECT_EQ(map.Resolve(PhysicalKey::kPrimary, KeyGesture::kPress, ForegroundMode::kHome),
              ProductAction::kNone);
    const InputBinding duplicate[] = {bindings[0], bindings[0]};
    EXPECT_TRUE(!map.Configure(duplicate, 2));

    PageFlowModel pages;
    ProductSnapshot snapshot;
    snapshot.power = PowerPhase::kRunning;
    snapshot.foreground = ForegroundMode::kHome;
    pages.Present(snapshot);
    EXPECT_EQ(pages.View().page, PageId::kHome);
    EXPECT_EQ(pages.View().focus_count, 4U);
    EXPECT_EQ(PageViewModel::kWidth, 200U);
    EXPECT_EQ(PageViewModel::kHeight, 320U);
    pages.MoveFocus(1);
    EXPECT_EQ(pages.Activate(), ProductAction::kStartRecording);
    snapshot.foreground = ForegroundMode::kError;
    snapshot.active_error = ErrorCode::kStorageUnavailable;
    pages.Present(snapshot);
    EXPECT_EQ(pages.View().page, PageId::kError);
    EXPECT_EQ(pages.View().error, ErrorCode::kStorageUnavailable);
    EXPECT_EQ(pages.Back(), ProductAction::kBack);
}

std::filesystem::path FreshRecordingDirectory(const char* name) {
    const auto path = std::filesystem::temp_directory_path() /
                      (std::string("hi-yaka-") + name);
    std::error_code error;
    std::filesystem::remove_all(path, error);
    std::filesystem::create_directories(path, error);
    EXPECT_TRUE(!error);
    return path;
}

void TestRecordingCommitAndPowerLossRecoveryOnHostFilesystem() {
    const auto path = FreshRecordingDirectory("recording-commit-recovery");
    test::HostRecordingBackend backend(path);
    std::vector<uint8_t> block(5000, 0x5A);

    RecordingStore committed(backend);
    EXPECT_TRUE(committed.Begin(100));
    EXPECT_TRUE(!committed.Begin(100));
    EXPECT_TRUE(committed.Append(block.data(), block.size()));
    EXPECT_TRUE(committed.Commit());
    EXPECT_TRUE(backend.FinalExists(100, false));
    EXPECT_EQ(backend.FinalSize(100, false), block.size());

    {
        RecordingStore interrupted(backend);
        EXPECT_TRUE(interrupted.Begin(101));
        EXPECT_TRUE(interrupted.Append(block.data(), block.size()));
        const uint8_t tail[3] = {1, 2, 3};
        EXPECT_TRUE(interrupted.Append(tail, sizeof(tail)));
    }
    RecordingStore restarted(backend);
    const auto recovery = restarted.RecoverPending();
    EXPECT_EQ(recovery.recovered, 1U);
    EXPECT_EQ(recovery.discarded, 0U);
    EXPECT_TRUE(backend.FinalExists(101, true));
    EXPECT_EQ(backend.FinalSize(101, true), block.size());

    std::error_code error;
    std::filesystem::remove_all(path, error);
}

void TestRecordingControllerUsesReliableStoreAdapter() {
    const auto path = FreshRecordingDirectory("recording-controller");
    test::HostRecordingBackend backend(path);
    RecordingStore store(backend);
    RecordingStoragePortAdapter storage(store);
    storage.SetNextRecordingId(150);

    CapabilityRegistry capabilities;
    MakeAllCapabilitiesReady(capabilities);
    SimulatedProductPorts simulated;
    ProductPorts ports = simulated.Bind();
    ports.storage = &storage;
    ProductController controller(ports, capabilities);
    Dispatch(controller, Event(ProductEventType::kBootCompleted, 140));
    Dispatch(controller, Event(ProductEventType::kRecordingRequested, 141));
    std::vector<uint8_t> audio(5000, 0x6B);
    EXPECT_TRUE(storage.AppendAudio(audio.data(), audio.size()));
    Dispatch(controller, Event(ProductEventType::kRecordingStopped, 142));
    EXPECT_EQ(controller.Snapshot().foreground, ForegroundMode::kHome);
    EXPECT_TRUE(backend.FinalExists(150, false));
    EXPECT_EQ(backend.FinalSize(150, false), audio.size());

    std::error_code error;
    std::filesystem::remove_all(path, error);
}

void TestRecordingFullDiskTruncationAndRepeatedRestart() {
    const auto full_path = FreshRecordingDirectory("recording-full");
    test::HostRecordingBackend full_backend(full_path, 5000);
    std::vector<uint8_t> checkpoint_block(4096, 0x11);
    std::vector<uint8_t> overflow(1000, 0x22);
    {
        RecordingStore store(full_backend);
        EXPECT_TRUE(store.Begin(200));
        EXPECT_TRUE(store.Append(checkpoint_block.data(), checkpoint_block.size()));
        EXPECT_TRUE(!store.Append(overflow.data(), overflow.size()));
        EXPECT_EQ(store.State(), RecordingStoreState::kFaulted);
    }
    RecordingStore full_restart(full_backend);
    auto recovery = full_restart.RecoverPending();
    EXPECT_EQ(recovery.recovered, 1U);
    EXPECT_EQ(full_restart.RecoverPending().recovered, 0U);
    EXPECT_EQ(full_backend.FinalSize(200, true), checkpoint_block.size());

    const auto truncated_path = FreshRecordingDirectory("recording-truncated");
    test::HostRecordingBackend truncated_backend(truncated_path);
    {
        RecordingStore store(truncated_backend);
        EXPECT_TRUE(store.Begin(201));
        EXPECT_TRUE(store.Append(checkpoint_block.data(), checkpoint_block.size()));
    }
    EXPECT_TRUE(truncated_backend.Truncate(201, 2000));
    RecordingStore truncated_restart(truncated_backend);
    recovery = truncated_restart.RecoverPending();
    EXPECT_EQ(recovery.discarded, 1U);
    EXPECT_TRUE(!truncated_backend.FinalExists(201, true));

    std::error_code error;
    std::filesystem::remove_all(full_path, error);
    error.clear();
    std::filesystem::remove_all(truncated_path, error);
}

void TestAuxiliaryCapabilitiesDegradeWithoutBlockingHome() {
    Fixture fixture;
    fixture.simulated.SetFeedbackChannelAvailable(FeedbackChannel::kVibration, false);
    for (const auto capability : {CapabilityId::kImu, CapabilityId::kFeedback,
                                  CapabilityId::kPower, CapabilityId::kNfc}) {
        auto event = Event(ProductEventType::kCapabilityChanged,
                           300 + static_cast<uint32_t>(capability));
        event.capability = capability;
        event.capability_state = CapabilityState::kDegraded;
        event.error = ErrorCode::kCapabilityFailed;
        Dispatch(fixture.controller, event);
        EXPECT_EQ(fixture.controller.Snapshot().foreground, ForegroundMode::kHome);
    }
    Dispatch(fixture.controller, Event(ProductEventType::kNfcRequested, 320));
    EXPECT_EQ(fixture.controller.Snapshot().foreground, ForegroundMode::kNfc);
    Dispatch(fixture.controller, Event(ProductEventType::kCancel, 321));
    EXPECT_EQ(fixture.controller.Snapshot().foreground, ForegroundMode::kHome);
    EXPECT_TRUE(fixture.simulated.calls.feedback_rgb > 0);
    EXPECT_TRUE(fixture.simulated.calls.feedback_led_array > 0);
    EXPECT_EQ(fixture.simulated.calls.feedback_vibration, 0U);
    EXPECT_TRUE(fixture.simulated.calls.feedback_tone > 0);

    Fixture power_failure;
    power_failure.simulated.accept_power = false;
    Dispatch(power_failure.controller, Event(ProductEventType::kShutdownRequested, 330));
    EXPECT_EQ(power_failure.controller.Snapshot().foreground, ForegroundMode::kError);
    EXPECT_EQ(power_failure.controller.Snapshot().active_error, ErrorCode::kPortRejected);
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
    TestDialogueInterruptionReconnectAndTimeout();
    TestConfigurableInputAndPageFlow();
    TestRecordingCommitAndPowerLossRecoveryOnHostFilesystem();
    TestRecordingControllerUsesReliableStoreAdapter();
    TestRecordingFullDiskTruncationAndRepeatedRestart();
    TestAuxiliaryCapabilitiesDegradeWithoutBlockingHome();
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
