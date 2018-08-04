#ifndef FREETURES_UART_HPP
#define FREETURES_UART_HPP

#include <string>
#include <system_error>

#include "scheduler.hpp"
#include "future.hpp"
#include "detail/reactor.hpp"

//#include "driver/uart.h"

namespace ft {

/**
 * @code
 * uart::conf conf;
 * ft::executor executor;
 * ft::uart uart(executor, conf);
 * ft::error_code error;
 *
 * // Writing and reading on the UARt using the synchronous API:
 * uart.write("uart-command", error);
 * std::string response = uart.read(error);
 *
 * // And the same using the future-based asynchronous API:
 * uart.post("another-uart-command", std::chrono::seconds(2))
 *     .then([](std::string response) {
 *         // do something with result
 *     }).on_error([](ft::error_code error) {
 *         // handle error
 *     }).on_timeout([] {
 *         // handle timeout
 *     });
 *
 * @endcode
 */
struct uart
{
    /**
     * See
     * https://esp-idf.readthedocs.io/en/latest/api-reference/peripherals/uart.html
     * for config details.
     */
    struct config {
        //uart_port_t uart_num;
        //uart_config_t init;
        struct {
            int tx_pin;
            int rx_pin;
            int rts_pin;
            int cts_pin;
        } pins;
        struct {
            int rx_buf_size;
            int tx_buf_size;
            //int queue_size;
            //int interrupt_flags;
            //void* queue_handle;
        } driver;
    };

private:
    detail::reactor& reactor_;
    const config conf_;

public:
    uart(scheduler& s, const config& conf);

    /** Synchronous API. */
    void write(const std::string& msg, std::error_code error);
    std::string read(std::error_code error);

    /**
     * @brief Asynchronous UART communication API.
     *
     * Posts a command to UART and returns a future containing the future value
     * of the response, if it arrives within @p timeout amount of time. If it
     * doesn't and if it exists, the @ref on_timeout continuation of the
     * returned future is invoked.
     * Any error is reported via the @ref on_error continuation of the returned
     * future, if it exists.
     *
     * @note One must *never* have more than a single outstanding asynchronous
     * operation on a single UART channel.
     *
     * @param msg The message to post to UART.
     * @param timeout The amount of time to wait for a response before
     * concluding the command as having timed out. If it's 0, the timeout value
     * is unlimited.
     *
     * @return A future that will hold the respone to @p msg, or an empty string
     * if no response is received within the timeout window.
     */
    future<std::string> post(const std::string& msg, duration timeout = duration{0});
};

} // ft

//#include "impl/uart.ipp"

#endif
