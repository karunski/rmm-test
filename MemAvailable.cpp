#include "MemAvailable.h"

#include <system_error>
#include <fstream>

namespace rmm
{
MemAvailable GetAvailableMemoryReport()
{
    // Open the /proc/meminfo file
    std::ifstream meminfoFile("/proc/meminfo");
    if (!meminfoFile)
    {
        throw std::system_error{errno, std::system_category(), "Failed to open /proc/meminfo"};
    }

    MemAvailable linebuffer;
    while (std::getline(meminfoFile, linebuffer.m_memAvailable))
    {
        // Just fetch the MemAvailable line.  It's a pretty reasonable summary of available memory
        if (linebuffer.m_memAvailable.find("MemAvailable:") == 0)
        {
            return linebuffer;
        }
    }

    throw std::runtime_error{"This kernel is too old to report the MemAvailable metric"};
}

std::ostream & operator<<(std::ostream & os, const MemAvailable & memAvailable)
{
    return os << memAvailable.m_memAvailable;
}

}