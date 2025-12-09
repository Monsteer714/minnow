#include "tcp_receiver.hh"
#include "debug.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  if (message.RST) {
    reader().set_error();
    return;
  }
  const auto seqno = message.seqno;
  if (!syn_flag) {
    if (message.SYN) {
      zero_point = seqno;
      syn_flag = true;
    } else {
      return;
    }
  }
  const auto first_index = seqno.unwrap(zero_point, 0);
  const auto data = message.payload;
  bool is_last_substring = false;
  if (message.FIN) {
    is_last_substring = true;
    fin_flag = true;
  }
  reassembler_.insert(first_index, data, is_last_substring);
}

TCPReceiverMessage TCPReceiver::send() const
{
  TCPReceiverMessage receive;
  if (writer().has_error()) {
    receive.RST = true;
    return receive;
  }
  if (this->syn_flag) {
    receive.ackno = Wrap32::wrap(reader().bytes_buffered(), zero_point)
                    + syn_flag + fin_flag;
  }
  receive.window_size = min((uint64_t) UINT16_MAX,
                               writer().available_capacity());
  return receive;
}