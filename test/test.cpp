#include "../include/freetures.hpp"
#include <iostream>

int main()
{
    ft::scheduler scheduler;
    ft::uart::config conf;
    ft::uart uart(scheduler, conf);

    scheduler
        .wait(std::chrono::seconds(5))
        .then([](ft::null_tag) { return 5; })
        .then([](int delay) {
            std::cout << "here after " << delay << " seconds\n";
        });

    scheduler.run();
}
