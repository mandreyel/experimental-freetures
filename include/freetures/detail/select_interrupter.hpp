#ifndef FREETURES_SELECT_INTERRUPTER_HPP
#define FREETURES_SELECT_INTERRUPTER_HPP

namespace ft {
namespace detail {

class select_interrupter
{
    int read_descriptor_ = -1;
    int write_descriptor_ = -1;

public:
    select_interrupter()
    {
        // TODO open descriptors
    }

    void interrupt();
    void reset();
    int read_descriptor() { return read_descriptor_; }
};

} // detail
} // ft

#endif
