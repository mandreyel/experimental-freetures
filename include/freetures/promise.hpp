#ifndef FREETURES_PROMISE_HPP
#define FREETURES_PROMISE_HPP

#include <memory>
#include <system_error>

#include "future.hpp"
#include "detail/shared_state.hpp"

namespace ft {

template<typename T>
class promise
{
    std::shared_ptr<detail::shared_state<T>> state_;

public:
    /**
     * Constructs the promise and its associated shared state.
     */
    promise() : state_(std::make_shared<detail::shared_state<T>>()) {}

    /**
     * @brief Returns a future associated with this promise.
     */
    future<T> get_future();

    void set_value(T&& t);
    void set_error(std::error_code& error);
    void set_timeout();

    bool is_fulfilled() const;

    void invoke_handler();
};

/** @brief Void specialization. */
template<>
class promise<void>
{
    std::shared_ptr<detail::shared_state<void>> state_;

public:
    /**
     * Constructs the promise and its associated shared state.
     */
    promise() : state_(std::make_shared<detail::shared_state<void>>()) {}

    /**
     * @brief Returns a future associated with this promise.
     */
    future<void> get_future();

    void set_value();
    void set_error(std::error_code& error);
    void set_timeout();

    bool is_fulfilled() const;

    void invoke_handler();
};
} // ft

#endif
