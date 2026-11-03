#pragma once
/**
 * PS4 input driver backed by scePad (OpenOrbis DualShock 4 / DualSense).
 * Public mirror of src/input/ps4/ps4_input_driver.h so the consumer
 * executable can reference the type from CreateDefaultInputSystem().
 */
#include <rex/platform.h>

#if REX_PLATFORM_PS4

#include <array>
#include <atomic>
#include <mutex>
#include <rex/input/input_driver.h>

struct OrbisPadData;

namespace rex::input::ps4 {

static constexpr size_t kPS4UserCount = 4;

class PS4InputDriver final : public InputDriver {
 public:
  explicit PS4InputDriver(rex::ui::Window* window, size_t window_z_order);
  ~PS4InputDriver() override;
  X_STATUS Setup() override;
  X_RESULT GetCapabilities(uint32_t user_index, uint32_t flags,
                           X_INPUT_CAPABILITIES* out_caps) override;
  X_RESULT GetState(uint32_t user_index, X_INPUT_STATE* out_state) override;
  X_RESULT SetState(uint32_t user_index, X_INPUT_VIBRATION* vibration) override;
  X_RESULT GetKeystroke(uint32_t user_index, uint32_t flags,
                        X_INPUT_KEYSTROKE* out_keystroke) override;

 private:
  struct PadSlot {
    int32_t user_id = -1;
    int32_t handle = -1;
    uint32_t last_buttons = 0;
    uint32_t packet_number = 0;
    uint64_t last_timestamp = 0;
    bool connected = false;
  };
  void RefreshUsers();
  static void TranslatePadState(const ::OrbisPadData& pad, X_INPUT_GAMEPAD& out);
  std::mutex mutex_;
  std::array<PadSlot, kPS4UserCount> slots_{};
  std::atomic<bool> pad_initialized_{false};
  std::atomic<bool> user_service_initialized_{false};
};

}  // namespace rex::input::ps4
#endif  // REX_PLATFORM_PS4
