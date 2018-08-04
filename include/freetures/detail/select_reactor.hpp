#ifndef FREETURES_SELECT_REACTOR_HPP
#define FREETURES_SELECT_REACTOR_HPP

#include <string>

#include "../promise.hpp"
#include "../future.hpp"
#include "select_interrupter.hpp"

namespace ft {
namespace detail {

class scheduler;

struct descriptor_data
{
    int descriptor;
};

class select_reactor
{
    enum op
    {
        read,
        write,
        except,
        max,
    };

    scheduler& scheduler_;
    //fd_set fd_sets_[op::max];
    select_interrupter interrupter_;

public:
    select_reactor(scheduler& s)
        : scheduler_(s)
    {}

    future<void> start_read_op(descriptor_data& data)
    {
        // if shutting down, post immediate completion with error
        // enqueue operation in fd_sets_ and if its the first operation for this
        // descriptor, we need to interrupt the selector (because otherwise its
        // not going to be in the fd_set used by select and we're not going to
        // learn about any events for this descriptor)
    }

    future<void> start_write_op(descriptor_data& data)
    {
    }

    void run()
    {
        // reinstate fd_set:
        // 1) FD_ZERO(&fd_set)
        // 2) FD_SET(fd, &fd_set) for each outstanding work
        // 3) FD_SET(interrupter_.read_descriptor(), &fd_set)
        // then block on the select call 
        // int result = select(max_fd, fd_sets_[op::read], fd_sets_[op::write],
        // fd_sets_[op::except], timeout)
        // for each the descriptor that became available, execute the operation
        // (i.e. read fd, write fd etc) and if finished, set its associated
        // promise, then invoke scheduler_.post(promise)
    }

    void interrupt()
    {
        interrupter_.interrupt();
    }
};

} // detail
} // ft

#endif
