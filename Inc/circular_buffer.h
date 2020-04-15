#ifndef CIRCULAR_BUFFER_H_
#define CIRCULAR_BUFFER_H_
#include <stdint.h>

#include <algorithm>

template <typename T, size_t S, bool IsWrapable = false>
class CircularBuffer {
    static_assert(std::is_pod<T>::value, "T must be POD");

  public:
    CircularBuffer() : buf{0}, front(0), back(0) {}
    ~CircularBuffer() = default;

    constexpr bool empty() noexcept {
        return back == front;
    }

    constexpr bool full() noexcept {
        return (back) == incrementIndex(front);
    }

    static constexpr size_t max_size(void) noexcept {
        return S;
    }

    size_t count(void) {
        return front - back + (front < back ? (S + 1) : 0);
    }

    /**
     * @brief Push the given value to the buffer
     * @param value The value
     */
    bool push(T const &value) noexcept {
        if (full()) {
            if (IsWrapable) {
                free();
            } else {
                return false;
            }
        }

        buf[front] = value;
        front = incrementIndex(front);
        return true;
    }

    /**
     * @brief Push a spot for memory in the buffer, but don't copy just return a
     * pointer.
     * @return Returns nullptr if full. Otherwise allocates some memory.
     */
    T *alloc() noexcept {
        // If we are full and wrapable free the back of the buffer so we can
        // wrap around.
        if (full() && IsWrapable) {
            free();
        }

        T *alloc_ptr = nullptr;
        if (!full()) {
            alloc_ptr = buf + front;        // Calculate the pointer
            front = incrementIndex(front);  // Increment the circular buffer
        }

        return alloc_ptr;
    }

    /**
     * @brief Returns a pointer to the value at the back of the buffer or n
     * counts from it
     */
    inline T *peek(uint8_t n = 0) noexcept {
        return empty() ? nullptr : &buf[incrementIndexByN(back, n)];
    }

    /**
     * @brief Returns the value at the back of the buffer
     */
    bool pop(T *const t) noexcept {
        if (empty() || t == nullptr) {
            return false;
        }

        // Copy data out of the buffer first before doing anything else,
        // this should make the buffer reentrant.
        T tmp = buf[back];
        // Increment the tops index now freeing the resource.
        back = incrementIndex(back);
        *t = tmp;

        return true;
    }

    /**
     * @brief deallocate the slot at the back of the buffer. This is a pop
     * without copy.
     */
    bool free() noexcept {
        if (empty()) {
            return false;
        }

        // Increment the back index now freeing the resource.
        back = incrementIndex(back);

        return true;
    }

  private:
    T buf[S + 1];
    volatile size_t front;
    volatile size_t back;

    static constexpr size_t incrementIndex(size_t idx) noexcept {
        return (idx == S) ? 0 : (idx + 1);
    }

    static constexpr size_t incrementIndexByN(size_t idx, size_t n) noexcept {
        return (idx + n) % (S + 1);
    }
};

#endif  // CIRCULAR_BUFFER_H)_
