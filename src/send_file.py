import serial
import sys
import time



file_name = str(sys.argv[1])
port = str(sys.argv[2])
baud_rate = int(sys.argv[3])

ser = serial.Serial(port, baud_rate)

with open(file_name, 'r') as file:
    time.sleep(2)
    for line in file:
        ser.write(line.encode())
        time.sleep(0.1)

ser.close() 