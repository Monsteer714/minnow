#include "tcp_sender.hh"

#include "debug.hh"
#include "tcp_config.hh"

using namespace std;

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::sequence_numbers_in_flight() const {
    return next_seqno_ - last_ackno_;
}

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::consecutive_retransmissions() const {
    return consecutive_retransmissions_;
}

void TCPSender::push(const TransmitFunction &transmit) {
    while ((!fin_sent_ && sender_window_size_ > 0) || zero_window_size_received || !syn_sent_) {
        TCPSenderMessage msg = make_empty_message();
        if (!syn_sent_) {
            syn_sent_ = true;
            msg.SYN = true;
        }
        if (zero_window_size_received) {
            temp_sender_window_size_ = sender_window_size_;
            sender_window_size_ = 1;
        }
        const std::string payload = static_cast<std::string>(reader().peek().substr(0,
            min({
                reader().bytes_buffered(), sender_window_size_, static_cast<uint64_t>(TCPConfig::MAX_PAYLOAD_SIZE)
            }))); //reader.byte_buffered?
        msg.payload = payload;
        if (writer().is_closed() && !fin_sent_) {
            uint64_t last_send_seqno = reader().bytes_popped() + reader().bytes_buffered();
            if (next_seqno_ + msg.sequence_length() >= last_send_seqno) {
                if (payload.size() + msg.FIN < sender_window_size_) {
                    msg.FIN = true;
                }
            }
        }
        if (msg.sequence_length() == 0) {
            return;
        }
        reader().pop(payload.size());
        transmit(msg);
        if (msg.FIN) {
            fin_sent_ = true;
        }
        next_seqno_ = next_seqno_ + msg.SYN + msg.FIN;
        outstanding_segments_.emplace(next_seqno_, msg);

        next_seqno_ += payload.size();
        if (zero_window_size_received) {
            zero_window_size_received = false;
            sender_window_size_ = temp_sender_window_size_;
        } else {
            sender_window_size_ -= payload.size();
        }
    }
}

TCPSenderMessage TCPSender::make_empty_message() const {
    TCPSenderMessage msg;
    msg.seqno = Wrap32::wrap(next_seqno_, isn_);
    msg.SYN = false;
    msg.FIN = false;
    msg.RST = input_.has_error();
    msg.payload = "";
    return msg;
}

void TCPSender::receive(const TCPReceiverMessage &msg) {
    if (msg.RST) {
        input_.set_error();
    }
    const uint64_t receive_ackno_ = msg.ackno ? msg.ackno->unwrap(isn_, next_seqno_) : last_ackno_;
    if (receive_ackno_ > next_seqno_) {
        return;
    }
    if (receive_ackno_ > last_ackno_) {
        timer_.initialize();
        consecutive_retransmissions_ = 0;
        if (!outstanding_segments_.empty()) {
            timer_.reset();
        }
        last_ackno_ = receive_ackno_;
    }
    receiver_window_size_ = static_cast<uint64_t>(msg.window_size);
    right_window_edge_ = last_ackno_ + receiver_window_size_ - 1;
    sender_window_size_ = right_window_edge_ - next_seqno_ + 1 - !syn_sent_;
    zero_window_size_received = msg.window_size == 0 ? true : false;

    while (!outstanding_segments_.empty()) {
        auto segment = outstanding_segments_.begin();
        auto seqno = segment->first;
        auto message = segment->second;
        if (last_ackno_ > seqno + message.payload.size() - 1) {
            outstanding_segments_.erase(seqno);
        } else {
            break;
        }
    }
}

void TCPSender::tick(uint64_t ms_since_last_tick, const TransmitFunction &transmit) {
    if (outstanding_segments_.empty()) {
        return;
    }
    timer_.time(ms_since_last_tick);
    if (timer_.expired() == true && !outstanding_segments_.empty()) {
        TCPSenderMessage msg = outstanding_segments_.begin()->second;
        if (receiver_window_size_ > 0) {
            consecutive_retransmissions_++;
            timer_.double_RTO();
        }
        timer_.reset();
        transmit(msg);
    }
}
