#pragma once
#ifndef RMM_SCRIPT_RUNNER_H
#define RMM_SCRIPT_RUNNER_H

#include <boost/asio/io_context.hpp>
#include <filesystem>
#include <functional>

namespace rmm
{

class ScriptRunner
{
public:
    ScriptRunner(boost::asio::io_context & ioc, std::filesystem::path scriptOutputPath);
    ~ScriptRunner();

    using ScriptCompletedHandler = std::function<void(const std::filesystem::path & script, int exit_code, const boost::system::error_code & ec)>;
    void connectCompletionSlot(ScriptCompletedHandler);

    void runScriptAsync(const std::filesystem::path & scriptPath);
private:

    void completeScript(const std::filesystem::path & script, int exit_code, const boost::system::error_code & ec);
    
    boost::asio::io_context & m_ioc;
    const std::filesystem::path m_scriptOutputPath;
    ScriptCompletedHandler m_scriptCompletedSignal;
};

}

#endif