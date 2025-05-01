#include "PollingTimer.h"
#include "CPUSampler.h"
#include "MemAvailable.h"
#include "ScriptMonitor.h"
#include "ScriptRunner.h"

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

    const auto scriptDir = binaryPath.parent_path().parent_path() / "var" / "scripts";
    const auto scriptOutputDir = binaryPath.parent_path().parent_path() / "var" / "script_output";
    std::filesystem::create_directories(scriptDir);
    std::filesystem::create_directories(scriptOutputDir);
    std::cout << "Script directory: " << scriptDir << "\n";
    std::cout << "Script output directory: " << scriptOutputDir << "\n";

    rmm::ScriptDropboxMonitor scriptMonitor{ioContext, scriptDir};
    rmm::ScriptRunner scriptRunner{ioContext, scriptOutputDir};

    // Define what to do when a script is added to the dropbox
    scriptMonitor.asyncWaitForScriptAdded([&scriptRunner](const auto & script, const auto & ec)
    {
        if (ec)
        {
            std::cerr << "Error waiting for script: " << ec.message() << "\n";
            return;
        }

        scriptRunner.runScriptAsync(script); // run it.
    });

    // Define what to do when a script completes
    scriptRunner.connectCompletionSlot([](const auto & script, int exit_code, const auto & ec)
    {
        if (ec)
        {
            std::cerr << "Error running script " << script << ": " << ec.message() << "\n";
            return;
        }

        std::cout << "Script completed: " << script << ", exit code: " << exit_code << "\n";
    });

    // Enter the event loop and wait for events (timer expiring, inotify events, process completions)
    ioContext.run();
    return EXIT_SUCCESS;
}
catch (const std::exception & ex) // Top-level exception backstop
{
    std::cerr << "Unhandled exception: " << ex.what() << "\n";
    return EXIT_FAILURE;
}
catch (...) // catch-all... maybe catch SEH here on windows...
{
    std::cerr << "Unhandled unknown exception\n";
    return EXIT_FAILURE;
}