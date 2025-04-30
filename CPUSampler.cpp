#include "CPUSampler.h"
#include <fstream>
#include <system_error>
#include <unistd.h>
#include <iomanip>

namespace rmm
{

static const auto CLK_TCK = static_cast<double>(sysconf(_SC_CLK_TCK)); // assume this is constant during the life of the process
static const auto NPROC = static_cast<unsigned long>(sysconf(_SC_NPROCESSORS_ONLN));

CPUSample GetCPUSample()
{
    std::ifstream cpuStatStream("/proc/stat");
    if (!cpuStatStream)
    {
        throw std::system_error{errno, std::system_category(), "Failed to open /proc/stat"};
    }

    CPUSample sample;
    // just read the first few values... don't care after that
    std::string cpuName;
    std::uint64_t user = 0;
    std::uint64_t nice = 0;
    std::uint64_t system = 0;
    cpuStatStream >> cpuName >> user >> nice >> system >> sample.m_idle_jiffies;
    if (cpuStatStream) // if all conversions o.k.
    {
        return sample;
    }
    return {};
}

CPUSampleDelta operator-(const CPUSample &lhs, const CPUSample &rhs)
{
    if (lhs.m_sample_time == rhs.m_sample_time)
    {
        return {}; // no time difference; so samples were too close together
    }

    // convert jiffies to milliseconds
    // (average the number of jiffies per second over the number of processors)
    const auto idleJiffies = lhs.m_idle_jiffies - rhs.m_idle_jiffies; // total idle over all processors

    // calculation is jiffies * (1  second / CLK_TCK jiffies) * (1000 ms / 1 second) 
    // integer multiplication shouldn't overflow since the difference between samples 
    // should be small.
    const auto idleDurationTotal = (idleJiffies * 1000.0 /*ms per sec*/) / CLK_TCK;
    const auto idleDurationAvg = idleDurationTotal / NPROC; // average over all processors

    CPUSampleDelta diff;
    diff.m_idle_duration = CPUSampleDelta::FloatMilliseconds{idleDurationAvg};
    diff.m_duration = std::chrono::duration_cast<std::chrono::milliseconds>(lhs.m_sample_time - rhs.m_sample_time);
    return diff;
}

CPUSample::CPUSample()
: m_sample_time{std::chrono::steady_clock::now()}
{
}

std::ostream & operator <<(std::ostream & os, const CPUSampleDelta & sampleDelta)
{
    if (sampleDelta.m_duration.count() == 0)
    {
        // some sampling error occurred.  Dont divide by zero.
        os << "CPU usage err%\n";
        return os;
    }

    const auto not_idle = sampleDelta.m_duration - sampleDelta.m_idle_duration;
    
    os << "CPU usage " << (not_idle.count() * 100.0 / sampleDelta.m_duration.count()) << "%";
    return os;
}

}