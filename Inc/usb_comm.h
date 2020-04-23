#ifndef USB_COMM_H
#define USB_COMM_H

#include <string.h>

#include "circular_buffer.h"

class USBComm {
public:
    USBComm();

    bool send(uint8_t *data, size_t len);

    size_t receive_line(uint8_t *data);

    void purge();

    bool append(uint8_t *data, size_t len);

    void set_as_cdc_consumer();

    template <typename... Args>
    bool sendf(const char *fmt, Args... args) {
        static char data[256];
        vsnprintf(data, sizeof(data), fmt, args...);
        bool result = send((uint8_t *)data, strlen(data));
        return result;
    }
    
    struct packet_handler {
        void (*callback)(uint8_t *data, size_t len, void *arg);
        void *arg;
    };

    void set_packet_handlers(packet_handler *handlers, size_t num_handlers);
    
    static constexpr size_t MAX_PACKET_SIZE = 64;

private:
    static constexpr size_t BUFFER_SIZE = 16;

    packet_handler *packet_handlers;
    size_t num_packet_handlers;

    struct UsbData {
        size_t size;
        uint8_t data[MAX_PACKET_SIZE];
    };

    CircularBuffer<UsbData, BUFFER_SIZE> rx_buf;

    static void cdcConsumer(uint8_t *data, size_t len, void *arg);
    constexpr bool is_seperator(char x) {
        return x == '\n' || x == '\r';
    }
};

#endif  // USB_COMM_H