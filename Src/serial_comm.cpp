#include "serial_comm.h"

#include <string.h>

#include "clock.h"
#include "crc16.h"

CommEndpoint::CommEndpoint(uint8_t id, void *const data_ptr, size_t size, bool read_only)
    : id(id), size(size), data(data_ptr), read_only(read_only) {}

uint8_t CommEndpoint::write(void *data, size_t size) {
    if (size != this->size) {
        return (uint8_t)CommError::ERROR_SIZE;
    }

    if (read_only) {
        return (uint8_t)CommError::ERROR_WRITE;
    }

    memcpy(this->data, data, size);

    return (uint8_t)CommError::ERROR_NONE;
}

uint8_t CommEndpoint::read(void *data, size_t size) {
    if (size != this->size) {
        return (uint8_t)CommError::ERROR_SIZE;
    }

    memcpy(data, this->data, size);

    return (uint8_t)CommError::ERROR_NONE;
}

uint8_t CommEndpoint::stream_out(void *data, size_t size, size_t current_ms) {
    last_stream_ms = current_ms;
    return read(data, size);
}

uint8_t CommEndpoint::get_id() {
    return id;
}

size_t CommEndpoint::get_size() {
    return size;
}

void CommEndpoint::set_streaming(uint32_t interval_ms) {
    stream_interval_ms = interval_ms;
    last_stream_ms = 0;
}

bool CommEndpoint::should_stream_out(uint32_t current_ms) {
    return (stream_interval_ms != 0) && (current_ms > (last_stream_ms + stream_interval_ms));
}

SerialComm::SerialComm(CommEndpoint **endpoints, size_t num_endpoints, USBComm *uc)
    : endpoints(endpoints), num_endpoints(num_endpoints), usb(uc) {}

void SerialComm::packet_callback(uint8_t *data, size_t len, void *arg) {
    ((SerialComm *)arg)->handle_incoming_message(data, len);
}

void SerialComm::handle_incoming_message(uint8_t *data, size_t len) {
    msgFrame f;
    CommError err = mk_frame(&f, data, len);
    if (err != CommError::ERROR_NONE) {
        if (err == CommError::ERROR_BAD_FRAME) return;
        send_error_frame((uint8_t)err);
        return;
    }

    bool processed = false;
    for (size_t i = 0; i < num_endpoints; i++) {
        if (endpoints[i]->get_id() == f.id) {
            processed = true;
            switch (f.header.type) {
                case MSG_READ: {
                    msgFrame resp_frame = {
                        header : {
                            type : MSG_READ_RESP,
                            flags : 0,
                        },
                        id : f.id,
                        size : endpoints[i]->get_size(),
                    };

                    uint8_t err = endpoints[i]->read(resp_frame.data, resp_frame.size);

                    if (err) {
                        send_error_frame(err);
                        break;
                    }

                    send_frame(&resp_frame);

                    break;
                }
                case MSG_WRITE: {
                    uint8_t err = endpoints[i]->write(f.data, f.size);

                    if (err) {
                        send_error_frame(err);
                        break;
                    }

                    msgFrame resp_frame = {
                        header : {
                            type : MSG_WRITE_RESP,
                            flags : FLAG_ZERO_SIZE,
                        },
                        id : f.id,
                        size : 0,
                    };

                    send_frame(&resp_frame);

                    break;
                }
                case MSG_STREAM_SETUP: {
                    uint32_t stream_interval;
                    if (f.size == sizeof(uint32_t)) {
                        stream_interval = *(uint32_t *)f.data;
                    } else {
                        send_error_frame((uint8_t)CommError::ERROR_SIZE);
                    }

                    endpoints[i]->set_streaming(stream_interval);

                    msgFrame resp_frame = {
                        header : {
                            type : MSG_STREAM_RESP,
                            flags : 0,
                        },
                        id : f.id,
                        size : endpoints[i]->get_size(),
                    };

                    if (stream_interval == 0) {
                        resp_frame.header.flags = FLAG_ZERO_SIZE;
                        resp_frame.size = 0;
                    } else {
                        uint8_t err = endpoints[i]->stream_out(resp_frame.data, resp_frame.size, millis());
                        if (err) {
                            send_error_frame(err);
                            break;
                        }
                    }

                    send_frame(&resp_frame);

                    break;
                }
            }
            break;
        }
    }

    if (!processed) {
        send_error_frame((uint8_t)CommError::ERROR_ID);
    }
}

void SerialComm::update() {
    msgFrame stream_frame = {
        header : {
            type : MSG_STREAM_RESP,
            flags : 0,
        }
    };

    for (size_t i = 0; i < num_endpoints; i++) {
        if (endpoints[i]->should_stream_out(millis())) {
            stream_frame.id = endpoints[i]->get_id();
            stream_frame.size = endpoints[i]->get_size();

            uint8_t err = endpoints[i]->stream_out(stream_frame.data, stream_frame.size, millis());

            if (err) {
                send_error_frame(err);
            } else {
                send_frame(&stream_frame);
            }
        }
    }
}

CommError SerialComm::mk_frame(msgFrame *f, uint8_t *data, size_t size) {
    size_t data_idx = 0;

    if (data[data_idx++] != START_BYTE) {
        return CommError::ERROR_BAD_FRAME;
    }

    f->header = *(msgHeader *)&data[data_idx++];
    f->id = data[data_idx++];

    size_t frame_size;

    if (f->header.flags & FLAG_ZERO_SIZE) {
        f->size = 0;
        frame_size = 2;
    } else {
        f->size = data[data_idx++];
        memcpy(f->data, &data[data_idx], f->size);
        data_idx += f->size;
        frame_size = 3 + f->size;
    }

    f->_crc = *(uint16_t *)&data[data_idx];
    data_idx += sizeof(uint16_t);

    if (data[data_idx] != END_BYTE) {
        return CommError::ERROR_BAD_FRAME;
    }

    uint16_t crc = CRC16::calc(&data[1], frame_size);

    if (crc != f->_crc) {
        return CommError::ERROR_CRC;
    }

    return CommError::ERROR_NONE;
}

void SerialComm::send_frame(msgFrame *f) {
    size_t buf_idx = 0;

    tx_buf[buf_idx++] = START_BYTE;
    *(msgHeader *)&tx_buf[buf_idx++] = f->header;
    tx_buf[buf_idx++] = (uint8_t)f->id;

    if (!(f->header.flags & FLAG_ZERO_SIZE)) {
        tx_buf[buf_idx++] = (uint8_t)f->size;
        memcpy(&tx_buf[buf_idx], f->data, f->size);
        buf_idx += f->size;
    }

    uint16_t crc = CRC16::calc(&tx_buf[1], buf_idx - 1);
    *(uint16_t *)&tx_buf[buf_idx] = crc;
    buf_idx += sizeof(uint16_t);

    tx_buf[buf_idx++] = END_BYTE;

    usb->send(tx_buf, buf_idx);
}

void SerialComm::send_error_frame(uint8_t err) {
    msgFrame err_frame = {
        header : {
            type : MSG_ERROR,
            flags : FLAG_ZERO_SIZE,
        },
        id : err,
        size : 0,
    };

    send_frame(&err_frame);
}
