#pragma once

#include <array>
#include <cstdint>
#include <string>
#include <vector>

class SHA256
{
public:
    SHA256();

    void Update(const uint8_t* data, size_t length);
    void Update(const std::string& data);
    std::array<uint8_t, 32> Finalize();
    std::string CalculateHex(const std::string& input);

private:
    void Transform(const uint8_t* chunk);
    void Pad();
    void Reset();

private:
    uint64_t bitLength_;
    std::array<uint8_t, 64> buffer_;
    std::array<uint32_t, 8> state_;
    size_t bufferSize_;
    bool finalized_;
};

