import argparse
import serial
import struct
from crccheck.crc import Crc16Buypass
import time
import toml
import json

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

class CommError(Exception):
    def __init__(self, msg):
        self.msg = msg

def print_values(values, display_hex):
    if display_hex:
        values = json.loads(json.dumps(values), parse_int=lambda v: hex(int(v)))
    print(json.dumps(values, indent=2))

def read(device, args):
    values = {}
    for endpoint in args.endpoints:
        values[endpoint] = device.read(endpoint, args.timeout)
    print_values(values, args.hex)

def write(device, args):
    if len(args.endpoints) != 1:
        print('Can only write to a single ID')
        exit(1)

    if not args.values:
        print('Must specify value(s)')
        exit(1)

    device.write(args.endpoints[0], json.loads(args.values.replace('\'', '"')), args.timeout)

    print(f'Successfully wrote to "{args.endpoints[0]}"')

def stream(device, args):
    if args.interval is None and not args.monitor:
        print('Must specify either --interval or --monitor')

    if args.interval is not None:
        for endpoint in args.endpoints:
            device.set_stream_interval(endpoint, args.interval, args.timeout)
            if args.interval != 0:
                print(f'Set streaming interval for "{endpoint}" to {args.interval}ms')
            else:
                print(f'Stopped streaming for "{endpoint}"')

    if args.monitor:
        try:
            while True:
                endpoint, values = device.read_stream(args.timeout)
                if endpoint in args.endpoints:
                    display = {endpoint: values, 'timestamp': time.time()}
                    print_values(display, args.hex)
                    
        except KeyboardInterrupt:
            return


class Descriptor:
    class DescriptorError(Exception):
        def __init__(self, msg):
            self.msg = msg

    def __init__(self, descriptor_file):
        with open(descriptor_file, 'r') as f:
            self.descriptor = toml.load(f)

    def get_endpoint_descriptor(self, endpoint_name):
        try:
            return self.descriptor[endpoint_name]
        except KeyError:
            raise Descriptor.DescriptorError(f'endpoint "{endpoint_name}" has no descriptor')

    def get_endpoint_from_id(self, id):
        for endpoint, desc in self.descriptor.items():
            if desc['id'] == id:
                return endpoint

    def unpack_data(self, endpoint_name, data):
        try:
            desc = self.get_endpoint_descriptor(endpoint_name)
            unpacked = struct.unpack(desc['format'], data)

            if (len(unpacked) == 1):
                return unpacked[0]

            if 'subitems' in desc:
                if len(desc['subitems']) != len(unpacked):
                    raise Descriptor.DescriptorError(
                        f'Endpoint "{endpoint_name}" specifies {len(desc["subitems"])} subitems, expected {len(unpacked)}.'
                    )
                return dict(zip(desc['subitems'], unpacked))
            else:
                return unpacked
            
        except struct.error:
            raise Descriptor.DescriptorError(
                f'Data from "{endpoint_name}" cannot be unpacked using the specified format string.'
            )

    def pack_data(self, endpoint_name, values):
        desc = self.get_endpoint_descriptor(endpoint_name)
        
        _values = []
        if type(values) == dict:
            if 'subitems' not in desc:
                raise Descriptor.DescriptorError(
                    f'Values supplied as dictionary, but "{endpoint_name}" has no subitems'
                )
            for subitem in desc['subitems']:
                try:
                    _values.append(values[subitem])
                except KeyError:
                    raise Descriptor.DescriptorError(
                        f'Could not find subitem "{subitem}" in supplied values'
                    )
        elif type(values) == list:
            _values = values
        else:
            _values = [values]

        write_values = []
        for val in _values:
            try:
                write_values.append(int(val))
            except ValueError:
                try:
                    write_values.append(int(val, base=16))
                except ValueError:
                    raise Descriptor.DescriptorError(
                        f'Invalid datatype "{val}"'
                    )

        try:
            return struct.pack(desc['format'], *write_values)
        except struct.error:
            raise Descriptor.DescriptorError(
                f'Values for "{endpoint_name}" cannot be packed using the specified format string.'
            )


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
        if (len(msg) < 6):
            return None

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

    @staticmethod
    def calc_length(data_len):
        if data_len == 0:
            return 6
        else:
            return 7 + data_len

    def __repr__(self):
        return f'Message<Code={self.command_code}, ID={self.id}, Flags={self.flags}, Size={len(self.data)}, Data={self.data}>'
        

