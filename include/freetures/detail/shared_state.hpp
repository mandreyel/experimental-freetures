#ifndef FREETURES_SHARED_STATE_HPP
#define FREETURES_SHARED_STATE_HPP

#include <utility>
#include <functional>
#include <cassert>

#include "../promise.hpp"
#include "scheduler.hpp"
#include "optional.hpp"
#include "type_traits.hpp"

namespace ft {
namespace detail {

using namespace tl;

/**
 * This class handles the variation between a void future and a normal future
 * (void future's handler does not take an argument).
 */
template<typename T>
class continuation
{
    std::function<void(T)> handler_;

public:
    continuation() = default;

    template<typename Handler>
    continuation(Handler&& h)
        : handler_(std::forward<Handler>(h))
    {}

    template<typename U>
    void operator()(U&& u) {
        handler_(std::forward<U>(u));
    }
};

// TODO For now, due to some complications, null future continuations must take
// a null_tag argument (which they should ignore). This will be fixed later, but
// it facilitates quicker iteration.
//template<>
//class continuation<null_tag>
//{
    //std::function<void()> handler_;

//public:
    //continuation() = default;

    //template<typename Handler>
    //continuation(Handler&& h)
        //: handler_(std::forward<Handler>(h))
    //{}

    //void operator()(null_tag) {
        //handler_();
    //}
//};

template<typename T>
class shared_state
{
    enum {
        not_ready,
        ready,
        error,
        timed_out,
    } status_ = not_ready;

    scheduler& scheduler_;
    optional<T> result_;
    optional<std::error_code> error_;

    optional<continuation<T>> continuation_;
    //??? on_error_;
    //??? on_timeout_;

public:
    explicit shared_state(scheduler& s) : scheduler_(s) {}

    scheduler& get_scheduler()
    {
        return scheduler_;
    }

    bool is_ready() const noexcept { return status_ == ready; }

    void set_value(T&& t)
    {
        if(status_ != not_ready) {
            // TODO
            throw "ERROR";
        }
        result_ = optional<T>(std::move(t));
        status_ = ready;
    }

    void set_error(std::error_code error)
    {
        if(status_ != not_ready) {
            // TODO
            throw "ERROR";
        }
        error_ = optional<T>(error);
        status_ = error;
    }

    void set_timeout()
    {
        if(status_ != not_ready) {
            // TODO
            throw "ERROR";
        }
        status_ = timed_out;
    }

    void attach_continuation(continuation<T>&& c)
    {
        switch(status_) {
        case not_ready:
        case ready:
            // register callback
            break;
        default:
            throw "cannot overwrite existing continuation";
            break;
        }
    }

    void move_handlers_to(shared_state& other)
    {
        if(this == &other) {
            return;
        }
    }

    void invoke_handler()
    {
        if(status_ == not_ready) {
            throw "cannot invoke handler on not ready promise";
        }

        switch(status_) {
        case ready:
            break;
        case error:
            break;
        case timed_out:
            break;
        default: assert(0);
        }
    }
};

} // detail
} // ft

#endif
