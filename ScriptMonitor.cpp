#include "ScriptMonitor.h"

#include <sys/inotify.h>
#include <system_error>
#include <boost/asio/read.hpp>
#include <boost/asio/buffer.hpp>

namespace rmm
{

ScriptDropboxMonitor::ScriptDropboxMonitor(boost::asio::io_context & ioc, std::filesystem::path dropboxPath)
: m_dropboxPath{std::move(dropboxPath)}, m_dropboxInotifyFd{ioc}
{
    const auto dropboxFd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC);
    if (dropboxFd < 0)
    {
        throw std::system_error{errno, std::system_category(), "Failed to open inotify"};
    }

    // Just watch for files renamed into the dropbox directory.  
    // My thinking here is that the "drop" operation would be defined as a copy+rename
    // sequence.... potentially via some other "drop" sub-command implmented as a mode of this program
    const auto watchDescriptor = inotify_add_watch(dropboxFd, m_dropboxPath.c_str(), IN_MOVED_TO);
    if (watchDescriptor < 0)
    {
        throw std::system_error{errno, std::system_category(), "Failed to add watch"};
    }

    m_dropboxInotifyFd.assign(dropboxFd);
    m_watchDescriptor = watchDescriptor;
} 

ScriptDropboxMonitor::~ScriptDropboxMonitor() = default;

void ScriptDropboxMonitor::asyncWaitForScriptAdded(ScriptAddedHandler scriptAddedHandler)
{
    if (m_scriptAddedSignal)
    {
        throw std::runtime_error{"Already waiting for a script to be added"};
    }

    m_scriptAddedSignal = std::move(scriptAddedHandler);

    initAsyncWait();
}

void ScriptDropboxMonitor::initAsyncWait()
{
    // poll for a readability event.  Actual reading is done in the handler
    m_dropboxInotifyFd.async_wait(boost::asio::posix::stream_descriptor::wait_read,
        [this](const boost::system::error_code & ec)
        {
            if (ec)
            {
                // forward the error to the handler.
                completeAsyncWait({}, ec);
                
                // reset the wait handler... could re-use this object?
                m_scriptAddedSignal = {};
                return;
            }

            // Read the inotify event; this won't block because we've been notified that it is readable
            m_eventBuffer.resize(sizeof(struct inotify_event) + NAME_MAX + 1); // see man inotify(7)
            if (::read(m_dropboxInotifyFd.native_handle(), m_eventBuffer.data(), m_eventBuffer.size()) < 0)
            {
                completeAsyncWait({}, boost::system::error_code{errno, boost::system::system_category()});
                m_scriptAddedSignal = {};
                return;
            }

            const auto current_event = reinterpret_cast<const ::inotify_event *>(m_eventBuffer.data());
            if (current_event->mask & IN_MOVED_TO)
            {
                // The event is a file being moved into (or renamed inside) the dropbox directory
                std::filesystem::path scriptPath{m_dropboxPath / std::string{current_event->name, current_event->len}};
                completeAsyncWait(scriptPath, {});
            }
            else
            {
                // Some other event occurred.  Just ignore it.
            }
            initAsyncWait(); // initiate the next event wait
        }
    );
}

void ScriptDropboxMonitor::completeAsyncWait(const std::filesystem::path & script, const boost::system::error_code & ec)
{
    if (m_scriptAddedSignal)
    {
        m_scriptAddedSignal(script, ec);
    }
}

} // namespace rmm
