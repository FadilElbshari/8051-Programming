# 8051 Programming:
An 8051 Microcontroller serial programmer using arduino.

# Working Steps:
1. Ensure python is installed on your system to be able to serially transmit hex files to the arduino.
2. Install the pyserial library using: pip install pyserial, not serial.
3. Upload code to the arduino, so it awaits the hex file serially.
4. Run the python script as the following: python send_file.py (dir/hexfile.hex) (Serial Port i.e. COM3) (Baud Rate: 9600).
5. An actuall command would look like this: python send_file.py hexfile.hex COM3 9600.
6. After receieving the hex file, the arduino will erase the 8051 chip and flash the newly sent code into the 8051 ROM.
