#!/usr/bin/python3

import argparse
import datetime as dt
import logging
import json
import os
import requests
import RPi.GPIO as GPIO
import signal
import sqlite3
import sys
import threading
import time


# app_dir: the app's real address on the filesystem
app_dir = os.path.dirname(os.path.realpath(__file__))
app_name = 'IR Sensor Monitor'
debug_mode = False
settings_path = os.path.join(app_dir, 'settings.json')
settings = None
stop_signal = False


def prepare_table(con: sqlite3.Connection):
    cur = con.cursor()
    cur.execute('''
        CREATE TABLE IF NOT EXISTS DetectionRecords (
            timestamp   TEXT
        )
    ''')
    con.commit()
    cur.execute(f'''
        DELETE FROM DetectionRecords
        WHERE timestamp <= ?
    ''', (dt.date.today() - dt.timedelta(days=60), ))
    con.commit()


def stop_signal_handler(*args):

    global stop_signal
    stop_signal = True
    logging.info(f'Signal [{args[0]}] received, exiting')
    sys.exit(0)


def monitor_thread():

    signal_pin = settings['app']['signal_pin']
    # should be the GPIO pin connected to the OUTPUT pin of the sensor

    GPIO.setmode(GPIO.BCM)
    GPIO.setup(signal_pin, GPIO.IN, pull_up_down=GPIO.PUD_UP)

    con = sqlite3.connect(
        os.path.join(app_dir, '..', '..', 'detection-records.sqlite')
    )
    prepare_table(con)
    con.close()

    while stop_signal is False:
        sensor_state = GPIO.input(signal_pin)
        if sensor_state == 1:
            logging.info('human detected')
            for api in settings['app']['restful_apis']:
                url = api
                start = dt.datetime.now()
                r = requests.get(url=url)
                response_timestamp = dt.datetime.now()
                response_time = int((response_timestamp - start).
                                    total_seconds() * 1000)

                logging.info(f'response_text: {r.text}, '
                             f'status_code: {r.status_code}, '
                             f'response_time: {response_time}ms')

            con = sqlite3.connect(
                os.path.join(app_dir, '..', '..', 'detection-records.sqlite')
            )
            cur = con.cursor()
            cur.execute(
                'INSERT INTO DetectionRecords (timestamp) VALUES (?);',
                [dt.datetime.now()])
            con.commit()
            con.close()
            
            time.sleep(1)
        else:
            logging.debug('human NOT detected')
        time.sleep(0.1)


def main() -> None:

    global settings, debug_mode
    ap = argparse.ArgumentParser()
    ap.add_argument('--debug', dest='debug', action='store_true')
    args = vars(ap.parse_args())
    debug_mode = args['debug']

    with open(settings_path, 'r') as json_file:
        json_str = json_file.read()
        settings = json.loads(json_str)

    os.environ['REQUESTS_CA_BUNDLE'] = settings['app']['ca_path']

    signal.signal(signal.SIGINT, stop_signal_handler)
    signal.signal(signal.SIGTERM, stop_signal_handler)

    logging.basicConfig(
        filename=settings['app']['log_path'],
        level=logging.DEBUG if debug_mode else logging.INFO,
        format='%(asctime)s %(levelname)06s - %(funcName)s: %(message)s',
        datefmt='%Y%m%d-%H%M%S',
    )
    logging.info(f'{app_name} started')

    th_monitor = threading.Thread(target=monitor_thread, args=())
    th_monitor.start()


if __name__ == '__main__':

    main()
    
