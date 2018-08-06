#ifndef FREETURES_FUTURE_HPP 
#define FREETURES_FUTURE_HPP 

#include <memory>
#include <type_traits>

#include "promise.hpp"
#include "detail/shared_state.hpp"
#include "detail/type_traits.hpp"

namespace ft {

/**
 * @code
 * future<int> async_operation();
 *
 * async_operation()
 *     .then([](int result) {
 *         // Do something with result.
 *         return std::to_string(result);
 *     }).on_error([](std::error_code error) {
 *         // Handle error.
 *     }).on_timeout([] {
 *         // Handle timeout.
 *     }).then([](std::string result) {
 *         // If no timeout or error occured, the async chain continues.
 *     }).then([] {
 *         // And on it goes... You get the idea.
 *     });
 * @endcode
 */
template<typename T>
struct future
{
    static_assert(not std::is_same<T, void>::value,
            "future<void> is not supported, use void_future instead");

    template<typename U>
    friend class future;

    std::weak_ptr<detail::shared_state<T>> state_;

public:
    // TODO cleaner API
    explicit future(std::weak_ptr<detail::shared_state<T>> state) : state_(state) {}

    /**
     * @brief Sets a handler to be invoked with the result of the future, once
     * the it is ready.
     *
     * If the handler returns a value, it is wrapped in a future. If it returns
     * a future, sucha a future (not the same one, of course) is returned.
     *
     * @code
     * future<int> async_operation();
     *
     * async_operation()
     *     .then([](int result) {
     *         // Do something with result.
     *     });
     * @endcode
     *
     * @param h The handler, which must accept a single parameter of `T`, that
     * is chained to this future.
     *
     * @return A future that will contain the return value of the handler.
     */
    template<
        typename Handler,
        typename HandlerTraits = callable_traits<Handler, T>,
        typename U = typename HandlerTraits::inner_result_type,
        typename = typename std::enable_if<
            not std::is_same<U, void>::value
            //is_callable<Handler>::value
            //and accepts_args<Handler, T>::value
        >::type
    > future<U> then(Handler&& h)
    {
        return attach_continuation<Handler>(std::forward<Handler>(h));
    }

    /**
     * @brief Specialization for handlers that return void.
     *
     * The handler is wrapped in a handler that returns `null_tag`.
     */
    template<
        typename Handler,
        typename HandlerTraits = callable_traits<Handler, T>,
        typename U = typename HandlerTraits::inner_result_type
    > auto then(Handler&& h)
        -> typename std::enable_if<
            std::is_same<U, void>::value,
            future<null_tag>
        >::type
    {
        return attach_continuation<Handler>([h](T t) -> null_tag {
            h(std::move(t));
            return null_tag();
        });
    }

    /**
     * @brief Sets a handler to be invoked if the future resulted in an error.
     *
     * Handler must return either a future wrapping the same type as this future
     * (i.e. `future<T>`), or a value of type T.
     *
     * @code
     * future<int> async_operation();
     *
     * async_operation()
     *     .then([](int result) {
     *         // Do something with result.
     *     }).on_error([](std::error_code error) {
     *         // Handle error.
     *     });
     * @endcode
     *
     * @param h The handler, which must accept a single parameter of type
     * `std::error_code`, describing the error.
     *
     * @return A reference to `*this`.
     */
    template<typename Handler>
    future<T>& on_error(Handler&& h)
    {
        return attach_error_handler<Handler>(std::forward<Handler>(h));
    }

    /**
     * @brief Sets a handler to be invoked if the future has timed out.
     *
     * Handler must return either a future wrapping the same type as this future
     * (i.e. `future<T>`), or a value of type T.
     *
     * @code
     * future<int> async_operation();
     *
     * async_operation()
     *     .then([](int result) {
     *         // Do something with result.
     *         return std::to_string(result);
     *     }).on_error([](std::error_code error) {
     *         // Handle error.
     *     }).on_timeout([] {
     *         // Handle timeout.
     *     });
     * @endcode
     *
     * @param h The handler that is invoked on a timeout, which accepts no
     * argument.
     *
     * @return A reference to `*this`.
     */
    template<typename Handler>
    future<T>& on_timeout(Handler&& h)
    {
        return attach_timeout_handler<Handler>(std::forward<Handler>(h));
    }

private:
    /**
     * @brief Specialization for a `then` continuation variant that returns a future.
     */
    template<
        typename Handler,
        typename HandlerTraits = callable_traits<Handler, T>,
        typename U = typename HandlerTraits::inner_result_type
    > auto attach_continuation(Handler&& handler)
        -> typename std::enable_if<HandlerTraits::returns_future, future<U>>::type
    {
        auto state = state_.lock();
        if(!state) {
            throw "TODO add proper exception";
        }

        // Handler returns a future, which means that it already has an
        // associated promise (only promises can create futures).
        //
        // The tricky part, however, is that this future (and thus its
        // associated promise) do not yet exist (since the future is returned by
        // handler, which hasn't run). Thus we create a bogus future-promise
        // pair, to which a continuation and possibly error and timeout handlers
        // may be attached, and when this continuation is invoked, we transfer
        // these handlers to the actual future the handler returns, so that the
        // scheduler associated with the future returned by handler can execute
        // them.
        promise<U> bogus_handler_promise(state->get_scheduler());
        auto bogus_handler_future = bogus_handler_promise.get_future();
        detail::continuation<U> cont([this, bogus_handler_promise, handler](T&& t) mutable
        {
            // Invoke the handler with the result to retrieve its future.
            auto handler_future = handler(std::forward<T>(t));
            auto handler_state = handler_future.state_.lock();
            if(!handler_state) {
                // TODO
                assert(0);
                return;
            }

            // Grab the future associated with our bogus promise so that we can
            // access its shared state.
            auto bogus_handler_future = bogus_handler_promise.get_future();
            auto bogus_state = bogus_handler_future.state_.lock();
            // This shouldn't happen.
            if(!bogus_state) { assert(0); }

            // Then move any handlers registered with the bogus future to the
            // actual future. This way, when the future returned by handler will
            // become ready, the scheduler can call them.
            bogus_state->move_handlers_to(*handler_state);
        });

        state->attach_continuation(std::move(cont));

        return bogus_handler_future;
    }

