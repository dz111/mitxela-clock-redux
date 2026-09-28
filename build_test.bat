avr-gcc -std=c++20 -fno-exceptions -fno-rtti -fno-threadsafe-statics -Wall -O2 -mmcu=attiny3226 test.cpp -o test.elf

avr-objcopy -O ihex -R .eeprom test.elf test.hex

avrdude -v -p attiny3226 -c serialupdi -P COM7 -U flash:w:test.hex:i


