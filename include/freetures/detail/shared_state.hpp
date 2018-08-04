#ifndef FREETURES_SHARED_STATE_HPP
#define FREETURES_SHARED_STATE_HPP

#include <utility>

#include "../promise.hpp"
#include "type_traits.hpp"

namespace ft {
namespace detail {

struct fn_wrapper
{
    template<typename F>
    fn_wrapper(F&& f) {}
};

template<typename T>
class continuation
{
    promise<T> prom_;
    fn_wrapper func_;

public:
    continuation() = default;

    template<typename F>
    continuation(promise<T>&& p, F&& f)
        : prom_(std::forward<promise<T>>(p))
        , func_(std::forward<F>(f))
    {}
};

template<typename T>
class shared_state
{
    enum {
        not_ready,
        ready,
        error,
        timed_out,
    } status_ = not_ready;

    //T result;
    //continuation<T> continuation_;
    //??? on_error_;
    //??? on_timeout_;

public:
    template<typename R>
    void register_continuation(continuation<R>&& c)
    {
        // if already has callback: error
        switch(status_) {
        case not_ready:
            // register callback
            break;
        case ready:
            // invoke callback
            break;
        default:
            break;
        }
    }
};

} // detail
} // ft

#endif
