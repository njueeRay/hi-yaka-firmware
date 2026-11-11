#ifndef HI_YAKA_WAV_WRITER_H_
#define HI_YAKA_WAV_WRITER_H_

#include <cstddef>
#include <cstdint>
#include <cstdio>

class HiYaKaWavWriter final {
public:
    HiYaKaWavWriter() = default;
    ~HiYaKaWavWriter();

    bool Open(const char* path, uint32_t sample_rate, uint16_t channels);
    bool Append(const int16_t* samples, std::size_t sample_count);
    bool Close();

    bool open() const { return file_ != nullptr; }
    uint32_t data_bytes() const { return data_bytes_; }

private:
    bool WriteHeader(uint32_t data_bytes);

    FILE* file_ = nullptr;
    uint32_t sample_rate_ = 0;
    uint16_t channels_ = 0;
    uint32_t data_bytes_ = 0;
};

#endif  // HI_YAKA_WAV_WRITER_H_
