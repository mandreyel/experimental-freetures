#include "../include/freetures.hpp"
#include <iostream>

int main()
{
    ft::promise<int> p;
    ft::future<int> f = p.get_future();
    f.then([](int a) {
        return 5;
    });
}
