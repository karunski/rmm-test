#pragma once
#ifndef RMM_MEM_AVAILABLE_H
#define RMM_MEM_AVAILABLE_H

#include <string>
#include <iosfwd>

namespace rmm
{
    
struct MemAvailable
{
    std::string m_memAvailable;
};

std::ostream & operator<<(std::ostream & os, const MemAvailable & memAvailable);

// Function to get the available memory in bytes
MemAvailable GetAvailableMemoryReport();
}

#endif