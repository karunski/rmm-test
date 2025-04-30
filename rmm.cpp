#include "PollingTimer.h"
#include "CPUSampler.h"

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

    // A boost asio io_context is used as the main event loop.
    boost::asio::io_context ioContext;

    // System stats are collected periodically.
    rmm::PollingTimer sysStatsTimer{ioContext};

    auto lastCPUSample = rmm::GetCPUSample();

    // A 2 second polling loop seems reasonable for CPU and memory stats.
    sysStatsTimer.launchAsyncPollingLoop(2s, [&lastCPUSample, &cpu_stats_out](const auto & ec)
    {
        if (ec) { return;} // some kind of error occurred, just return
        
        const auto currentCPUSample = rmm::GetCPUSample();
        cpu_stats_out << (currentCPUSample - lastCPUSample) << "\n";
        cpu_stats_out.flush(); // force line to be written immediately
        lastCPUSample = currentCPUSample;
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