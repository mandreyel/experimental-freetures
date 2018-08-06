#ifndef FREETURES_PROMISE_HPP
#define FREETURES_PROMISE_HPP

#include <memory>
#include <system_error>

#include "future.hpp"
#include "detail/shared_state.hpp"
#include "detail/scheduler.hpp"

namespace ft {

template<typename T>
class promise
{
    std::shared_ptr<detail::shared_state<T>> state_;

public:
    /**
     * Constructs the promise and its associated shared state.
     */
    explicit promise(detail::scheduler& s)
        : state_(std::make_shared<detail::shared_state<T>>(s))
    {}

    /** Returns a future associated with this promise. */
    future<T> get_future() { return {state_}; }

    void set_value(T&& t)
    {
        state_->set_value(std::move(t));
    }

    void set_error(std::error_code error)
    {
        state_->set_error(error);
    }

    void set_timeout()
    {
        state_->set_timeout();
    }

    bool is_ready() const
    {
        return state->is_ready();
    }

    /**
     * @brief Invokes the appropriate handler depending on whether prmoise has
     * a value, an error, or if it has timed out.
     *
     * @note This function should be invoked by the scheduler associated with
     * this promise.
     *
     * @throw If promise is not ready, an exception is thrown. TODO which?
     */
    void invoke_handler()
    {
        state->invoke_handler();
    }
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
    explicit promise(detail::scheduler& s)
        : state_(std::make_shared<detail::shared_state<T>>(s))
    {}

    /** Returns a future associated with this promise. */
    future<void> get_future() { return {state_}; }

    void set_error(std::error_code error)
    {
        state_->set_error(error);
    }

    void set_timeout()
    {
        state_->set_timeout();
    }

    /**
     * Returns true if promise has a value, an error or a timeout associated
     * with it.
     */
    bool is_ready() const
    {
        return state->is_ready();
    }

    /**
     * @brief Invokes the appropriate handler depending on whether prmoise has
     * a value, an error, or if it has timed out.
     *
     * @note This function should be invoked by the scheduler associated with
     * this promise.
     *
     * @throw If promise is not ready, an exception is thrown. TODO which?
     */
    void invoke_handler()
    {
        state->invoke_handler();
    }
};
} // ft

#endif
