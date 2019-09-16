#ifndef IMP_CIRCULAR_RW_BUFFER
#define IMP_CIRCULAR_RW_BUFFER

#include "constants.hpp"

#include <utility>

class CircularRWBufferBase {
public:
  enum class ReadError { BufferEmpty };

  enum class WriteError { BufferFull };

  enum class State { Empty, Normal, Full };
};

class CircularRWBuffer : CircularRWBufferBase {
public:
  template <typename... Args>
  void write(const Args... datas);

  const u8 read();

  const State get_state() const;

private:
  static constexpr size_t BUFSIZE = 128;

  void write_impl(const u8 data);

  u8 buffer[BUFSIZE] = {0};
  size_t r_ptr_offset = 0;
  size_t w_ptr_offset = 0;
  State state = State::Empty;
};

template <typename... Args>
void CircularRWBuffer::write(const Args... datas)
{
  (write_impl(std::forward<const Args>(datas)), ...);
}

const u8 CircularRWBuffer::read()
{
  if (state == State::Empty) {
    throw ReadError::BufferEmpty;
  }

  const u8 result = *(buffer + r_ptr_offset);

  if (++r_ptr_offset >= BUFSIZE) {
    r_ptr_offset = 0;
  }

  if (r_ptr_offset == w_ptr_offset) {
    state = State::Empty;
  }
  else {
    state = State::Normal;
  }

  return result;
}

const CircularRWBufferBase::State CircularRWBuffer::get_state() const
{
  return state;
}

void CircularRWBuffer::write_impl(const u8 data)
{
  if (state == State::Full) {
    throw WriteError::BufferFull;
  }

  *(buffer + w_ptr_offset) = data;

  if (++w_ptr_offset >= BUFSIZE) {
    w_ptr_offset = 0;
  }

  if (w_ptr_offset == r_ptr_offset) {
    state = State::Full;
  }
  else {
    state = State::Normal;
  }
}

#endif
