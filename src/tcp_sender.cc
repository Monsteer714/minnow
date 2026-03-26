#include "tcp_sender.hh"

#include <iostream>

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
    if (writer().is_closed() && !fin_sent_) {
        fin_flag_ = true;
        msg.FIN = true;
    }
    const std::string payload = static_cast<std::string>(reader().peek().substr(0, window_size_));
    //if FIN was not included in the window?
    if (msg.SYN && msg.FIN) { // SYN + FIN in send_close.cc
    } else if (payload.size() == 0 && !msg.SYN && !msg.FIN) {
        return;
    } else if (msg.FIN && msg.payload.size() + msg.FIN > window_size_) {
        return;
    }
    msg.payload = payload;
    reader().pop(payload.size());
    transmit(msg); //keep sending?
    if (msg.FIN) {
        fin_sent_ = true;
    }
    outstanding_segments_.emplace(next_seqno_ + syn_flag_ + fin_flag_, msg);

    next_seqno_ += payload.size();
    window_size_ -= payload.size();
}

TCPSenderMessage TCPSender::make_empty_message() const {
    TCPSenderMessage msg;
    msg.seqno = Wrap32::wrap(next_seqno_ + syn_flag_ + fin_flag_, isn_);
    msg.SYN = false;
    msg.FIN = false;
    msg.RST = false;
    msg.payload = "";
    return msg;
}

void TCPSender::receive(const TCPReceiverMessage &msg) {
    const uint64_t receive_ackno_ = msg.ackno->unwrap(isn_, next_seqno_);
    if (receive_ackno_ > next_seqno_ + syn_flag_ + fin_flag_) {
        return;
    }

    last_ackno_ = receive_ackno_;
    right_window_edge_ = last_ackno_ + static_cast<uint64_t>(msg.window_size) - 1;
    left_window_edge_ = next_seqno_ + syn_flag_;
    window_size_ = right_window_edge_ - left_window_edge_ + 1;

    for (auto [seqno, message]: outstanding_segments_) {
        if (last_ackno_ > seqno + message.payload.size() - 1) {
            outstanding_segments_.erase(seqno);
            if (!outstanding_segments_.empty()) {
                timer_.reset();
            }
            consecutive_retransmissions_ = 0;
            break;
        }
    }
}

void TCPSender::tick(uint64_t ms_since_last_tick, const TransmitFunction &transmit) {
    timer_.time(ms_since_last_tick);
    if (timer_.expired() == true) {
        TCPSenderMessage msg = outstanding_segments_.begin()->second;
        timer_.double_RTO();
        timer_.reset();
        consecutive_retransmissions_++;
        transmit(msg);
    }
}
