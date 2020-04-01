#ifndef CUBER_BASE_COPYABLE_H
#define CUBER_BASE_COPYABLE_H

namespace cuber
{

/// A tag class emphasises the objects are copyable.
/// The empty base class optimization applies.
/// Any derived class of copyable should be a value type.
class copyable
{
 protected:
  copyable() = default;
  ~copyable() = default;
};

}  // namespace cuber

#endif  // CUBER_BASE_COPYABLE_H
