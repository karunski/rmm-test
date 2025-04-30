#include "PollingTimer.h"
#include <iostream>

namespace rmm
{
PollingTimer::PollingTimer(boost::asio::io_context &ioc)
: m_timer{ioc}
{
}

PollingTimer::~PollingTimer() = default;

void PollingTimer::launchAsyncPollingLoop(std::chrono::seconds pollingPeriod, PollFunction pollFunc)
{
    if (m_pollSignal) { throw std::runtime_error{"This poller is already in use."}; }

    initOneCycle(pollingPeriod);

    m_pollSignal = std::move(pollFunc);
}

void PollingTimer::initOneCycle(std::chrono::seconds pollingPeriod)
{
    m_timer.expires_after(pollingPeriod);

    // note-- this function is guaranteed that it will not call the provided completion
    // token lambda function within the call to async_wait; this ensures it doesn't enter infinite recursion
    // when a successful poll results in another call to this function.
    m_timer.async_wait([this, pollingPeriod](const boost::system::error_code & ec)
    {
        if (ec == boost::asio::error::operation_aborted)
        {
            // normal cancelation;
            completeOneCycle(ec); // forward the error condition to the outer operation
            reset();  // don't init another wait cycle.  
            return;
        }

        if (ec)
        {
            // any other error
            std::cerr << "Unexpected error during PollingTimer wait: " << ec.message() << "\n";
            reset();
            return;
        }

        // normal timer expiration
        completeOneCycle(ec);
        initOneCycle(pollingPeriod);
    });
}

void PollingTimer::reset()
{
    m_pollSignal = PollFunction{};
}

void PollingTimer::completeOneCycle(const boost::system::error_code & ec)
{
    if (m_pollSignal)
    {
        m_pollSignal(ec);
    }
}

}