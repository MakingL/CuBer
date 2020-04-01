// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)

#ifndef CUBER_BASE_COUNTDOWNLATCH_H
#define CUBER_BASE_COUNTDOWNLATCH_H

#include "base/Condition.h"
#include "base/Mutex.h"

namespace cuber
{

class CountDownLatch : noncopyable
{
 public:

  explicit CountDownLatch(int count);

  void wait();

  void countDown();

  int getCount() const;

 private:
  mutable MutexLock mutex_;
  Condition condition_ GUARDED_BY(mutex_);
  int count_ GUARDED_BY(mutex_);
};

}  // namespace cuber
#endif  // CUBER_BASE_COUNTDOWNLATCH_H