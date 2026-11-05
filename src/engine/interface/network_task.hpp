#pragma once

#include "engine/interface/i_network.hpp"

#include <coroutine>
#include <exception>

namespace game::net {

// Coroutine that runs as soon as it starts and destroys itself when it finishes (fire-and-forget).
//   net::Task serve(INetwork& network) {
//       while (true) {
//           NetEvent_t event = co_await net::nextEvent(network);
//           ...
//       }
//   }
struct Task {
    struct promise_type {
        Task get_return_object() { return {}; }
        std::suspend_never initial_suspend() noexcept { return {}; }
        std::suspend_never final_suspend() noexcept { return {}; }
        void return_void() {}
        void unhandled_exception() { std::terminate(); }
    };
};

// If an event is already queued, it continues immediately; otherwise it wakes up when an event arrives in a later pump().
struct NextEvent {
    INetwork& network;

    [[nodiscard]] bool await_ready() const { return network.hasEvent(); }
    void await_suspend(std::coroutine_handle<> waiter) { network.setWaiter(waiter); }
    NetEvent_t await_resume() { return *network.nextEvent(); }
};

[[nodiscard]] inline NextEvent nextEvent(INetwork& network) {
    return NextEvent{network};
}

}  // namespace game::net