class Device:
    def __init__(self, serial_port, descriptor):
        self.serial_port = serial_port
        self.descriptor = descriptor

    def read(self, endpoint, timeout=5):
        """
        Read the value(s) of an endpoint.

        Args:
            endpoint: Endpoint name to read.

            timeout (optional): Maximum time to wait for response in seconds(default: 5)

        Returns:
            tuple of tuples - outer tuple contains tuples of (value, subitem) where subitem is either
            the subitem name defined in the descriptor TOML or 'endpoint_name[i]' (or just 'endpoint_name'
            if only one value is returned) if subitems are not defined
        """
        desc = self.descriptor.get_endpoint_descriptor(endpoint)

        out_msg = Message(CODE_READ, FLAG_ZERO_SIZE, desc['id'])
        self.send_msg(out_msg)

        in_msg = self.receive_msg(desc['size'], timeout)
        self.check_error_message(in_msg)
        if in_msg.id != out_msg.id or in_msg.command_code != CODE_READ_RESP:
            raise CommError(f'Unexpected response from device when reading from {endpoint}')

        return self.descriptor.unpack_data(endpoint, in_msg.data)

    def write(self, endpoint, values, timeout=5):
        """
        Write value(s) to an endpoint.

        Args:
            endpoint: Endpoint name to write to.

            values: Value(s) to send to the endpoint. Must be iterable. The number of values must
            match the number of format specifiers defined in the descriptor TOML.

            timeout (optional): Maximum time to wait for response in seconds(default: 5)
        
        Returns: None
        """
        desc = self.descriptor.get_endpoint_descriptor(endpoint)

        data_packed = self.descriptor.pack_data(endpoint, values)
        out_msg = Message(CODE_WRITE, 0, desc['id'], data_packed)
        self.send_msg(out_msg)

        in_msg = self.receive_msg(timeout)
        self.check_error_message(in_msg)
        if in_msg.id != out_msg.id or in_msg.command_code != CODE_WRITE_RESP:
            raise CommError(f'Unexpected response from device when writing to {endpoint}')

    def set_stream_interval(self, endpoint, interval, timeout=5):
        """
        Set the streaming interval of an endpoint.

        Args:
            endpoint: Endpoint to set the streaming interval of.

            interval: Streaming interval in milliseconds. If zero, streaming is stopped.

            timeout (optional): Maximum time to wait for response in seconds(default: 5)

        Return: None
        """
        desc = self.descriptor.get_endpoint_descriptor(endpoint)

        interval_packed = struct.pack('L', interval)
        out_msg = Message(CODE_STREAM, 0, desc['id'], interval_packed)
        self.send_msg(out_msg)

        msg_size = 0
        if interval != 0:
            msg_size = desc['size']

        in_msg = self.receive_msg(msg_size, timeout)
        self.check_error_message(in_msg)
        if in_msg.id != out_msg.id or in_msg.command_code != CODE_STREAM_DATA:
            raise CommError(f'Unexpected response from device when setting streaming interval for {endpoint}')

    def read_stream(self, timeout=5):
        """
        Listen for a streamed message. This will return the latest message streamed from any endpoint.

        Args:
            timeout (optional): Maximum time to wait for message in seconds (default: 5)

        Return:
            Unpacked data from streamed message - identical to read(), or None in the case of a timeout
        """        
        buf = b''
        start_time = time.time()
        while time.time() < start_time + timeout:
            recv = self.serial_port.read(1)
            buf += recv
            if len(buf) != 0 and buf[-1] == END_BYTE:
                msg = Message.load(buf)
                if msg is not None:
                    endpoint = self.descriptor.get_endpoint_from_id(msg.id)
                    return endpoint, self.descriptor.unpack_data(endpoint, msg.data)
        return None

    def receive_msg(self, size, timeout=5):
        msg = None
        start_time = time.time()
        while (msg == None) and (True if timeout == 0 else (time.time() < start_time + timeout)):
            data = self.serial_port.read(Message.calc_length(size))
            msg = Message.load(data)

        if not msg:
            raise CommError('Did not receive a message in time')

        return msg
    
    def send_msg(self, msg):
        self.serial_port.write(msg.dump())

    def check_error_message(self, msg):
        if msg.command_code == 0:
            error_text = 'ERROR_OTHER'
            if msg.id < len(ERRORS):
                error_text = ERRORS[msg.id]
            raise CommError(f'Received error code {msg.id} ({error_text})')

COMMANDS = {
    'read': read,
    'write': write,
    'stream': stream,
}

def main():
    parser = argparse.ArgumentParser(description='Serial Comm Interface')

    parser.add_argument('-p', '--port', help='Serial port', default='/dev/ttyACM1')
    parser.add_argument('descriptor', help='Descriptor TOML file for the device')
    parser.add_argument('command', choices=COMMANDS.keys(), help='Command to execute (use command -h for more information')
    parser.add_argument('endpoints', help='endpoint name(s) (only 1 allowed for write command)', nargs='+')
    parser.add_argument('-x', '--hex', help='process all command-line in/out data in hex', action='store_true')
    parser.add_argument('-v','--values', help='Value(s) to write to id (JSON; single quotes allowd)', default=None)
    parser.add_argument('-i', '--interval', type=int, help='Streaming interval in milliseconds (zero stops streaming)', default=None)
    parser.add_argument('-m', '--monitor', help='Monitor streams', action='store_true')
    parser.add_argument('-t', '--timeout', help='Communication timeout (seconds)', default=5)

    args = parser.parse_args()

    try:
        with serial.Serial(args.port, 115200, timeout=1) as ser:
            descriptor = Descriptor(args.descriptor)
            device = Device(ser, descriptor)
            COMMANDS[args.command](device, args)
    except serial.serialutil.SerialException:
        print('Error opening serial port')
        exit(1)
    except CommError as e:
        print(e.msg)
        exit(1)
    except Descriptor.DescriptorError as e:
        print(e.msg)
        exit(1)

if __name__ == '__main__':
    main()