#ifndef _UTIL_BASE_ATOMIC__
#define  _UTIL_BASE_ATOMIC__

#include "util/base/port.h"

namespace util {
template<typename T>
inline void ignore_result(const T& ignored) {
}

// Atomically execute:
//      result = *ptr;
//      if (*ptr == old_value)
//        *ptr = new_value;
//      return result;
//
// I.e., replace "*ptr" with "new_value" if "*ptr" used to be "old_value".
// Always return the old value of "*ptr"
template<typename T>
T AtomicCompareAndSwap(volatile T* ptr,
                       T old_value,
                       T new_value) {
  return __sync_val_compare_and_swap(ptr, old_value, new_value);
}

// Atomically assign a new value to the pointer.
template<typename T>
void AtomicPointerAssgin(T* ptr, T new_value) {
  ignore_result(__sync_lock_test_and_set(ptr, new_value));
}

// Atomically increment *ptr by "increment".  Returns the new value of
// *ptr with the increment applied.  This routine implies no memory barriers.
template<typename T>
T AtomicIncrement(volatile T* ptr, T increment) {
  return __sync_add_and_fetch(ptr, increment);
}

// Atomically increment *ptr by 1.
template<typename T>
inline T AtomicIncrement(volatile T* ptr) {
  return AtomicIncrement(ptr, static_cast<T>(1));
}

template<typename T>
T AtomicDecrement(volatile T* ptr, T decrement) {
  return __sync_sub_and_fetch(ptr, decrement);
}

template<typename T>
inline T AtomicDecrement(volatile T* ptr) {
  return AtomicDecrement(ptr, static_cast<T>(1));
}

}  // namespace util 

#endif  // _UTIL_BASE_ATOMIC__
