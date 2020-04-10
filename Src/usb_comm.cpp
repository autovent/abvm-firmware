#include "usb_comm.h"
#include "usbd_cdc_if.h"
#include <stdarg.h>

USBComm::USBComm() : 
    idxFirst(0), idxCurrent(0)
{}

bool USBComm::send(uint8_t *data, size_t len) {
    if (len > MAX_PACKET_SIZE) return false;
    return CDC_Transmit_FS(data, len) == USBD_OK;
}

bool USBComm::sendf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    char data[MAX_PACKET_SIZE];
    vsnprintf(data, sizeof(data), fmt, args);
    bool result = send((uint8_t*)data, sizeof(data));
    va_end(args);
    return result;
}

size_t USBComm::receiveLine(uint8_t **data) {
    if (idxFirst == idxCurrent) {
        *data = NULL;
        return 0;
    }

    *data = dataBuf[idxFirst].data;
    size_t size = dataBuf[idxFirst].size;

    if (++idxFirst == BUFFER_SIZE) {
        idxFirst = 0;
    }

    return size;
}

void USBComm::append(uint8_t *data, size_t len) {
    for (size_t i = 0; i < len; i++) {
        if (data[i] != '\n' && data[i] != '\r' && dataBuf[idxCurrent].size < MAX_PACKET_SIZE) {
            dataBuf[idxCurrent].data[dataBuf[idxCurrent].size++] = data[i];
        } else if (dataBuf[idxCurrent].size != 0) {
            idxCurrent++;
            dataBuf[idxCurrent].size = 0;
        }
    }
}

void USBComm::setAsCDCConsumer() {
    CDC_Set_Message_Consumer(cdcConsumer, this);
}

void USBComm::cdcConsumer(uint8_t *data, size_t len, void *arg) {
    USBComm *comm = (USBComm*)arg;
    comm->append(data, len);
}
