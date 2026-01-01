#!/usr/bin/env python3

import serial
import sys
import time


# Open minitel serial connection
ser = serial.Serial(sys.argv[1], 1200, parity=serial.PARITY_EVEN, bytesize=7, timeout=None)
time.sleep(1)

# Send serial speed 1200 request
ser.write(b'\x1b\x3a\x6b\x64')
# Pas de réponse protocolaire si P_ACK_OFF_PRISE
# Response: \x1b
# Response: \x3a
# Response: \x75
# Response: \x64

# "\x1b\x3aud"
# response = ser.read(8)
# ser.write(b'\x1b\x3aud')

# Wait 8 chars or timeout and display response in hexadecimal
while True:
    response = ser.read(1)
    print("Response:", ' '.join(f'\\x{byte:02x}' for byte in response))
# end while

ser.close()
