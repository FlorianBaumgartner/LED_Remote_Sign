# LED_Remote_Sign
Hardware and Firmware of an ESP32-C3 bases LED Matrix, that can display messages sent over internet.


# Use this command to read complete flash memory:

esptool.py -b 921600 read_flash 0 0x400000 flash_dump.bin
