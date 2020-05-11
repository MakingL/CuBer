#ifndef CUBER_HTTPTIMER_H
#define CUBER_HTTPTIMER_H

#include <list>
#include <unordered_map>
#include "net/TcpConnection.h"

namespace cuber
{
namespace net
{

class HttpRequest;
class HttpResponse;

typedef std::weak_ptr<TcpConnection> WeakTcpConnectionPtr;
typedef std::list<WeakTcpConnectionPtr> WeakConnectionList;
typedef WeakConnectionList::iterator HttpTimerPos;

class HttpTimer : noncopyable
{
public:
    typedef std::function<void(const TcpConnectionPtr &)> HttpTimeoutCallback;

    HttpTimer(EventLoop *loop, int expireSeconds, float tickSeconds = 1.0);

    void setHttpTimeoutCallback(const HttpTimeoutCallback &cb)
    {
        httpTimeoutCallback_ = cb;
    }

    HttpTimerPos addHttpConnection(const TcpConnectionPtr &conn)
    {
        connectionList_.push_back(conn);
        return --connectionList_.end();
    }

    void removeHttpConnection(const HttpTimerPos &timerPos)
    {
        connectionList_.erase(timerPos);
    }

private:
    void defaultTimeoutCallback(const TcpConnectionPtr &conn);
    void onTimer();

    void dumpConnectionList() const;

private:
    typedef TcpConnection *ConnectionPtr;

    WeakConnectionList connectionList_;
    int expireSeconds_;
    float tickSeconds_;
    HttpTimeoutCallback httpTimeoutCallback_;
};
} // namespace net
} // namespace cuber

#endif //CUBER_HTTPTIMER_H
