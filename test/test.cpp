#include "../include/freetures.hpp"
#include <iostream>

int main()
{
    ft::scheduler scheduler;
    ft::uart::config conf;
    ft::uart uart(scheduler, conf);

    ft::promise<int> p;
    ft::future<int> f = p.get_future();
    f.then([](int a) {
        return 5;
    });

    scheduler
        .wait(std::chrono::seconds(5))
        .then([] { return 5; })
        .then([](int delay) {
            std::cout << "here after " << delay << " seconds\n";
        });

    scheduler.run();
}
