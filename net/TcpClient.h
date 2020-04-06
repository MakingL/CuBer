// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef CUBER_NET_TCPCLIENT_H
#define CUBER_NET_TCPCLIENT_H

#include "base/Mutex.h"
#include "net/TcpConnection.h"

namespace cuber
{
namespace net
{

class Connector;
typedef std::shared_ptr<Connector> ConnectorPtr;

class TcpClient : noncopyable
{
 public:
  // TcpClient(EventLoop* loop);
  // TcpClient(EventLoop* loop, const string& host, uint16_t port);
  TcpClient(EventLoop* loop,
            const InetAddress& serverAddr,
            const string& nameArg,
            int maxRetry=3);
  ~TcpClient();  // force out-line dtor, for std::unique_ptr members.

  void connect();
  void disconnect();
  void stop();

  TcpConnectionPtr connection() const
  {
    MutexLockGuard lock(mutex_);
    return connection_;
  }

  EventLoop* getLoop() const { return loop_; }

  int maxRetryTimes() const { return retryTimes_; }

  void setMaxRetryTimes(int retryTime);

  bool retry() const { return retry_; }
  void enableRetry() { retry_ = true; }
  const string &name() const { return name_; }

  void setCloseCallback(const CloseCallback &cb)
  { connectionCloseCallback_ = cb; }

  /// Set connection callback.
  /// Not thread safe.
  void setConnectionCallback(ConnectionCallback cb)
  { connectionCallback_ = std::move(cb); }

  /// Set message callback.
  /// Not thread safe.
  void setMessageCallback(MessageCallback cb)
  { messageCallback_ = std::move(cb); }

  /// Set write complete callback.
  /// Not thread safe.
  void setWriteCompleteCallback(WriteCompleteCallback cb)
  { writeCompleteCallback_ = std::move(cb); }

  typedef std::function<void (const std::string& clientName, const InetAddress& peerAddr)> TCPClientConnectFailCallback;

  void setConnectFailCallback(TCPClientConnectFailCallback cb)
  { connectFailCallback_ = std::move(cb); }

  void defaultTCPClientConnectFailCallback(const std::string &clientName, const InetAddress &peerAddr);
 private:
  /// Not thread safe, but in loop
  void newConnection(int sockfd);
  /// Not thread safe, but in loop
  void removeConnection(const TcpConnectionPtr& conn);

  void onConnectFailed(const InetAddress & peerAddr) {
      connectFailCallback_(name_, peerAddr);
  }

  EventLoop* loop_;
  ConnectorPtr connector_; // avoid revealing Connector
  const string name_;
  ConnectionCallback connectionCallback_;
  MessageCallback messageCallback_;
  WriteCompleteCallback writeCompleteCallback_;
  CloseCallback connectionCloseCallback_;
  TCPClientConnectFailCallback connectFailCallback_;
  int retryTimes_;
  bool retry_;   // atomic
  bool connect_; // atomic
  // always in loop thread
  int nextConnId_;
  mutable MutexLock mutex_;
  TcpConnectionPtr connection_ GUARDED_BY(mutex_);
};

}  // namespace net
}  // namespace cuber

#endif  // CUBER_NET_TCPCLIENT_H
