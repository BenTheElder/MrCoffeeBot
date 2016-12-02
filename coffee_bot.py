"""
Copyright 2016 Benjamin Elder (BenTheElder)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
"""
from Queue import Queue
from threading import Thread
from time import time


import serial
import serial.tools.list_ports


class AlreadyRunningException(Exception):
    def __init__(self):
        Exception.__init__(self, "Communication Thread is Already Running!")

def _run_thread(serial_port, state):
        # TODO: what else should we do besides exiting the thread
        # on timeout?
        if not serial_port.is_open:
            serial_port.open()
        serial_port.flushInput()
        serial_port.flushOutput()
        shutdown = False
        while not shutdown:
            message = "TEMP+?\n".encode('utf-8')
            if not state.queue.empty():
                message = state.queue.get_nowait()
                if message == "SHUTDOWN":
                    shutdown = True
                    message = "BREW+0\n".encode('utf-8')
            written = 0
            while written < len(message):
                #print(written, message)
                try:
                    curr_written = serial_port.write(message[written:])
                except serial.SerialTimeoutException:
                    print("EXCEPTION")
                    return
                written += curr_written            
            bytes, curr_byte = "", ''
            while not '\n' in bytes:
                read_bytes = serial_port.read(size=20)
                if len(read_bytes) > 0:
                    bytes += read_bytes
            state.last_response_time = time()
            reply = bytes.decode('utf-8')
            #print(reply)
            if reply.startswith("TEMP+"):
                state.current_temp = float(reply[5:len(reply)-1])
            elif reply.startswith("BREW+"):
                state.heater_on = reply[6] == '1'
            else:
                # TODO
                pass


class MrCoffeeBot(object):
    class __State(object):
        def __init__(self):
            self.queue = Queue()
            self.last_response_time = None
            self.current_temp = 0.0
            self.heater_on = False

    def __init__(self, port_path, baud=115200):
        self._port_path = port_path
        self._baud = baud
        self._serial = serial.Serial(self._port_path, self._baud)
        self._serial.timeout = 1
        self._serial.write_timeout = 1
        self._state = self.__State()
        self._thread = None

    def get_port_path(self):
        return self._port_path

    def disconnect(self):
        self._serial.close()

    def connect(self):
	if not self._serial.is_open:
        	self._serial.open()

    def run(self):
        if self.is_running():
            raise AlreadyRunningException()
        self._thread = Thread(target=_run_thread,
                              args=(self._serial, self._state))
        self._thread.daemon = True
        self._thread.start()

    def stop(self):
        self._state.queue.put("SHUTDOWN")

    def turn_on_heater(self):
        self._state.queue.put("BREW+1\n".encode('utf-8'))

    def turn_off_heater(self):
        self._state.queue.put("BREW+0\n".encode('utf-8'))

    def maybe_heater_is_on(self):
        return self._state.heater_on

    def maybe_current_temp(self):
        return self._state.current_temp

    def maybe_last_response_time(self):
        return self._state.last_response_time

    def is_running(self):
        return self._thread != None and self._thread.is_alive()


def get_possible_ports():
    devices, details = [], []
    for port in serial.tools.list_ports.comports():
        if port.manufacturer == "mbed":
            if port.vid == 3368 and port.pid == 516:
                devices.append(port.device)
                details.append((port.serial_number, port))
    return devices, details

def get_coffee_bot(serial_string=None):
    devices, details = get_possible_ports()
    for i in range(len(devices)):
        path, (ser, port) = devices[i], details[i]
        if serial_string:
            if serial_string == ser:
                return MrCoffeeBot(path)
        else:
            return MrCoffeeBot(path)
    return None

