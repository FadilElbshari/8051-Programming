# 8051 Microcontroller Serial Programmer using Arduino

This project allows you to program an **8051 microcontroller** using an **Arduino** as a programmer and a Python script to transmit `.hex` files over a serial connection.

---

## 📋 Features

- Program 8051 microcontrollers via serial interface.
- Use Arduino as a bridge programmer.
- Automatically erases and flashes the 8051 ROM.
- Supports most 8051-compatible chips (e.g., AT89S52, AT89C51).

---

## 🔧 Requirements

- Python 3.x
- Arduino board (Uno, Nano, Mega, etc.)
- 8051 microcontroller with ISP support
- HEX file compiled for 8051
- USB connection between Arduino and PC
- Serial connection between Arduino and 8051
- Python `pyserial` library

---

## 📦 Installation

### 1. Install Python

Download and install Python from [https://www.python.org](https://www.python.org).

### 2. Install PySerial

Install the required serial library using pip:

```bash
pip install pyserial
```

---

## Usage
1. Upload "Programming-8051.ino" to the arduino.
2. Send hex file serially by the following:
   ```bash
    python send_file.py (dir/hexfile.hex) (Serial Port i.e. COM3) (Baud Rate: 9600)
   ```
4. An actuall command would look like this:
   ```bash
   python send_file.py hexfile.hex COM3 9600
   ```
6. After receieving the hex file, the arduino will erase the 8051 chip and flash the newly sent code into the 8051 ROM.

---

## References
- AT89S52 Datasheet: [https://ww1.microchip.com/downloads/en/DeviceDoc/doc1919.pdf](https://ww1.microchip.com/downloads/en/DeviceDoc/doc1919.pdf)
- Pyserial: [https://pyserial.readthedocs.io/en/latest/](https://pyserial.readthedocs.io/en/latest/)
