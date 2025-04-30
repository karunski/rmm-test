#include "PollingTimer.h"
#include "CPUSampler.h"
#include "MemAvailable.h"

#include <iostream>
#include <boost/asio/io_context.hpp>
#include <chrono>
#include <filesystem>
#include <system_error>
#include <fstream>

using namespace std::literals::chrono_literals;


int main()
try
{
    std::cout << "Starting RMM...\n";

    const auto binaryPath = std::filesystem::read_symlink("/proc/self/exe");
    const auto statsDir = binaryPath.parent_path().parent_path() / "var" / "stats";

    std::cout << "Stats directory: " << statsDir << "\n";
    std::filesystem::create_directories(statsDir);

    std::ofstream cpu_stats_out{statsDir / "cpu_stats.txt", std::fstream::app};
    if (!cpu_stats_out)
    {
        throw std::system_error{errno, std::system_category(), "Failed to open cpu_stats.txt"};
    }
    std::ofstream mem_stats_out{statsDir / "mem_stats.txt", std::fstream::app};
    if (!mem_stats_out)
    {
        throw std::system_error{errno, std::system_category(), "Failed to open mem_stats.txt"};
    }

    // A boost asio io_context is used as the main event loop.
    boost::asio::io_context ioContext;

    // System stats are collected periodically.
    rmm::PollingTimer sysStatsTimer{ioContext};

    auto lastCPUSample = rmm::GetCPUSample();

    // A 2 second polling loop seems reasonable for CPU and memory stats.
    sysStatsTimer.launchAsyncPollingLoop(2s, [&lastCPUSample, &cpu_stats_out, &mem_stats_out](const auto & ec)
    {
        if (ec) { return;} // some kind of error occurred, just return
        
        const auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
        const auto localtm = std::localtime(&now); // single-threaded application... localtime doesn't need to be thread safe

        const auto currentCPUSample = rmm::GetCPUSample();
        cpu_stats_out << std::put_time(localtm, "[%Y-%m-%d %H:%M:%S %z] ") 
            << (currentCPUSample - lastCPUSample) << "\n";
        cpu_stats_out.flush(); // force line to be written immediately
        lastCPUSample = currentCPUSample;

        // mem stats are stateless... just report the current value
        mem_stats_out << std::put_time(localtm, "[%Y-%m-%d %H:%M:%S %z] ")
            << rmm::GetAvailableMemoryReport() << "\n";
        mem_stats_out.flush(); // force line to be written immediately
    });

    // Wait for events (timer expiring, inotify events, etc.)
    ioContext.run();
    return EXIT_SUCCESS;
}
catch (const std::exception & ex) // Top-level exception backstop
{
    std::cerr << "Exception: " << ex.what() << std::endl;
    return EXIT_FAILURE;
}
catch (...)
{
    std::cerr << "Unknown exception" << std::endl;
    return EXIT_FAILURE;
}