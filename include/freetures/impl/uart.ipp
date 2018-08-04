#ifndef FREETURES_UART_IPP
#define FREETURES_UART_IPP

#include "detail/reactor.hpp"

namespace ft {

uart::uart(executor& ex, const config& conf)
    : reactor_(ex.scheduler().reactor())
    , conf_(conf)
{
    uart_param_config(conf_.uart_num, &conf_.init);
    // Driver API:
    // esp_err_t uart_set_pin(
    //  uart_port_t uart_num,
    //  int tx_io_num,
    //  int rx_io_num,
    //  int rts_io_num,
    //  int cts_io_num)
    uart_set_pin(conf_.uart_num, conf_.pins.tx_pin, conf_.pins.rx_pin,
            conf_.pins.rts_pin, conf_.pins.cts_pin);
    // Driver API:
    // esp_err_t uart_driver_install(
    //  uart_port_t uart_num,
    //  int rx_buffer_size,
    //  int tx_buffer_size,
    //  int queue_size,
    //  QueueHandle_t *uart_queue,
    //  int intr_alloc_flags)
    uart_driver_install(conf_.uart_num, conf_.driver.rx_buf_size,
            conf_.driver.tx_buf_size, conf_.driver.queue_size, 0, 0);
}

future<std::string> uart::post(const std::string& msg, duration timeout = duration{0})
{
    std::error_code error;
    write(msg, error);
    // TODO 
    //return reactor_.start_write_op();
}

} // 

#endif
