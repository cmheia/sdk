#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# W600 flash updater by cmheia

import configparser
import logging
import os
import pyprind
import serial
import sys
import time

import serial.tools.list_ports as list_ports

from xmodem import XMODEM1k

DEFAULT_BAUD = 2000000
INIT_BAUD = 115200
INIT_TIMEOUT = 1

BAUD_SET_CMD = {
    2000000: bytes.fromhex('210a00ef2a3100000080841e00'),
    1000000: bytes.fromhex('210a005e3d3100000040420f00'),
    921600: bytes.fromhex('210a005d503100000000100e00'),
    460800: bytes.fromhex('210a0007003100000000080700'),
    115200: bytes.fromhex('210a00974b3100000000c20100'),
}


def valid_speed(baud=DEFAULT_BAUD):
    if baud in BAUD_SET_CMD:
        return True
    else:
        return False


def is_serial_avaliable(port):
    ports = [p[0] for p in list(list_ports.comports())]
    if port in ports:
        return True
    else:
        return False


class config_helper(object):
    def __init__(self, file='downloader.ini'):
        self._file = file
        self._cfg = configparser.ConfigParser()

    def load(self):
        self._cfg.read(self._file)
        if len(self._cfg.sections()) is 0:
            self._cfg['base'] = {}
        return self._cfg

    def save(self):
        with open(self._file, 'w') as f:
            print('save cfg: {}'.format(self._file))
            self._cfg.write(f)

    def ref(self):
        return self._cfg


