#ifndef SERIAL_CONFIG_H
#define SERIAL_CONFIG_H

#include <stddef.h>

#include "usb_comm.h"

enum class CommError : uint8_t {
    ERROR_NONE = 0,
    ERROR_BAD_FRAME,
    ERROR_ID,
    ERROR_CRC,
    ERROR_SIZE,
    ERROR_WRITE,
    ERROR_READ,
};

/**
 * @brief  CommEndpoint class - handles getting and setting config options
 * @note   The write() and read() methods can be overwritten to implement input validation and remote procedure calls.
 */
class CommEndpoint {
public:
    explicit CommEndpoint(uint8_t id, void *const data_ptr, size_t size, bool read_only=false);

    virtual uint8_t write(void *data, size_t size);
    virtual uint8_t read(void *data, size_t size);

    uint8_t stream_out(void *data, size_t size, size_t current_ms);

    uint8_t get_id();
    size_t get_size();

    void set_streaming(uint32_t interval_ms);
    bool should_stream_out(uint32_t current_ms);

protected:
    uint8_t id;
    size_t size;
    void *const data;
    uint32_t stream_interval_ms;
    uint32_t last_stream_ms;
    bool read_only;
};

class SerialComm {
public:
    SerialComm(CommEndpoint **endpoints, size_t num_endpoints, USBComm *uc);

    void update();

    static void packet_callback(uint8_t *data, size_t len, void *arg);

private:
    static constexpr uint8_t MAX_FRAME_DATA_SIZE = 32;

    static constexpr uint8_t START_BYTE = 0x3F;
    static constexpr uint8_t END_BYTE = 0x0A;

    static constexpr uint8_t FLAG_ZERO_SIZE = 0x01;

    CommEndpoint **endpoints;
    size_t num_endpoints;
    USBComm *usb;

    uint8_t tx_buf[USBComm::MAX_PACKET_SIZE];

    enum messageType {
        MSG_ERROR = 0,
        MSG_READ,
        MSG_READ_RESP,
        MSG_WRITE,
        MSG_WRITE_RESP,
        MSG_STREAM_SETUP,
        MSG_STREAM_RESP,
    };

    struct __attribute__((__packed__)) msgHeader {
        uint8_t type : 4;
        uint8_t flags : 4;
    };

    struct __attribute__((__packed__)) msgFrame {
        msgHeader header;
        uint8_t id;
        uint8_t size;
        uint8_t data[MAX_FRAME_DATA_SIZE];
        uint16_t _crc;
    };

    CommError mk_frame(msgFrame *f, uint8_t *data, size_t size);

    void send_frame(msgFrame *f);

    void send_error_frame(uint8_t err);

    void handle_incoming_message(uint8_t *data, size_t len);
};

#endif  // SERIAL_CONFIG_H
