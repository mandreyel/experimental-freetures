#ifndef FREETURES_SELECT_REACTOR_HPP
#define FREETURES_SELECT_REACTOR_HPP

#include <string>

#include "../promise.hpp"
#include "../future.hpp"
#include "select_interrupter.hpp"

#include <sys/select.h>

namespace ft {
namespace detail {

class scheduler;

struct descriptor_data
{
    int descriptor;
    // operations
};

class select_reactor
{
    enum op
    {
        read,
        write,
        except,
    };

    // A reference to the scheduler to post ready promises for completion
    // handler invocation.
    scheduler& scheduler_;

    // A call to select will block until there is a ready descriptor, but we may
    // need to unblock the thread to handle events outside the reactor. For
    // this, we employ an interrupter, which is simply a pair of loopback
    // sockets, where the read side will be passed to select and when an
    // interrupt is needed, we write to the other end of the socket, making the
    // select call return.
    select_interrupter interrupter_;

    // A set of file descriptor sets corresponding to the 3 types of operations.
    fd_set fd_sets_[3];

    bool stopping_ = false;

public:
    select_reactor(scheduler& s)
        : scheduler_(s)
    {}

    //void_future start_read_op(descriptor_data& data)
    //{
        //// if shutting down, post immediate completion with error
        //// enqueue operation in fd_sets_ and if its the first operation for this
        //// descriptor, we need to interrupt the selector (because otherwise its
        //// not going to be in the fd_set used by select and we're not going to
        //// learn about any events for this descriptor)
    //}

    //void_future start_write_op(descriptor_data& data)
    //{
    //}

    void interrupt()
    {
        interrupter_.interrupt();
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

    void stop()
    {
    }
};

} // detail
} // ft

#endif
