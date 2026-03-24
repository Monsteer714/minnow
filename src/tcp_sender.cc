#include "tcp_sender.hh"
#include "debug.hh"
#include "tcp_config.hh"

using namespace std;

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::sequence_numbers_in_flight() const {
    return next_seqno_ + syn_flag_ + fin_flag_ - last_ackno_;
}

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::consecutive_retransmissions() const {
    return consecutive_retransmissions_;
}

void TCPSender::push(const TransmitFunction &transmit) {
    //while (reader.peek.size >= 0 && window_size > 0)
    TCPSenderMessage msg = make_empty_message();
    if (!syn_sent_) {
        syn_flag_ = true;
        syn_sent_ = true;
        msg.SYN = true;
    }
    if (writer().is_closed()) {
        fin_flag_ = true;
        msg.FIN = true;
    }
    std::string payload = static_cast<std::string>(reader().peek().substr(0, window_size_));
    //if FIN was not included in the window?
    if (payload.size() == 0 && !msg.SYN && !msg.FIN) {
        return;
    }
    if (msg.FIN && msg.payload.size() + msg.FIN > window_size_) {
        return;
    }
    msg.payload = payload;
    reader().pop(payload.size());
    transmit(msg);//keep sending?

    outstanding_segments_.emplace(next_seqno_, payload);

    next_seqno_ += payload.size();
    window_size_ -= payload.size();
}

TCPSenderMessage TCPSender::make_empty_message() const {
    TCPSenderMessage msg;
    msg.seqno = Wrap32::wrap(next_seqno_ + syn_flag_ + fin_flag_, isn_); // No seq is consumed,so left edge of sender's window?
    msg.SYN = false; // Need to SYN if?
    msg.FIN = false; // Need to FIN if?
    msg.RST = false;
    msg.payload = "";
    return msg;
}

void TCPSender::receive(const TCPReceiverMessage &msg) {
    if (msg.ackno->unwrap(isn_, next_seqno_) > next_seqno_ + syn_flag_) {
        return;
    }
    last_ackno_ = msg.ackno->unwrap(isn_, next_seqno_);
    right_window_edge_ = last_ackno_ + static_cast<uint64_t>(msg.window_size) - 1;
    left_window_edge_ = next_seqno_ + syn_flag_;
    window_size_ = right_window_edge_ - left_window_edge_ + 1;

    for (auto [seqno, segment] : outstanding_segments_) {
        if (last_ackno_ >= seqno + segment.size()) {
            outstanding_segments_.erase(seqno);
        }
    }
}

void TCPSender::tick(uint64_t ms_since_last_tick, const TransmitFunction &transmit) {
    timer_.time(ms_since_last_tick);
    if (timer_.expired() == true) {
        TCPSenderMessage msg = make_empty_message();
        msg.seqno = Wrap32(outstanding_segments_.begin()->first);
        msg.payload = outstanding_segments_.begin()->second;
        transmit(msg);
    }
}
