#ifndef FREETURES_SCHEDULER_IMPL_HPP
#define FREETURES_SCHEDULER_IMPL_HPP

#include <deque>
#include <memory>

#include "../future.hpp"
#include "../promise.hpp"
#include "../time.hpp"
#include "reactor.hpp"
#include "type_traits.hpp"

namespace ft {
namespace detail {

struct ready_promise
{
    virtual void invoke_continuation() = 0;
    virtual ~ready_promise() {}
};

template<typename T>
struct concrete_ready_promise final : public ready_promise
{
    promise<T> p;

    void invoke_continuation() override
    {
    }
};

/**
 * @brief Concrete scheduler implementation.
 */
class scheduler
{
    // The queue of fulfilled promises that are ready to be delivered.
    std::deque<std::unique_ptr<ready_promise>> ready_promises_;
    reactor reactor_;

public:
    scheduler()
        : reactor_(*this)
    {}

    reactor& get_reactor() noexcept
    {
        return reactor_;
    }

    template<
        typename F,
        typename R = typename detail::callable_traits<F, void>::inner_result_type,
        typename = typename std::enable_if<detail::is_callable<F>::value>::type
    > future<R> post(F&& f)
    {
    }

    template<
        typename F,
        typename R = typename detail::callable_traits<F, void>::inner_result_type,
        typename = typename std::enable_if<detail::is_callable<F>::value>::type
    > future<R> defer(F&& f, duration delay)
    {
        //return wait(delay).then(std::forward(f));
    }

    template<
        typename F,
        typename = typename std::enable_if<detail::is_callable<F>::value>::type
    > void repeat(F&& f, duration frequency)
    {
    }

    //null_future wait(duration delay)
    //{
    //}

    void run()
    {
    }

    void stop()
    {
    }
};

} // detail
} // ft

#endif
