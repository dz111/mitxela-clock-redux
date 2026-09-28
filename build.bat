avr-gcc -std=c++20 -fno-exceptions -fno-rtti -fno-threadsafe-statics -Wall -O2 -mmcu=attiny3226 main.cpp -o clock.elf

avr-objcopy -O ihex -R .eeprom clock.elf clock.hex

avrdude -v -p attiny3226 -c serialupdi -P COM7 -U flash:w:clock.hex:i


