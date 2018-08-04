# experimental-freetures

This repository encompasses an ongoing experiment in bringing a solely futures based Asio like framework to ESP devices running FreeRTOS (using [ESP-IDF](https://github.com/espressif/esp-idf)).

It doesn't even work currently. Thus this repository is best regarded as a place for me to collect ideas, but not to actually deliver any working software, as I still have doubts as to whether this architecture would be a good fit for programming microcontrollers. The reason it's on GitHub is so that I can access my ideas from other machines and perhaps share ideas with others.

The motivation is to hopefully reduce the complexity inherent in communicating with unerliable hardware by creating a well defined model of asynchronous execution.

## Problems

It is inspired by Asio in that it uses `select(2)` to poll for ready file descriptors. The problem with this approach is that it can only be used with UART and traditional BSD sockets, which expose a descriptor based API, but modules such as WiFi, BLE and others that don't expose such an API cannot be integrated into this framework.

I haven't yet come up with a solution, but it looks like [VFS](https://github.com/espressif/esp-idf/tree/master/components/vfs) can be used to map such a module's API to a POSIX compliant descriptor based API. Another solution would be to make scheduler aware of this and handle non-descriptor based entities in a separate thread and somehow tie it all together to expose a unified interface.

The other significant problem is managing timers, because their FreeRTOS equivalent, too, does not expose a descriptor based interface--the main reason why the ESP-IDF port of [Asio](https://github.com/espressif/esp-idf/tree/master/components/asio) is not a practical solution (because it assumes a descriptor based API for OS timers exists). I think this will have to be solved by a separate thread managed by the scheduler, but I haven't gotten that far yet.
Programming is tough.

## Example usage

demonstrating how one might send and receive a message via Lora WAN.

```c++
#include <freetures.hpp>
#include <system_error>
#include <chrono>
#include <string>

int main()
{
    ft::scheduler scheduler;
    // Set UART config...
    ft::uart::config conf;
    // Each object that wishes to provide a future-based asynchronous interface
    // takes a reference to a scheduler.
    ft::uart lora_rak(scheduler, conf);

    // None of the below commands are executed: they're just "registered" as
    // operations and they'll be executed once the scheduler is run (see below).

    // `scheduler::post` returns a future.
    scheduler.post([&scheduler] {
        // Reset the hardware by pulling down the corresponding pin.
        gpio_set_level(SOME_PIN, 0);
        // Defer the invocation of this future's continuation so that the
        // hardware has time to reset.
        return scheduler.wait(std::chrono::milliseconds(5));
    }).then([] {
        // This continuation is executed 5ms later.
        // Pull up the same pin.
        gpio_set_level(SOME_PIN, 1);
    }).then([&lora_rak] {
        // Now we are ready to use the UART channel to communicate with the
        // Lora RAK module.
        // `uart::post` also returns a future, which will eventually contain
        // the response to this command, which is passed to its `then`
        // continuation, if it exists.
        return lora_rak.post("at+join", std::chrono::seconds(2));
    }).on_error([](std::error_code error) {
        assert(error);
        // Handle RAK/UART error here.
        // The async chain is broken, i.e. no further `then` clauses are
        // invoked.
    }).on_timeout([] {
        // Handle RAK timeout here.
        // The async chain is broken, i.e. no further `then` clauses are
        // invoked.
    }).then([&lora_rak, &data](std::string response) {
        // The response to "at+join" is passed to this continuation.
        // Send next command.
        return lora_rak.post("at+send,somedata", seconds(1));
    }).then([&lora_rak](std::string response) {
        // Send empty command to listen to response
        return lora_rak.post("", std::chrono::seconds(9));
    }).then([](std::string response) {
        // Received Lora WAN response; handle it.
        // ...
    });

    // Run the scheduler, which will wait for promises associated with the above
    // futures to be fulfilled and invoke their completion handlers.
    scheduler.run();
}
```
