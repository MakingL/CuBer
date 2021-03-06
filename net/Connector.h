// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is an internal header file, you should not include this.

#ifndef CUBER_NET_CONNECTOR_H
#define CUBER_NET_CONNECTOR_H

#include "base/noncopyable.h"
#include "net/InetAddress.h"

#include <functional>
#include <memory>

namespace cuber
{
namespace net
{

class Channel;
class EventLoop;

class Connector : noncopyable,
                  public std::enable_shared_from_this<Connector>
{
 public:
  typedef std::function<void (int sockfd)> NewConnectionCallback;
  typedef std::function<void(const InetAddress& serverAddr)> ConnectFailCallback;

  Connector(EventLoop* loop, const InetAddress& serverAddr);
  ~Connector();

  void setNewConnectionCallback(const NewConnectionCallback& cb)
  { newConnectionCallback_ = cb; }

  void setConnectFailCallback(const ConnectFailCallback& cb)
  { connectFailCallback_ = cb; }

  void setMaxRetry(int retryTime)
  { maxRetry_ = retryTime; }

  int maxRetry() const { return maxRetry_; }

  void start();  // can be called in any thread
  void restart();  // must be called in loop thread
  void stop();  // can be called in any thread

  const InetAddress& serverAddress() const { return serverAddr_; }

 private:
  enum States { kDisconnected, kConnecting, kConnected };
  static const int kMaxRetryDelayMs = 30*1000;
  static const int kInitRetryDelayMs = 500;

  void setState(States s) { state_ = s; }
  void startInLoop();
  void stopInLoop();
  void connect();
  void connecting(int sockfd);
  void handleWrite();
  void handleError();
  void retry(int sockfd);
  int removeAndResetChannel();
  void resetChannel();

  EventLoop* loop_;
  InetAddress serverAddr_;
  bool connect_; // atomic
  States state_;  // FIXME: use atomic variable
  std::unique_ptr<Channel> channel_;
  ConnectFailCallback connectFailCallback_;
  NewConnectionCallback newConnectionCallback_;
  int retryDelayMs_;
  int maxRetry_;
};

}  // namespace net
}  // namespace cuber

#endif  // CUBER_NET_CONNECTOR_H
