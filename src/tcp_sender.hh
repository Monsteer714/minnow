#pragma once

#include "byte_stream.hh"
#include "tcp_receiver_message.hh"
#include "tcp_sender_message.hh"

#include <functional>

class RetransmissonTimer {
private:
    uint64_t initial_RTO_ms_;
    uint64_t current_RTO_ms_;
    uint64_t time_elapsed_;

public:
    RetransmissonTimer(uint64_t initial_RTO_ms_)
        : initial_RTO_ms_(initial_RTO_ms_), current_RTO_ms_(initial_RTO_ms_),
          time_elapsed_(static_cast<uint64_t>(0)) {
    }

    void start() {
        time_elapsed_ = 0;
    }

    void reset() {
        time_elapsed_ = 0;
        current_RTO_ms_ = initial_RTO_ms_;
    }

    void double_RTO() {
        current_RTO_ms_ *= 2;
    }

    void time(uint64_t ms_passed) {
        time_elapsed_ += ms_passed;
    }

    bool alarm() {
        return time_elapsed_ >= current_RTO_ms_;
    }
};

class TCPSender {
public:
    /* Construct TCP sender with given default Retransmission Timeout and possible ISN */
    TCPSender(ByteStream &&input, Wrap32 isn, uint64_t initial_RTO_ms)
        : input_(std::move(input)), isn_(isn), initial_RTO_ms_(initial_RTO_ms), timer_(initial_RTO_ms) {
    }

    /* Generate an empty TCPSenderMessage */
    TCPSenderMessage make_empty_message() const;

    /* Receive and process a TCPReceiverMessage from the peer's receiver */
    void receive(const TCPReceiverMessage &msg);

    /* Type of the `transmit` function that the push and tick methods can use to send messages */
    using TransmitFunction = std::function<void(const TCPSenderMessage &)>;

    /* Push bytes from the outbound stream */
    void push(const TransmitFunction &transmit);

    /* Time has passed by the given # of milliseconds since the last time the tick() method was called */
    void tick(uint64_t ms_since_last_tick, const TransmitFunction &transmit);

    // Accessors
    uint64_t sequence_numbers_in_flight() const; // For testing: how many sequence numbers are outstanding?
    uint64_t consecutive_retransmissions() const; // For testing: how many consecutive retransmissions have happened?
    const Writer &writer() const { return input_.writer(); }
    const Reader &reader() const { return input_.reader(); }
    Writer &writer() { return input_.writer(); }

private:
    Reader &reader() { return input_.reader(); }

    ByteStream input_;
    Wrap32 isn_;
    uint64_t initial_RTO_ms_;
    RetransmissonTimer timer_;
};
