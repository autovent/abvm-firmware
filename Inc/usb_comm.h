#ifndef USB_COMM_H
#define USB_COMM_H

#include "circular_buffer.h"
#include <string.h>

class USBComm {
public:
  USBComm();

  bool send(uint8_t *data, size_t len);

  size_t receive_line(uint8_t *data);

  bool append(uint8_t *data, size_t len);

  void setAsCDCConsumer();

template <typename... Args>
bool sendf(const char *fmt, Args... args) {
    static char data[256];
    vsnprintf(data, sizeof(data), fmt, args...);
    bool result = send((uint8_t*)data, strlen(data));
    return result;
}

private:
  static constexpr size_t BUFFER_SIZE = 16;
  static constexpr size_t MAX_PACKET_SIZE = 64;

  struct UsbData {
    size_t size;
    uint8_t data[MAX_PACKET_SIZE];
  };

  CircularBuffer<UsbData, BUFFER_SIZE> rx_buf;

  static void cdcConsumer(uint8_t *data, size_t len, void *arg);
  constexpr bool is_seperator(char x) { return x == '\n' || x == '\r'; }
};

#endif  // USB_COMM_H