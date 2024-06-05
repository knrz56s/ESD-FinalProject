from machine import Pin
from machine import UART

com = UART(2, 9600, tx=17, rx=16)
com.init(9600)

print("MicroPython is ready.")

com.write("hello\r\n")
msg = input("Write a message or word(under 30 characters):")
com.write(msg + '\r\n') # Need to add '\r\n' to finish a message

while True:
    if com.any() > 0:
        a = com.readline()
        print(a)