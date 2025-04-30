#pragma once
#ifndef RMM_POLLING_TIMER_H
#define RMM_POLLING_TIMER_H

#include <boost/asio/steady_timer.hpp>
#include <functional>
#include <chrono>

namespace rmm
{

class PollingTimer
{
public:
    explicit PollingTimer(boost::asio::io_context & ioc);
    ~PollingTimer();

    using PollFunction = std::function<void(const boost::system::error_code & ec)>;

    void launchAsyncPollingLoop(std::chrono::seconds pollingPeriod, PollFunction pollFunc);

    void cancel();

private:
    void initOneCycle(std::chrono::seconds pollingPeriod);

    void completeOneCycle(const boost::system::error_code & ec);

    void reset();

    boost::asio::steady_timer m_timer;
    PollFunction m_pollSignal;
};

}

#endif