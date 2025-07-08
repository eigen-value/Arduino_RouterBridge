# THIS CODE IS FOR TESTING PURPOSES ONLY

import threading
import msgpack
import serial
import time
from io import BytesIO

REQUEST = 0
RESPONSE = 1
NOTIFY = 2

GENERIC_EXCEPTION = 0xff

class SerialBridge:

    def __init__(self, port, baudrate=115200):
        self.port = port
        self.baudrate = baudrate
        self.ser = serial.Serial(port, baudrate, timeout=0.1)
        self.msg_id = 0

        self.data = bytearray()

        self.callbacks = {}
        self.running = False

        self.requests_q = []
        self.response_q = []

        self.read_lock = threading.Lock()
        self.write_lock = threading.Lock()
        self.response_q_lock = threading.Lock()

    def call(self, method, *args):
        with self.write_lock:
            request = [REQUEST, self.msg_id, method, [*args]]
            self.ser.write(msgpack.packb(request))

        data = None
        while True:
            with self.response_q_lock:
                if len(self.response_q) > 0 and self.response_q[0][1] == self.msg_id:
                    data = self.response_q[0]
                    self.response_q = self.response_q[1:]
                    break
            time.sleep(0)

        return data[3]

    def notify(self, method, *args):
        request = [NOTIFY, method, [*args]]
        self.ser.write(msgpack.packb(request))

    def register_callback(self, command, func):
        """Register a callback for a specific command key"""
        self.callbacks[command] = func

    def on_request(self, msg_id, command, args):
        """Execute the callback and respond"""
        try:
            result = self.callbacks[command](*args)
            return [RESPONSE, msg_id, None, result]
        except Exception as e:
            return [RESPONSE, msg_id, [GENERIC_EXCEPTION, str(e)], None]

    def on_notify(self, command, args):
        """Execute the callback"""
        try:
            self.callbacks[command](*args)
        except Exception as e:
            print(f"Exception on notification... the client will never know {str(e)}")

    def handle_message(self, message):
        """Process incoming messages"""
        msgsize = len(message)
        if msgsize != 4 and msgsize != 3:
            raise Exception("Invalid MessagePack-RPC protocol: message = {0}".format(message))

        msgtype = message[0]
        if msgtype == REQUEST:
            # response = self.on_request(message[1], message[2], message[3])
            self.requests_q.append(message)
        elif msgtype == RESPONSE:
            # response = self.on_response(message[1], message[2], message[3])
            with self.response_q_lock:
                self.response_q.append(message)
        elif msgtype == NOTIFY:
            # self.on_notify(message[1], message[2])
            # return None
            self.requests_q.append(message)
        else:
            raise Exception("Unknown message type: type = {0}".format(msgtype))
        return

    def start(self):
        """Start the serial server loop"""
        self.running = True
        threading.Thread(target=self._run, daemon=True).start()

    def is_request(self, message):
        msgtype = message[0]
        return msgtype == REQUEST

    def is_notify(self, message):
        msgtype = message[0]
        return msgtype == NOTIFY

    def is_response(self, message):
        msgtype = message[0]
        return msgtype == RESPONSE

    def dispatch(self):
        if len(self.data) > 0:
            unpacker = msgpack.Unpacker(BytesIO(self.data))
            for message in unpacker:
                self.handle_message(message)
                self.data = self.data[len(msgpack.packb(message)):]

    def process_requests(self):
        if len(self.requests_q) == 0:
            return

        response = None

        if self.is_request(self.requests_q[0]):
            response = self.on_request(self.requests_q[0][1], self.requests_q[0][2], self.requests_q[0][3])
            self.requests_q = self.requests_q[1:]
        elif self.is_notify(self.requests_q[0]):
            self.on_notify(self.requests_q[0][1], self.requests_q[0][2])
            self.requests_q = self.requests_q[1:]

        if response:
            with self.write_lock:
                self.ser.write(msgpack.packb(response))

    def consume_first(self):
        with self.read_lock:
            unpacker = msgpack.Unpacker(BytesIO(self.data))
            for i, message in enumerate(unpacker):
                if i>0:
                    return
                if isinstance(message, list):
                    self.data = self.data[len(msgpack.packb(message)):]
                else:
                    self.data.clear()


    def _run(self):
        while self.running:
            try:
                with self.read_lock:
                    self.data.extend(self.ser.read(1024))
                    if len(self.data) > 0:
                        print(self.data)

                self.dispatch()

                self.process_requests()

            except Exception as e:
                print(f"Error: {e}")
                self.consume_first()
                #raise(e)
        print("Server stopped")

    def stop(self):
        self.running = False
        self.ser.close()