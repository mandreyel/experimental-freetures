#ifndef FREETURES_SCHEDULER_HPP
#define FREETURES_SCHEDULER_HPP

#include "time.hpp"
#include "future.hpp"
#include "detail/type_traits.hpp"
#include "detail/scheduler.hpp"

namespace ft {

class scheduler
{
    detail::scheduler impl_;
public:
    /**
     * @brief Posts a function for invocation by @ref run.
     *
     * This may be used to post asynchronous completion handlers for which its
     * assiocatied operation has been concluded in a non-asynchronous fashion
     * such that the completion handler is still invoked as though the operation
     * had finished asynchronously. Thus strong guarantees about API contracts
     * can be kept.
     *
     * @code
     * ft::scheduler scheduler;
     * // Post a function, which is *not* invoked here.
     * scheduler.post([s] { std::cout << s << '\n'; });
     * // ...
     * // Run scheduler, where previously posted functions (such as the one
     * // above) will be invoked.
     * scheduler.run();
     * @endcode
     * 
     * @param f The function which will be executed by @ref run. It is guaranteed
     * not to be invoked from within this function.
     *
     * @return A future through which the result of invoking @p f, once
     * it's been executed, may be accessed.
     */
    template<
        typename F,
        typename R = typename detail::callable_traits<F, void>::inner_result_type,
        typename = typename std::enable_if<detail::is_callable<F>::value>::type
    > future<R> post(F&& f)
    {
        return impl_.post(std::forward<F>(f));
    }

    /**
     * @brief Posts a function for invocation by @ref run that will be invoked
     * after at least @p duration units.
     *
     * @code
     * // TODO come up with a suitable example
     * @endcode
     * 
     * @param f The function which will be executed by @p run. It is guaranteed
     * not to be invoked from within this function.
     * @param delay The minimum time by which the invocation of @p f is delayed.
     *
     * @return A future through which the result of invoking @p f, once
     * it's been executed, may be accessed.
     */
    template<
        typename F,
        typename R = typename detail::callable_traits<F, void>::inner_result_type,
        typename = typename std::enable_if<detail::is_callable<F>::value>::type
    > future<R> defer(F&& f, duration delay)
    {
        return impl_.defer(std::forward<F>(f), delay);
    }

    /**
     * @brief Posts a function for invocation by @ref run that will be invoked
     * after at least @p duration units.
     *
     * @code
     * // TODO come up with a suitable example
     * ft::scheduler scheduler;
     * scheduler.repeat([&event_generator] {
     *     event_generator.kick();
     * }, time::seconds(1));
     * @endcode
     * 
     * @param f The function which will be executed by @p run. It is guaranteed
     * not to be invoked from within this function.
     * @param frequency The interval that defines how frequently @p is going to
     * be invoked.
     *
     * @return Since the function is repeatedly invoked, no future is returned,
     * since attaching a continuation would not make sense.
     */
    template<
        typename F,
        typename = typename std::enable_if<detail::is_callable<F>::value>::type
    > void repeat(F&& f, duration frequency)
    {
        impl_.repeat(std::forward<F>(f), frequency);
    }

    /**
     * @brief Returns a future that may be used to delay its continuation by @p
     * delay units.
     *
     * It may be used to block a future's continuation's invocation, as shown in
     * the code example below.
     *
     * @code
     * ft::scheduler scheduler;
     * scheduler.post([&scheduler, pin] {
     *     gpio_set_level(pin, 0);
     *     return scheduler.wait(ft::milliseconds(5));
     * }).then([pin] {
     *     gpio_set_level(pin, 1);
     * });
     * @endcode
     *
     * @
     */
    null_future wait(duration delay)
    {
        //return impl_.wait(delay);
    }

    /**
     * @brief Runs the scheduler's event processing loop.
     *
     * This function blocks until all ready events have been processed.
     * This means that asynchronous events *must* be registered before calling
     * run, otherwise the application will never start.
     */
    void run()
    {
        impl_.run();
    }

    void stop()
    {
        impl_.stop();
    }
};

} // ft

#endif
