#pragma once
/** ReXGlue : PS4 Audio Driver (sceAudioOut) — public declaration. */
#include <rex/platform.h>
#if REX_PLATFORM_PS4
#include <rex/audio/audio_driver.h>
#include <rex/thread.h>
namespace rex::audio::orbis {
class OrbisAudioDriver : public AudioDriver {
 public:
  OrbisAudioDriver(memory::Memory* memory, rex::thread::Semaphore* semaphore);
  ~OrbisAudioDriver() override;
  bool Initialize();
  void SubmitFrame(uint32_t frame_ptr) override;
  void Shutdown();
 private:
  rex::thread::Semaphore* semaphore_ = nullptr;
  int32_t audio_handle_ = -1;
  static const uint32_t kFrequency = 48000;
  static const uint32_t kInputChannels = 6;
  static const uint32_t kOutputChannels = 2;
  static const uint32_t kSamplesPerChannel = 256;
  static const uint32_t kInputFrameSamples = kInputChannels * kSamplesPerChannel;
  static const uint32_t kOutputFrameSamples = kOutputChannels * kSamplesPerChannel;
  float output_buffer_[kOutputFrameSamples];
};
}  // namespace rex::audio::orbis
#endif  // REX_PLATFORM_PS4
