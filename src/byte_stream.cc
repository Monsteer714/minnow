#include "byte_stream.hh"
#include "debug.hh"
#include <algorithm>

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

// Push data to stream, but only as much as available capacity allows.
void Writer::push( string data )
{
  if (writer_closed
       || available_capacity() == 0) {
    return;
  }
  auto push_len = std::min(available_capacity(), static_cast<uint64_t> (data.size()));
  for (uint64_t i = 0; i < push_len; i++) {
    que.push_back(data[i]);
  }
  current_buffered += push_len;
  total_bytes_pushed += push_len;
}

// Signal that the stream has reached its ending. Nothing more will be written.
void Writer::close()
{
  writer_closed = true;
}

// Has the stream been closed?
bool Writer::is_closed() const
{
  return writer_closed;
}

// How many bytes can be pushed to the stream right now?
uint64_t Writer::available_capacity() const
{
  return capacity_ - current_buffered;
}

// Total number of bytes cumulatively pushed to the stream
uint64_t Writer::bytes_pushed() const
{
  return total_bytes_pushed;
}

// Peek at the next bytes in the buffer -- ideally as many as possible.
// It's not required to return a string_view of the *whole* buffer, but
// if the peeked string_view is only one byte at a time, it will probably force
// the caller to do a lot of extra work.
string_view Reader::peek() const
{
  return {que.begin() + read_index,
           que.begin() + read_index + current_buffered};
}

// Remove `len` bytes from the buffer.
void Reader::pop( uint64_t len )
{
  auto pop_len = std::min(len, static_cast<uint64_t> (que.size()));
  read_index += pop_len;
  current_buffered -= pop_len;
  total_bytes_popped += pop_len;

  if (read_index * 2 >= que.size()) {
    for (uint64_t i = 0; i < que.size() - read_index; i++) {
      que[i] = que[i + read_index];
    }
    que.resize(que.size() - read_index);
    read_index = 0;
  }
}

// Is the stream finished (closed and fully popped)?
bool Reader::is_finished() const
{
  return writer_closed && que.empty();
}

// Number of bytes currently buffered (pushed and not popped)
uint64_t Reader::bytes_buffered() const
{
  return current_buffered;
}

// Total number of bytes cumulatively popped from stream
uint64_t Reader::bytes_popped() const
{
  return total_bytes_popped;
}