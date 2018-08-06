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
        typename HandlerTraits = detail::callable_traits<Handler, T>,
        typename U = typename HandlerTraits::inner_result_type,
        typename = typename std::enable_if<
            detail::is_callable<Handler>::value and
            detail::accepts_args<Handler, T>::value
        >::type
    > future<U> then(Handler&& h)
    {
        return register_continuation<Handler>(std::forward<Handler>(h));
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
        return register_error_handler<Handler>(std::forward<Handler>(h));
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
        return register_timeout_handler<Handler>(std::forward<Handler>(h));
    }

private:
    /**
     * @brief Specialization for a `then` continuation variant that returns a future.
    template<
        typename Handler,
        typename HandlerTraits = detail::callable_traits<Handler, T>,
        typename U = typename HandlerTraits::inner_result_type,
        typename = typename std::enable_if<HandlerTraits::returns_future>::type
    > future<U> register_continuation(Handler&& h)
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
        // may be chained, and when this continuation is invoked, we transfer
        // these handlers to the actual future the handler returns. 

        promise<U> bogus_handler_promise(state->get_scheduler());
        auto bogus_handler_future = p.get_future();

        state->register_continuation([bogus_handler_promise, handler](T&& r)
        {
            // Invoke the handler with the result to retrieve its future.
            auto handler_future = handler(std::forward<T>(r));
            auto handler_state = handler_future.state_.lock();
            if(!handler_state) {
                // TODO
                return;
            }

            // Grab the futuer associated with our bogus promise so that we can
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

        return bogus_handler_future;
    }
     */

    /**
     * @brief Specialization for a `then` continuation variant that returns
     * a value.
     *
     * Since invoking this continuation returns non-future value, it means the
     * future in which it is going to be wrapped immediately becomes ready. The
     * reason it is wrapped in a future is so the continuation chain is not
     * broken.
     *
     * However, this means that the entire promise-shared_state-future trio
     * needs to be constructed in place, meaning the promise is moved into its
     * own `shared_state`. Due to using a `std::shared_ptr`, this introduces
     * a reference cycle, which is taken care of within `shared_state`.
     */
    template<
        typename Handler,
        typename HandlerTraits = detail::callable_traits<Handler, T>,
        typename U = typename HandlerTraits::inner_result_type,
        typename = typename std::enable_if<not HandlerTraits::returns_future>::type
    > future<U> register_continuation(Handler&& h)
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

        //state->register_continuation([handler_promise](T&& r)
        //{
            //// Since this continuation is only invoked if no error or timeout
            //// occurred, `r` is valid, which needs to be passed to
            //// `handler_future`.
            //handler_promise.set_value(std::move(r));
            //// Now, notify the associated executor that this promise has been
            //// fulfilled so that its associatd future's (`handler_future`)
            //// continuation can be invoked.
            ////handler_promise TODO
        //});

        return handler_future;
    }

    //template<
        //typename Handler,
        //typename HandlerTraits = detail::callable_traits<Handler, T>,
        //typename U = typename HandlerTraits::inner_result_type,
        //typename = typename std::enable_if<HandlerTraits::returns_future>::type
    //> future<T> register_error_handler(Handler&& h)
    //{
    //}

    template<
        typename Handler,
        typename HandlerTraits = detail::callable_traits<Handler, T>,
        typename U = typename HandlerTraits::inner_result_type,
        typename = typename std::enable_if<not HandlerTraits::returns_future>::type
    > future<T> register_error_handler(Handler&& h)
    {
    }

    //template<
        //typename Handler,
        //typename HandlerTraits = detail::callable_traits<Handler, T>,
        //typename U = typename HandlerTraits::inner_result_type,
        //typename = typename std::enable_if<HandlerTraits::returns_future>::type
    //> future<T> register_timeout_handler(Handler&& h)
    //{
    //}

    template<
        typename Handler,
        typename HandlerTraits = detail::callable_traits<Handler, T>,
        typename U = typename HandlerTraits::inner_result_type,
        typename = typename std::enable_if<not HandlerTraits::returns_future>::type
    > future<T> register_timeout_handler(Handler&& h)
    {
    }
};

class null_future_tag {};
using null_future = future<null_future_tag>;

} // ft

#endif
