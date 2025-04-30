#pragma once
#ifndef CPU_SAMPLER_H
#define CPU_SAMPLER_H

#include <cstdint>
#include <chrono>
#include <ostream>

namespace rmm
{

struct CPUSample
{
    CPUSample();

    std::uint64_t m_idle_jiffies = 0;
    std::chrono::steady_clock::time_point m_sample_time;
};

struct CPUSampleDelta
{
    using FloatMilliseconds = std::chrono::duration<double, std::ratio<1, 1000>>;
    FloatMilliseconds m_idle_duration = {};
    std::chrono::milliseconds m_duration = {};
};

CPUSample GetCPUSample();

CPUSampleDelta operator-(const CPUSample & lhs, const CPUSample & rhs);

std::ostream & operator <<(std::ostream & os, const CPUSampleDelta & sampleDelta);

}

#endif