    /**
     * @brief Specialization for a `then` continuation variant that returns
     * a value.
     *
     * Since invoking this continuation returns non-future value, it means the
     * future in which it is going to be wrapped immediately becomes ready. The
     * reason it is wrapped in a future is so the continuation chain is not
     * broken.
     */
    template<
        typename Handler,
        typename HandlerTraits = callable_traits<Handler, T>,
        typename U = typename HandlerTraits::inner_result_type
    > auto attach_continuation(Handler&& handler)
        -> typename std::enable_if<not HandlerTraits::returns_future, future<U>>::type
    {
        auto state = state_.lock();
        if(!state) {
            throw "TODO add proper exception";
        }

        // Since handler returns a value, we need to create the promise
        // ourselves. Usually promises are created directly by schedulers, so this
        // means that we'll have to associate this new promise with the
        // scheduler of this future.
        promise<U> handler_promise(state->get_scheduler());
        auto handler_future = handler_promise.get_future();
        detail::continuation<U> cont([handler_promise, handler](T&& t) mutable
        {
            auto result = handler(std::move(t));
            // Since this continuation is only invoked if no error or timeout
            // occurred, `t` is valid, which needs to be passed to
            // `handler_future`.
            handler_promise.set_value(std::move(result));
            // Since handler returns a value the promise effectively immdiately
            // becomes fulfilled, notify `handler_future`'s executor that this
            // promise has been fulfilled  so that its continuation can be
            // invoked.
            auto future = handler_promise.get_future();
            auto state = future.state_.lock();
            assert(state);
            state->get_scheduler().post_ready_promise(std::move(handler_promise));
        });

        state->attach_continuation(std::move(cont));

        return handler_future;
    }

    /**
     * @brief Specialization for an `on_error` continuation variant that returns
     * a future.
     */
    template<
        typename Handler,
        typename HandlerTraits = callable_traits<Handler, T>,
        typename U = typename HandlerTraits::inner_result_type
    > auto attach_error_handler(Handler&& handler)
        -> typename std::enable_if<HandlerTraits::returns_future, future<U>>::type
    {
    }

    /**
     * @brief Specialization for an `on_error` continuation variant that returns
     * a value.
     */
    template<
        typename Handler,
        typename HandlerTraits = callable_traits<Handler, T>,
        typename U = typename HandlerTraits::inner_result_type
    > auto attach_error_handler(Handler&& handler)
        -> typename std::enable_if<not HandlerTraits::returns_future, future<U>>::type
    {
    }

    /**
     * @brief Specialization for an `on_timeout` continuation variant that returns
     * a future.
     */
    template<
        typename Handler,
        typename HandlerTraits = callable_traits<Handler, T>,
        typename U = typename HandlerTraits::inner_result_type
    > auto attach_timeout_handler(Handler&& h)
        -> typename std::enable_if<HandlerTraits::returns_future, future<U>>::type
    {
    }

    /**
     * @brief Specialization for an `on_timeout` continuation variant that returns
     * a value.
     */
    template<
        typename Handler,
        typename HandlerTraits = callable_traits<Handler, T>,
        typename U = typename HandlerTraits::inner_result_type
    > auto attach_timeout_handler(Handler&& h)
        -> typename std::enable_if<not HandlerTraits::returns_future, future<U>>::type
    {
    }
};

/**
 * @brief A future that is not associated with a value, i.e. the operation that
 * returns it would return `void`.
 */
using void_future = future<null_tag>;

} // ft

#endif
