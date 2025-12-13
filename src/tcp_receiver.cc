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
    zero_point = message.seqno;
    syn_flag = true;
  }
  else if (message.seqno == zero_point) {
    return;
  }
  string data = message.payload;
  uint64_t checkpoint = writer().bytes_pushed() + syn_flag;
  uint64_t first_index = message.seqno.unwrap(this->zero_point, checkpoint);
  first_index = first_index == 0 ? first_index : first_index - 1;
  reassembler_.insert(first_index, data, message.FIN);
}

TCPReceiverMessage TCPReceiver::send() const
{
  TCPReceiverMessage receive;
  if (writer().has_error()) {
    receive.RST = true;
    return receive;
  }
  if (this->syn_flag) {
    uint64_t left_edge = writer().bytes_pushed()
                         + syn_flag + writer().is_closed();
    receive.ackno = Wrap32::wrap(left_edge, zero_point);
  }
<<<<<<< Updated upstream
  receive.window_size = min(static_cast<uint64_t>(UINT16_MAX),
                               writer().available_capacity());
=======
  receive.window_size = min((uint64_t) UINT16_MAX,
                             writer().available_capacity());
>>>>>>> Stashed changes
  return receive;
}