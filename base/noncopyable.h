#ifndef CUBER_BASE_NONCOPYABLE_H
#define CUBER_BASE_NONCOPYABLE_H

namespace cuber
{

class noncopyable
{
 public:
  noncopyable(const noncopyable&) = delete;
  void operator=(const noncopyable&) = delete;

 protected:
  noncopyable() = default;
  ~noncopyable() = default;
};

}  // namespace cuber

#endif  // CUBER_BASE_NONCOPYABLE_H
