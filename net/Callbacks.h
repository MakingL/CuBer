// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef CUBER_NET_CALLBACKS_H
#define CUBER_NET_CALLBACKS_H

#include "base/Timestamp.h"

#include <functional>
#include <memory>

namespace cuber
{

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

// should really belong to base/Types.h, but <memory> is not included there.

template<typename T>
inline T* get_pointer(const std::shared_ptr<T>& ptr)
{
  return ptr.get();
}

template<typename T>
inline T* get_pointer(const std::unique_ptr<T>& ptr)
{
  return ptr.get();
}

// Adapted from google-protobuf stubs/common.h
// see License in base/Types.h
template<typename To, typename From>
inline ::std::shared_ptr<To> down_pointer_cast(const ::std::shared_ptr<From>& f) {
  if (false)
  {
    implicit_cast<From*, To*>(0);
  }

#ifndef NDEBUG
  assert(f == NULL || dynamic_cast<To*>(get_pointer(f)) != NULL);
#endif
  return ::std::static_pointer_cast<To>(f);
}

namespace net
{

// All client visible callbacks go here.

class Buffer;
class TcpConnection;
class InetAddress;
typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;
typedef std::function<void()> TimerCallback;
typedef std::function<void (const TcpConnectionPtr&)> ConnectionCallback;
typedef std::function<void (const InetAddress&)> ConnectFailCallback;
typedef std::function<void (const std::string& clientName, const InetAddress& peerAddr)> TCPClientConnectFailCallback;
typedef std::function<void (const TcpConnectionPtr&)> CloseCallback;
typedef std::function<void (const TcpConnectionPtr&)> WriteCompleteCallback;
typedef std::function<void (const TcpConnectionPtr&, size_t)> HighWaterMarkCallback;

// the data has been read to (buf, len)
typedef std::function<void (const TcpConnectionPtr&,
                            Buffer*,
                            Timestamp)> MessageCallback;

void defaultConnectionCallback(const TcpConnectionPtr& conn);
void defaultConnectFailCallback(const InetAddress& serverAddr);
void defaultTCPClientConnectFailCallback(const std::string &clientName, const InetAddress &peerAddr);
void defaultMessageCallback(const TcpConnectionPtr& conn,
                            Buffer* buffer,
                            Timestamp receiveTime);

}  // namespace net
}  // namespace cuber

#endif  // CUBER_NET_CALLBACKS_H
