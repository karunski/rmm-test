#pragma once
#ifndef RMM_SCRIPT_MONITOR_H
#define RMM_SCRIPT_MONITOR_H

#include <boost/asio/io_context.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <filesystem>
#include <functional>
#include <vector>

namespace rmm
{

class ScriptDropboxMonitor
{
public:
    ScriptDropboxMonitor(boost::asio::io_context & ioc, std::filesystem::path dropboxPath);
    ~ScriptDropboxMonitor();

    using ScriptAddedHandler = std::function<void(const std::filesystem::path & script, const boost::system::error_code & ec)>;
    void asyncWaitForScriptAdded(ScriptAddedHandler);

    //todo: cancel async ops:
    //void cancel();
private:

    void initAsyncWait();
    void completeAsyncWait(const std::filesystem::path & script, const boost::system::error_code & ec);

    boost::asio::posix::stream_descriptor m_dropboxInotifyFd;
    std::filesystem::path m_dropboxPath;
    std::function<void(const std::filesystem::path & script, const boost::system::error_code & ec)> m_scriptAddedSignal;
    std::vector<char> m_eventBuffer;
    int m_watchDescriptor = -1;
};

}

#endif