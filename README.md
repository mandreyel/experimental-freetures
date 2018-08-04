# experimental-freetures

This repository encompasses an ongoing experiment in bringing a solely futures based Asio like framework to ESP devices running FreeRTOS (using ESP-IDF).

It doesn't even work currently. Thus this repository is best regarded as a place for me to collect ideas, but not to actually deliver any working software, as I still have doubts as to whether this architecture would be a good fit for programming microcontrollers. The reason it's on github is so that I can access my ideas from other machines and perhaps share ideas with others.

The motivation is to hopefully reduce the complexity inherent in communicating with unerliable harwarde by creating a well defined model of asynchronous operations.

It is inspired by Asio in that it uses `select(2)` to poll for ready file descriptors. The problem with this approach is that it can only be used with UART and traditional BSD sockets, but modules such as WiFi, BLE and others that don't expose a POSIX compliant API cannot be integrated into this framework. I haven't yet come up with a solution, but it looks like VFS can be used to map such a module's API to a POSIX compliant descriptor based API. Another solution would be to make scheduler aware of this and handle non-descriptor based entities in a separate thread and somehow tie it together in an application.

## Example usage

Demonstrating how one might send and receive a message via Lora WAN.

```c++
#include <freetures.hpp>
#include <chrono>

int main {
    ft::scheduler scheduler;
    ft::uart::config conf;
    // TODO: set UART config.
    ft::uart lora_rak(scheduler, conf);
    std::string data = "...";

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
            gpi_set_level(SOME_PIN, 1);
        }).then([&lora_rak] {
            // Now we are ready to use the UART channel to communicate with the
            // Lora RAK module.
            // `uart::post` also returns a future, which will eventually contain
            // the response to this command, which is passed to its `then`
            // continuation, if it exists.
            return lora_rak.post("at+join", std::chrono::seconds(2));
        }).on_error([&lora_rak](std::error_code error) {
            assert(error);
            // Handle RAK/UART error here.
            // The async chain is broken, i.e. no further `then` clauses are
            // invoked.
        }).on_timeout([&lora_rak] {
            // Handle RAK timeout here.
            // The async chain is broken, i.e. no further `then` clauses are
            // invoked.
        }).then([&lora_rak, &data](std::string response) {
            assert(response == "OK\r\n");
            // The response to "at+join" is passed to this continuation.
            // Send next command.
            return lora_rak.post(std::string("at+send,") + data, seconds(1));
        }).then([&lora_rak](std::string response) {
            assert(response == "OK\r\n");
            // Send empty command to listen to response
            return lora_rak.post("", seconds(9));
        }).then([](std::string response) {
            // Received Lora WAN response, handle it.
            // ...
        });

    // Run the scheduler, which will actually execute ("fulfill") the above
    // created futures and invoke their continuations.
    scheduler.run();
}
```
