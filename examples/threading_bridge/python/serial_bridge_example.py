import threading
import random
import time
from serial_bridge import SerialBridge

PORT = '/dev/ttyUSB0'

bridge = SerialBridge(port=PORT, baudrate=115200)

def call_lengthy_op():
    print("calling lengthyOp...")
    res = bridge.call("lengthyOp")
    print(f"lengthyOp result: {res}")
    time.sleep(1)


def get_rand():
    """
    Used for testing parameter-less RPCs
    """
    r = random.randint(0, 10)
    print(f"returning a random integer: {r}")
    return r


def dummy_reset():
    print("reset called")
    return True


def dummy_register(name):
    print(f"register called: {name}")
    return True


bridge.register_callback("get_rand", get_rand)
bridge.register_callback("$/register", dummy_register)
bridge.register_callback("$/reset", dummy_reset)


client_thread = threading.Thread(target=call_lengthy_op)

bridge.start()

time.sleep(15)
client_thread.start()

while True:
    time.sleep(1)