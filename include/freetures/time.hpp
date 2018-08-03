#ifndef FREETURES_TIME_HPP
#define FREETURES_TIME_HPP

#include <chrono>

namespace ft {

using clock = std::chrono::high_resolution_clock;

using time_point = clock::time_point;
using duration = clock::duration;

using std::chrono::microseconds;
using std::chrono::milliseconds;
using std::chrono::seconds;
using std::chrono::minutes;
using std::chrono::hours;

using std::chrono::duration_cast;
using std::chrono::time_point_cast;

} // ft

#endif