class w600dl(object):
    def __init__(self, image='', port='', baud=0):
        self._log = logging.getLogger(self.__class__.__name__)
        logging.basicConfig(level=logging.INFO)
        self._image = image
        if os.path.isfile(image):
            save_conf = False

            self._total_packets = int(
                (os.stat(self._image).st_size + 1023) / 1024)

            self._progbar = pyprind.ProgBar(self._total_packets)

            self._cfg = config_helper(
                '{}.ini'.format(os.path.splitext(__file__)[0]))
            self._config = self._cfg.load()
            if not 'base' in self._config:
                self._config['base'] = {}
                save_conf = True

            if is_serial_avaliable(port):
                if (not 'port' in self._config['base']) or self._config['base']['port'] != port:
                    self._config['base']['port'] = port
                    save_conf = True
            else:
                if 'port' in self._config['base']:
                    port = self._config['base']['port']
                    self._log.warning('conf port is {}'.format(port))
                else:
                    self._config['base']['port'] = port
                    save_conf = True

            if not is_serial_avaliable(port):
                raise RuntimeError('port {} NOT exist'.format(port))

            if not valid_speed(baud):
                if 'baud' in self._config['base'] and valid_speed(int(self._config['base']['baud'])):
                    baud = int(self._config['base']['baud'])
                    self._log.warning('conf baud is {}'.format(baud))
                else:
                    baud = DEFAULT_BAUD
                    self._log.warning('default baud is {}'.format(baud))
                    self._config['base']['baud'] = int(baud)
                    save_conf = True

            self._port = port
            self._baud = int(baud)

            self._log.warning(
                'comm {} with {}, {}'.format(port, self._baud, INIT_TIMEOUT))
            self._ser = serial.Serial(
                port=port, baudrate=INIT_BAUD, timeout=INIT_TIMEOUT)

            if save_conf:
                self._cfg.save()
        else:
            self._port = port
            self._baud = 0

    def set_baudrate(self, baud):
        self._ser.baudrate = baud

    def read(self, size=1, timeout=1):
        return self._ser.read(size)

    def readline(self):
        return self._ser.readline()

    def write(self, data, timeout=1):
        return self._ser.write(data)

    def set_reset(self, reset=1):
        if reset:
            self._ser.setRTS(1)
            self._ser.reset_output_buffer()
        else:
            self._ser.setRTS(0)

    def send_cmd(self, cmd):
        cmd_list = {
            'enter_load_mode': b'\x1b',
            'get_loader_version': b'\x56',
            'load_firmware': b'\x58',
        }
        if cmd in cmd_list:
            self.write(cmd_list[cmd])

    def enter_load_mode(self):
        self.send_cmd('enter_load_mode')

    def get_loder_version(self):
        self.send_cmd('get_loader_version')
        return self.readline()

    def set_speed(self, baud=DEFAULT_BAUD):
        if valid_speed(baud):
            self._log.warning('set speed to {}'.format(baud))
            self.write(BAUD_SET_CMD[baud])
            return True
        else:
            self._log.error('NOT supported baud {}'.format(baud))
            return False

    def set_timeout(self, timeout):
        self._ser.timeout = timeout

    def open(self):
        self._ser.open()

    def close(self):
        self._ser.flush()
        self._ser.flushInput()
        self._ser.close()

    def info(self):
        return (self._port, self._image)

    def reset_device(self):
        self._log.warning('reset device')
        self.set_reset()
        time.sleep(0.05)
        self.set_reset(0)
        self.set_timeout(0.2)

    def _check_baud(self, baud=DEFAULT_BAUD, retry=16):
        self._log.warning('check baud')
        checked = False
        while 0 < retry:
            c = self.read(1)
            retry -= 1
            if c == b'C':
                checked = True
                self._log.warning('baud checked')
                time.sleep(0.2)
                break
            else:
                self._log.warning('reopen serial')
                self.close()
                self.set_baudrate(INIT_BAUD)
                self.open()
                time.sleep(0.2)
                self.set_speed(baud)
                self.close()
                self.set_baudrate(baud)
                self.open()
                time.sleep(0.2)
        if not checked:
            self._log.error('baud check failed')
        return checked

    def sync_to_download(self, retry=16 + len('secboot running V3.1...')):
        self._log.warning('sync to download')
        if self._baud is 0:
            raise RuntimeError('Image file NOT exist')
        state = 0
        synced = False
        c_count = 0
        c_retry = retry
        while synced is False and 0 < c_retry:
            time.sleep(0.02)
            if state is 0:  # start
                self.enter_load_mode()
                self.reset_device()
                state = 1
                c_count = 0
            elif state is 1:  # CCC
                self.enter_load_mode()
                c = self.read(1)
                if c == b'C':
                    c_count += 1
                    if 2 < c_count:
                        c_count = 0
                        state = 3
                        self._log.warning('synced')
                elif c == b'P':
                    c_count = 0
                else:
                    self._ser.reset_output_buffer()
            elif state is 2:  # PPP
                self.enter_load_mode()
            elif state is 3:  # change baud
                if self.set_speed(self._baud) is True:
                    self._log.warning('change baud to {}'.format(self._baud))
                    time.sleep(0.5)
                    self.set_baudrate(self._baud)
                    # synced = self._check_baud(self._baud, retry)
                synced = True
            c_retry -= 1
        if not synced:
            self._log.error('sync failed')
        return synced

    def download(self):
        try:
            with open(self._image, 'rb') as f:
                def send_callback(total_packets, success_count, error_count):
                    # self._log.warning('total_packets:{}, success_count:{}, error_count:{}'.format(
                    #     total_packets, success_count, error_count))
                    self._progbar.update()

                self.set_timeout(1)
                time.sleep(0.2)
                modem = XMODEM1k(self.read, self.write)
                self._log.warning(
                    'downloading ... ({} packets)'.format(self._total_packets))
                result = modem.send(f, callback=send_callback)
                time.sleep(1)
                self._log.warning('')
                if result:
                    self._log.warning('done')
                else:
                    self._log.error('fail!')
        except Exception as e:
            self._log.error('error: {}'.format(e))


def help():
    help_msg = '''
USAGE:
    python {0} [COM] <image_file>
    eg: {0} COM6 w600_gz.img
    eg: {0} w600_gz.img
    '''
    print(help_msg.format(os.path.basename(__file__)))


def main(argv):
    argc = len(argv)
    print(argv)
    print(argc)
    if argc == 4:
        dl = w600dl(image=argv[1], port=argv[2], baud=int(argv[3]))
    elif argc == 3:
        dl = w600dl(image=argv[1], port=argv[2])
    elif argc == 2:
        dl = w600dl(image=argv[1])
    else:
        help()
        return

    print('')
    # print('serial: %s, image: %s' % dl.info())
    print('serial: {}, image: {}'.format(*dl.info()))
    if dl.sync_to_download() is True:
        dl.download()

    dl.close()


if __name__ == '__main__':
    main(sys.argv)
