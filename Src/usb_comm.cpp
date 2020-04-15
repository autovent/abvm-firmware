#include "usb_comm.h"

#include <stdarg.h>
#include <string.h>

#include "usbd_cdc_if.h"

USBComm::USBComm() {}

bool USBComm::send(uint8_t *data, size_t len) {
    if (len > MAX_PACKET_SIZE) {
        return false;
    }
    return CDC_Transmit_FS(data, len) == USBD_OK;
}

size_t USBComm::receive_line(uint8_t *data) {
    UsbData *line = rx_buf.peek();

    if (line == nullptr) {
        data = nullptr;
        return 0;
    } else {
        memcpy(data, line->data, line->size);
        return line->size;
    }
}

bool USBComm::append(uint8_t *data, size_t len) {
    if (rx_buf.full()) {
        return false;
    }

    UsbData *buf = rx_buf.alloc();
    buf->size = 0;

    for (size_t i = 0; i < len; i++) {
        if (!is_seperator(data[i]) && buf->size < MAX_PACKET_SIZE - 1) {
            buf->data[buf->size++] = data[i];
        } else if (buf->size != 0) {
            buf->data[buf->size++] = '\0';
            buf = rx_buf.alloc();
            buf->size = 0;
        }
    }

    buf->data[buf->size++] = '\0';
    return true;
}

void USBComm::setAsCDCConsumer() {
    CDC_Set_Message_Consumer(cdcConsumer, this);
}

void USBComm::cdcConsumer(uint8_t *data, size_t len, void *arg) {
    USBComm *comm = (USBComm *)arg;
    comm->append(data, len);
}
