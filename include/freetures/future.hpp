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
template<typename R>
struct future
{
    std::weak_ptr<detail::shared_state<R>> state_;

public:
    /**
     * @brief Sets a handler to be invoked with the result of the future, once
     * the it is ready.
     *
     * If the handler returns a value, it is wrapped in a future. If it does
     * not, the return value is unaltered. TODO more lucid explanation
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
     * @param h The handler, which must accept a single parameter of `R`, that
     * is chained to this future.
     *
     * @return A future that will contain the return value of the handler.
     */
    template<
        typename Handler,
        typename HandlerTraits = detail::callable_traits<Handler, R>,
        typename R2 = typename HandlerTraits::inner_result_type,
        typename = typename std::enable_if<
            detail::is_callable<Handler>::value and
            detail::accepts_args<Handler, R>::value
        >::type
    > future<R2> then(Handler&& h)
    {
        return register_continuation<Handler, HandlerTraits>(std::forward<Handler>(h));
    }

    /**
     * @brief Sets a handler to be invoked if the future resulted in an error.
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
    future<R>& on_error(Handler&& h);

    /**
     * @brief Sets a handler to be invoked if the future has timed out.
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
    future<R>& on_timeout(Handler&& h);

private:
    /**
     * @brief Specialization for a `then` continuation variant that returns a future.
    template<
        typename Handler,
        typename HandlerTraits,
        typename R2 = typename HandlerTraits::inner_result_type,
        typename = typename std::enable_if<HandlerTraits::returns_future>::type
    > future<R2> register_continuation(Handler&& h)
    {
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
        typename HandlerTraits,
        typename R2 = typename HandlerTraits::inner_result_type,
        typename = typename std::enable_if<not HandlerTraits::returns_future>::type
    > future<R2> register_continuation(Handler&& h)
    {
        auto state = state_.lock();
        if(!state) {
            throw "TODO add proper exception";
        }

        promise<R2> p;
        future<R2> f = p.get_future();
        detail::continuation<R2> cont(std::move(p), std::forward<Handler>(h));

        state->register_continuation(std::move(cont));

        return f;
    }
};

} // ft

#endif
