#include "SHA256.h"

#include <sstream>
#include <iomanip>
#include <cstring>

namespace
{
    constexpr std::array<uint32_t, 64> kConstants = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
        0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
        0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
        0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
        0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
        0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
        0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
        0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
        0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2 };

    inline uint32_t RotateRight(uint32_t value, uint32_t bits)
    {
        return (value >> bits) | (value << (32 - bits));
    }
}

SHA256::SHA256()
{
    Reset();
}

void SHA256::Reset()
{
    bitLength_ = 0;
    bufferSize_ = 0;
    finalized_ = false;
    state_ = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
               0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };
    buffer_.fill(0);
}

void SHA256::Update(const uint8_t* data, size_t length)
{
    if (finalized_)
    {
        Reset();
    }

    for (size_t i = 0; i < length; ++i)
    {
        buffer_[bufferSize_++] = data[i];
        if (bufferSize_ == buffer_.size())
        {
            Transform(buffer_.data());
            bitLength_ += 512;
            bufferSize_ = 0;
        }
    }
}

void SHA256::Update(const std::string& data)
{
    Update(reinterpret_cast<const uint8_t*>(data.data()), data.size());
}

std::array<uint8_t, 32> SHA256::Finalize()
{
    if (!finalized_)
    {
        Pad();
        finalized_ = true;
    }

    std::array<uint8_t, 32> hash{};
    for (size_t i = 0; i < state_.size(); ++i)
    {
        hash[i * 4] = static_cast<uint8_t>((state_[i] >> 24) & 0xff);
        hash[i * 4 + 1] = static_cast<uint8_t>((state_[i] >> 16) & 0xff);
        hash[i * 4 + 2] = static_cast<uint8_t>((state_[i] >> 8) & 0xff);
        hash[i * 4 + 3] = static_cast<uint8_t>((state_[i]) & 0xff);
    }

    return hash;
}

std::string SHA256::CalculateHex(const std::string& input)
{
    Reset();
    Update(input);
    auto hash = Finalize();
    std::ostringstream oss;
    for (auto byte : hash)
    {
        oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
    }
    return oss.str();
}

void SHA256::Transform(const uint8_t* chunk)
{
    uint32_t schedule[64];
    for (int i = 0; i < 16; ++i)
    {
        schedule[i] = (chunk[i * 4] << 24) | (chunk[i * 4 + 1] << 16) | (chunk[i * 4 + 2] << 8) | (chunk[i * 4 + 3]);
    }

    for (int i = 16; i < 64; ++i)
    {
        uint32_t s0 = RotateRight(schedule[i - 15], 7) ^ RotateRight(schedule[i - 15], 18) ^ (schedule[i - 15] >> 3);
        uint32_t s1 = RotateRight(schedule[i - 2], 17) ^ RotateRight(schedule[i - 2], 19) ^ (schedule[i - 2] >> 10);
        schedule[i] = schedule[i - 16] + s0 + schedule[i - 7] + s1;
    }

    uint32_t a = state_[0];
    uint32_t b = state_[1];
    uint32_t c = state_[2];
    uint32_t d = state_[3];
    uint32_t e = state_[4];
    uint32_t f = state_[5];
    uint32_t g = state_[6];
    uint32_t h = state_[7];

    for (int i = 0; i < 64; ++i)
    {
        uint32_t S1 = RotateRight(e, 6) ^ RotateRight(e, 11) ^ RotateRight(e, 25);
        uint32_t ch = (e & f) ^ ((~e) & g);
        uint32_t temp1 = h + S1 + ch + kConstants[i] + schedule[i];
        uint32_t S0 = RotateRight(a, 2) ^ RotateRight(a, 13) ^ RotateRight(a, 22);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t temp2 = S0 + maj;

        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }

    state_[0] += a;
    state_[1] += b;
    state_[2] += c;
    state_[3] += d;
    state_[4] += e;
    state_[5] += f;
    state_[6] += g;
    state_[7] += h;
}

void SHA256::Pad()
{
    bitLength_ += static_cast<uint64_t>(bufferSize_) * 8;

    buffer_[bufferSize_++] = 0x80;
    if (bufferSize_ > 56)
    {
        while (bufferSize_ < 64)
        {
            buffer_[bufferSize_++] = 0x00;
        }
        Transform(buffer_.data());
        bufferSize_ = 0;
    }

    while (bufferSize_ < 56)
    {
        buffer_[bufferSize_++] = 0x00;
    }

    uint64_t bitLenBigEndian = bitLength_;
    for (int i = 7; i >= 0; --i)
    {
        buffer_[bufferSize_++] = static_cast<uint8_t>((bitLenBigEndian >> (i * 8)) & 0xff);
    }

    Transform(buffer_.data());
    bufferSize_ = 0;
}

