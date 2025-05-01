#include "ScriptRunner.h"

#include <boost/process/v2/process.hpp>
#include <boost/process/v2/error.hpp>
#include <boost/process/v2/stdio.hpp>
#include <boost/process/v2/execute.hpp>
#include <boost/process/v2/default_launcher.hpp>

namespace rmm
{

ScriptRunner::~ScriptRunner() = default;

ScriptRunner::ScriptRunner(boost::asio::io_context & ioc, std::filesystem::path scriptOutputPath)
    : m_ioc{ioc}, m_scriptOutputPath{std::move(scriptOutputPath)}
{
}

void ScriptRunner::connectCompletionSlot(ScriptCompletedHandler handler)
{
    m_scriptCompletedSignal = std::move(handler);
}

void ScriptRunner::runScriptAsync(const std::filesystem::path &scriptPath)
{
    // TODO-- assign stdout and stderr into the same handle
    const auto scriptFilename = scriptPath.filename().string();
    const auto scriptOutputFile = m_scriptOutputPath / (scriptFilename + ".out");
    const auto scriptErrorFile = m_scriptOutputPath / (scriptFilename + ".err");

    boost::system::error_code ec;
    auto new_proc = boost::process::v2::default_process_launcher{}(m_ioc, ec, scriptPath,
        std::vector<std::string>{},
        boost::process::v2::process_stdio{nullptr, scriptOutputFile, scriptErrorFile});
    if (ec)
    {
        completeScript(scriptPath, -1, ec);
        return;
    }

    boost::process::v2::async_execute(std::move(new_proc),
        [this, scriptPath](const boost::system::error_code & ec, int exit_code)
        {
            completeScript(scriptPath, exit_code, ec);
        }
    );
}

void ScriptRunner::completeScript(const std::filesystem::path &script, int exit_code, const boost::system::error_code &ec)
{
    if (m_scriptCompletedSignal)
    {
        m_scriptCompletedSignal(script, exit_code, ec);
    }
}

}