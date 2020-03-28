#ifndef CIRCULAR_BUFFER_H_
#define CIRCULAR_BUFFER_H_
#include <algorithm>
#include <stdint.h>

template <typename T, size_t S, bool RollOver=false>
class CircularBuffer {
  static_assert(std::is_pod<T>::value, "T must be POD");

 public:
  CircularBuffer() : buf{0}, bottom(0), top(0) {}
  ~CircularBuffer() = default;

  constexpr bool empty() noexcept { return top == bottom; }

  constexpr bool full() noexcept { return (top) == incrementIndex(bottom); }

  static constexpr size_t max_size(void) noexcept { return S; }

  size_t count(void) { 
    return bottom - top + (bottom < top ? (S+1) : 0) ;
  }

  /**
   * @brief Push the given value to the buffer
   * @param value The value
   */
  bool push(T const &value) noexcept {
    if (full()) {
      if (RollOver) {
        top = incrementIndex(top);
      } else {
        return false;
      }
    }

    buf[bottom] = value;
    bottom = incrementIndex(bottom);
    return true;
  }

  /**
   * @brief Push a spot for memory in the buffer, but don't copy just return a
   * pointer.
   * @return Returns nullptr if full. Otherwise allocates some memory.
   */
  T *alloc() noexcept {
    T *alloc_ptr = nullptr;
    if (!full()) {
      alloc_ptr = buf + bottom;         // Calculate the pointer
      bottom = incrementIndex(bottom);  // Increment the circular buffer
    }

    return alloc_ptr;
  }

  /**
   * @brief Returns a pointer to the value at the top of the buffer
   */
  inline T *peek(uint8_t n = 0) noexcept {
    return empty() ? nullptr : &buf[incrementIndexByN(top, n)];
  }

  /**
   * @brief Returns the value at the top of the buffer
   */
  bool pop(T *const t) noexcept {
    if (empty() || t == nullptr) {
      return false;
    }

    // Copy data out of the buffer first before doing anything else,
    // this should make the buffer reentrant.
    T tmp = buf[top];
    // Increment the tops index now freeing the resource.
    top = incrementIndex(top);
    *t = tmp;

    return true;
  }

  /**
   * @brief deallocate the slot at the top of the buffer. This is a pop without
   * copy.
   */
  bool free() noexcept {
    if (empty()) {
      return false;
    }

    // Increment the tops index now freeing the resource.
    top = incrementIndex(top);

    return true;
  }

 private:
  T buf[S + 1];
  volatile size_t bottom;
  volatile size_t top;

  static constexpr size_t incrementIndex(size_t idx) noexcept {
    return (idx == S) ? 0 : (idx + 1);
  }

  static constexpr size_t incrementIndexByN(size_t idx, size_t n) noexcept {
    return (idx + n) % (S + 1);
  }
};

#endif  // CIRCULAR_BUFFER_H)_
