#ifndef IMP_CIRCULAR_RW_BUFFER_HPP
#define IMP_CIRCULAR_RW_BUFFER_HPP

#include <utility>

class CircularRWBufferBase {
public:
  enum ReadError { BufferEmpty };

  enum WriteError { BufferFull };

  enum State { Empty, Normal, Full };
};

template <typename T>
class CircularRWBuffer : CircularRWBufferBase {
public:
  template <typename... TArgs>
  void write(TArgs... datas);

  T read();

  State getState();

private:
  static constexpr size_t BUFSIZE = 128;

  void write_impl(T data);

  T buffer[BUFSIZE] = {0};
  size_t r_ptr_offset = 0;
  size_t w_ptr_offset = 0;
  State state = Empty;
};

template <typename T>
template <typename... TArgs>
void CircularRWBuffer<T>::write(TArgs... datas)
{
  (write_impl(std::forward<TArgs>(datas)), ...);
}

template <typename T>
T CircularRWBuffer<T>::read()
{
  if (state == Empty) {
    throw BufferEmpty;
  }

  T result = *(buffer + r_ptr_offset);

  if (++r_ptr_offset >= BUFSIZE) {
    r_ptr_offset = 0;
  }

  if (r_ptr_offset == w_ptr_offset) {
    state = Empty;
  }
  else {
    state = Normal;
  }

  return result;
}

template <typename T>
CircularRWBufferBase::State CircularRWBuffer<T>::getState()
{
  return state;
}

template <typename T>
void CircularRWBuffer<T>::write_impl(T data)
{
  if (state == Full) {
    throw BufferFull;
  }

  *(buffer + w_ptr_offset) = data;

  if (++w_ptr_offset >= BUFSIZE) {
    w_ptr_offset = 0;
  }

  if (w_ptr_offset == r_ptr_offset) {
    state = Full;
  }
  else {
    state = Normal;
  }
}

#endif
