# ABVM Serial Communication Interface

## Overview

The ABVM includes a basic transactional serial communication interface that can be accessed over USB. It allows for
easily reading and writing parameters on the device (configuration options, runtime values, etc.), as well as streaming
data from the device.

## Data Frames

All communication, with the exception of continuous data streaming, is transactional in nature. The device acts as a
slave and will respond to any commands sent to it over its serial interface. All frames sent over the serial interface
are as follows:

| Byte 0            | Byte 1 | Byte 2      | Byte 3    | Byte (4+N) | Byte (4+N)+1,(4+N)+2  | Byte (4+N)+3    |
|-------------------|--------|-------------|-----------|------------|-----------------------|-----------------|
| Start Byte (0x3F) | Header | Endpoint ID | Data Size | Data[N]    | CRC16                 | End Byte (0x3A) |

The header is as follows:

| Upper 4 bits | Lower 4 bits |
|--------------|--------------|
| Flags        | Command code |

In the case where there is no data in the message, the frame can be shortened by excluding the data size and data
fields. If this is done, the `ZERO_LENGTH` flag must be specified. Zero length frames are as follows:

| Byte 0            | Byte 1 | Byte 2      | Byte 3,4  | Byte 5          |
|-------------------|--------|-------------|-----------|-----------------|
| Start Byte (0x3F) | Header | Endpoint ID | CRC16     | End Byte (0x3A) |

All data is little-endian.

## Endpoints

The device can register as many endpoints as can be distinguished by the size field (255). These endpoints can either
use the `CommEndpoint` base class, which takes a pointer to an arbitrary piece of data and a size, allowing for very
flexible reading/writing of arbitrary.

More complicated endpoints can be created by extending `CommEndpoint` and overloading the `read` and `write` methods.
This allows for easy implementation of input validation and remote procedure calls (RPCs).

Endpoints are registered by creating an array of `CommEndpoint` pointers and passing it to the `SerialComm` constructor.

## Command codes

There are 7 command codes:

| Code | Name             | Direction   |
|------|------------------|-------------|
| 0    | MSG_ERROR        | From device |
| 1    | MSG_READ         | To device   |
| 2    | MSG_READ_RESP    | From device |
| 3    | MSG_WRITE        | To device   |
| 4    | MSG_WRITE_RESP   | From device |
| 5    | MSG_STREAM_SETUP | To device   |
| 6    | MSG_STREAM_RESP  | From device |

## Message Transactions

### Error

An error frame will be sent by the device in the case of any error. Error frames are all zero-length, and the `Endpoint 
ID` field contains the error number (instead of the usual endpoint ID). Some built-in error numbers exist:

| Error number | Error           |
|--------------|-----------------|
| 1            | ERROR_BAD_FRAME |
| 2            | ERROR_ID        |
| 3            | ERROR_CRC       |
| 4            | ERROR_SIZE      |
| 5            | ERROR_WRITE     |
| 6            | ERROR_READ      |

### Read

The read command should be sent as a zero-length frame (not required, but recommended to increase bandwidth). The device
will respond with either a read response containing the requested data or an error frame.

### Write

The write command should contain all data to be written to an endpoint. If the size does not match the endpoint's size,
the device will respond with `ERROR_SIZE`.

### Stream

The data field of the stream command should contain a 32-bit unsigned integer specifying the streaming interval (in
milliseconds). If this value is zero, streaming will be stopped. The device will immediately respond with a stream
response (which looks the same as a read response), and will continue sending stream responses at the sepcified
interval until stopped or rebooted.

Streaming can also be turned on programmaticaly using the `set_streaming` method of `CommEndpoint`. This is useful for
enabling datalogging on boot.