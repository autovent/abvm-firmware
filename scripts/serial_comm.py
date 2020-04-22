import argparse
import serial
import struct
from crccheck.crc import Crc16Buypass
import time

START_BYTE = 0x3F
END_BYTE = 0x0A

FLAG_ZERO_SIZE = 0x01

MAX_FRAME_SIZE = 39

CODE_ERROR = 0
CODE_READ = 1
CODE_READ_RESP = 2
CODE_WRITE = 3
CODE_WRITE_RESP = 4
CODE_STREAM = 5
CODE_STREAM_DATA = 6

ERRORS = [
    'ERROR_NONE',
    'ERROR_BAD_FRAME',
    'ERROR_ID',
    'ERROR_CRC',
    'ERROR_SIZE',
    'ERROR_WRITE',
    'ERROR_READ',
]

COMM_TIMEOUT = 1

COMMANDS = {
    'read': {
        'code': CODE_READ,
        'callback': None
    },
    'write': {
        'code': CODE_WRITE,
        'callback': None
    },
    'stream': {
        'code': CODE_STREAM,
        'callback': None
    }
}

class CommError(Exception):
    def __init__(self, msg):
        self.msg = msg

def check_error_message(msg):
    if msg.command_code == 0:
        error_text = 'ERROR_OTHER'
        if msg.id < len(ERRORS):
            error_text = ERRORS[msg.id]
        raise CommError(f'Received error code {msg.id} ({error_text})')

def read(serial_port, args):
    for id in args.id:
        out_msg = Message(COMMANDS['read']['code'], FLAG_ZERO_SIZE, int(id))
        serial_port.write(out_msg.dump())

        in_msg = None
        while in_msg == None:
            data = serial_port.readline()
            in_msg = Message.load(data)

        display = in_msg.data

        if args.format:
            try:
                display = struct.unpack(args.format, display)
            except struct.error:
                print('Invalid format string for this data')
                exit(1)

        for x, d in enumerate(display):
            if args.hex:
                print(f'{in_msg.id:03d}:{x:02d} = 0x{d:x}')
            else:
                print(f'{in_msg.id:03d}:{x:02d} = {d:d}')

        print()

def write(serial_port, args):
    if len(args.id) != 1:
        print('Can only write to a single ID')
        exit(1)

    if not args.value:
        print('Must specify value(s)')
        exit(1)

    data = args.value
    if args.hex:
        data = [int(d, base=16) for d in data]
    else:
        data = [int(d) for d in data]

    if args.format:
        data = list(struct.pack(args.format, *data))

    out_msg = Message(COMMANDS['write']['code'], 0, args.id[0], data)
    serial_port.write(out_msg.dump())

    in_msg = None
    while in_msg == None:
        data = serial_port.readline()
        in_msg = Message.load(data)

    check_error_message(in_msg)

    if in_msg.id == out_msg.id and in_msg.command_code == CODE_WRITE_RESP:
        print(f'Success')
    else:
        print(f'Error writing to ID {args.id[0]}')

def stream(serial_port, args):
    pass

COMMANDS['read']['callback'] = read
COMMANDS['write']['callback'] = write
COMMANDS['stream']['callback'] = stream

class Message:
    def __init__(self, command_code, flags, id, data=[]):
        self.command_code = int(command_code)
        self.flags = int(flags)
        self.id = int(id)
        self.data = data

    def dump(self):
        header = self.flags << 4 | self.command_code
        msg = [header, self.id]
        if (self.flags & FLAG_ZERO_SIZE) != FLAG_ZERO_SIZE:
            msg += [len(self.data), *self.data]
        msg_bytes = struct.pack('B'*len(msg), *msg)
        crc = Crc16Buypass.calc(msg_bytes)
        packed = struct.pack('B', START_BYTE) + msg_bytes + struct.pack('HB', crc, END_BYTE)
        return packed

    def check_crc(self, crc):
        return crc == struct.unpack('H', self.dump()[-3:-1])

    @classmethod
    def load(cls, msg):
        if msg[0] != START_BYTE:
            return None
        
        command = msg[1] & 0xF
        flags = (msg[1] & 0xF0) >> 4
        id = msg[2]

        data = []
        if (flags & FLAG_ZERO_SIZE) != FLAG_ZERO_SIZE:
            size = msg[3]
            data = msg[4:4+size]

        crc = struct.unpack('H', msg[-3:-1])

        ret = cls(command, flags, id, data)

        if not ret.check_crc(crc):
            raise CommError('Response invalid (bad CRC)')
        
        return ret

    def __repr__(self):
        return f'Message<Code={self.command_code}, ID={self.id}, Flags={self.flags}, Size={len(self.data)}, Data={self.data}>'
        

def main():
    parser = argparse.ArgumentParser(description='Serial Comm Interface')

    parser.add_argument('-p', '--port', help='Serial port', default='/dev/ttyACM1')
    parser.add_argument('command', choices=COMMANDS.keys(), help='Command to execute (use command -h for more information')
    parser.add_argument('id', help='Item ID(s) (can only be 1 for write command)', nargs='+')
    parser.add_argument('-f', '--format', help='(read, write) Format string to use for displaying/packing read/written data', default=None)
    parser.add_argument('-x', '--hex', help='(read, write, stream) process all commandline in/out data in hex', action='store_true')
    parser.add_argument('-v','--value', help='(write) Value(s) to write to id', nargs='*', default=None)
    parser.add_argument('-i', '--interval', help='(stream) Streaming interval')
    parser.add_argument('-m', '--monitor', help='(stream) Monitor streams', action='store_true')

    args = parser.parse_args()

    try:
        with serial.Serial(args.port, 115200, timeout=1) as ser:
            COMMANDS[args.command]['callback'](ser, args)
    except serial.serialutil.SerialException:
        print('Error opening serial port')
        exit(1)
    except CommError as e:
        print(e.msg)
        exit(1)

if __name__ == '__main__':
    main()