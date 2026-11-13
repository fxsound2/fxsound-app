/*
FxSound Linux port — linux/float_convert.h (Phase 6)
F32<->S16 helpers with clamping, no attenuation. DSP has only short* interface
(DfxDsp.h:44). PipeWire streams are F32 (Phase 7). Re-blocker for variable
n_frames (DSP chunks >16384 internally, but fixed block NOT required).
Untested.
*/
#pragma once
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace fxsound_linux {
inline int16_t f32_to_s16(float v) {
  if (!std::isfinite(v)) return 0;
  float c = std::max(-1.0f, std::min(1.0f, v));
  int32_t s = (int32_t)std::lrintf(c * 32767.0f);
  return (int16_t)std::max(-32768, std::min(32767, s));
}
inline float s16_to_f32(int16_t s) { return (float)s / 32767.0f; }

inline void f32_to_s16_block(const float* in, int16_t* out, size_t n) {
  for (size_t i = 0; i < n; ++i) out[i] = f32_to_s16(in[i]);
}
inline void s16_to_f32_block(const int16_t* in, float* out, size_t n) {
  for (size_t i = 0; i < n; ++i) out[i] = s16_to_f32(in[i]);
}

// Re-blocker: DSP accepts variable num_sets (chunks >16384 internally,
// u_dfxp.h:38 DAW_MAX_BUFFER_SIZE 16384), so no fixed block needed.
// This helper accumulates PipeWire variable quanta up to maxQuantum for callers
// that want fixed-size DSP calls. Max 8192 frames per Phase 7.
class Reblocker {
 public:
  explicit Reblocker(size_t maxFrames = 8192, int channels = 2)
      : max_(maxFrames), ch_(channels) {
    buf_.reserve(max_ * (size_t)ch_);
  }
  // Push F32 interleaved frames; returns complete blocks via callback.
  template <typename Fn>
  void push(const float* frames, size_t nFrames, Fn&& emit) {
    size_t off = 0;
    while (off < nFrames) {
      size_t need = max_ - buf_.size() / (size_t)ch_;
      size_t take = std::min(need, nFrames - off);
      buf_.insert(buf_.end(), frames + off * (size_t)ch_,
                  frames + (off + take) * (size_t)ch_);
      off += take;
      if (buf_.size() / (size_t)ch_ == max_) {
        emit(buf_.data(), max_);
        buf_.clear();
      }
    }
  }
  template <typename Fn>
  void flush(Fn&& emit) {
    if (!buf_.empty()) {
      emit(buf_.data(), buf_.size() / (size_t)ch_);
      buf_.clear();
    }
  }

 private:
  size_t max_;
  int ch_;
  std::vector<float> buf_;
};
}  // namespace fxsound_linux
