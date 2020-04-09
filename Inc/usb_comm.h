#ifndef USB_COMM_H
#define USB_COMM_H

#include "circular_buffer.h"

class USBComm {
public:
    USBComm();

    bool sendLine(uint8_t *data, size_t len);

    size_t receiveLine(uint8_t **data);

    void append(uint8_t *data, size_t len);

    void setAsCDCConsumer();
private:
    static constexpr size_t BUFFER_SIZE = 16;
    static constexpr size_t MAX_PACKET_SIZE = 64;

    struct usbData {
        size_t size;
        uint8_t data[MAX_PACKET_SIZE];
    };

    usbData dataBuf[BUFFER_SIZE];

    size_t idxFirst;
    size_t idxCurrent;

    static void cdcConsumer(uint8_t *data, size_t len, void *arg);
};

#endif  // USB_COMM_H