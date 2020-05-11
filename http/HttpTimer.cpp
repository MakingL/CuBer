#include "base/Logging.h"
#include "net/EventLoop.h"
#include "http/HttpTimer.h"

using namespace cuber;
using namespace cuber::net;

HttpTimer::HttpTimer(EventLoop *loop, int expireSeconds, float tickSeconds)
    : expireSeconds_(expireSeconds), tickSeconds_(tickSeconds)
{
    httpTimeoutCallback_ = std::bind(&HttpTimer::defaultTimeoutCallback, this, _1);
    loop->runEvery(tickSeconds_, std::bind(&HttpTimer::onTimer, this));
}

void HttpTimer::defaultTimeoutCallback(const TcpConnectionPtr &conn)
{
    LOG_DEBUG << conn->name() << " timeout. Shutting down.";
    conn->shutdown();
    conn->forceCloseWithDelay(3.5); // > round trip of the whole Internet.
}

void HttpTimer::onTimer()
{
    Timestamp now = Timestamp::now();
    for (WeakConnectionList::iterator it = connectionList_.begin();
         it != connectionList_.end();)
    {

        TcpConnectionPtr conn = it->lock();
        if (conn)
        {
            double age = timeDifference(now, conn->lastReceiveTime());
            if (age > expireSeconds_)
            {
                if (conn->connected())
                {
                    httpTimeoutCallback_(static_cast<TcpConnectionPtr>(conn));
                }
            }
            else if (age < 0)
            {
                LOG_DEBUG << "Time jump for conn: " << conn->name();
                conn->updateLastReceiveTime(now);
            }
            else
            {
                break;
            }
            ++it;
        }
        else
        {
            LOG_DEBUG << "remove closed conn";
            it = connectionList_.erase(it);
        }
    }
}

void HttpTimer::dumpConnectionList() const
{ // Timer DEBUG Helper
    LOG_DEBUG << "Timer connection list size: " << connectionList_.size();
    if (!connectionList_.empty())
    {
        Timestamp now = Timestamp::now();
        printf("\tTimestamp now %s\n", now.toString().c_str());
    }
    for (WeakConnectionList::const_iterator it = connectionList_.begin();
         it != connectionList_.end(); ++it)
    {
        TcpConnectionPtr conn = it->lock();
        if (conn)
        {
            printf("\t conn: %s", conn->name().c_str());
            printf("\t last_receive_time: %s\n", conn->lastReceiveTime().toString().c_str());
        }
        else
        {
            printf("a expired conn\n");
        }
    }
}
