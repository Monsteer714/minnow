#include "tcp_receiver.hh"
#include "debug.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )
{
  if (message.RST) {
    reader().set_error();
    return;
  }
  if (message.SYN) {
    this->zero_point = message.seqno;
    this->syn_flag = true;
    this->fin_flag = false;
    reassembler_.syn_flag = true;
    reassembler_.fin_flag = false;
  }
  if (message.FIN) {
    this->fin_flag = true;
  }
  string data = message.payload;
  uint64_t first_index = message.seqno.unwrap(this->zero_point, reassembler_.nextIndex())
                         + message.SYN;
  reassembler_.insert(first_index, data, message.FIN);
  if (fin_flag && writer().is_closed()) {
    reassembler_.fin_flag = true;
  }
}

TCPReceiverMessage TCPReceiver::send() const
{
  TCPReceiverMessage receive;
  if (writer().has_error()) {
    receive.RST = true;
    return receive;
  }
  if (this->syn_flag) {
    uint64_t left_edge = writer().bytes_pushed() + reassembler_.syn_flag + reassembler_.fin_flag;
    receive.ackno = Wrap32::wrap(left_edge, zero_point);
  }
  receive.window_size = min((uint64_t) UINT16_MAX,
                               writer().available_capacity());
  return receive;
}