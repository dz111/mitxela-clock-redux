PROG  = clock
MCU   = attiny3226
BUILD = build
F_CPU = 20E6
PORT ?= COM1
LANG  = c++20
BAUD ?= 230400
PGMR  = serialupdi

CC      = avr-gcc
OBJCOPY = avr-objcopy
SIZE    = avr-size
AVRDUDE = avrdude

CFLAGS = -std=$(LANG) -Wall -Os -mmcu=$(MCU) -DF_CPU=$(F_CPU)\
         -fno-exceptions -fno-rtti -fno-threadsafe-statics -MMD -MP -I.

.PHONY: main flash size clean test reset

main: $(BUILD)/$(PROG).hex size

$(BUILD)/$(PROG).elf: main.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/$(PROG).hex: $(BUILD)/$(PROG).elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

flash: $(BUILD)/$(PROG).hex size
	$(AVRDUDE) -p $(MCU) -c $(PGMR) -P $(PORT) -b $(BAUD) -U flash:w:$<:i

size: $(BUILD)/$(PROG).elf
	@echo ""
	$(SIZE) -G $<

clean:
	rm -rf $(BUILD)

-include $(BUILD)/$(PROG).d

TEST_DIR = tests
TEST_SOURCES = $(wildcard $(TEST_DIR)/test_*.cpp)
TEST_NAMES = $(basename $(notdir $(TEST_SOURCES)))

test:
	@echo "Available tests:"
	@$(foreach test,$(TEST_NAMES),echo " - $(test)";)
	@echo "\nRun 'make flash_test_name' to flash a test."

flash_%: $(BUILD)/%.hex
	$(AVRDUDE) -p $(MCU) -c $(PGMR) -P $(PORT) -b $(BAUD) -U flash:w:$<:i

$(BUILD)/%.hex: $(BUILD)/%.elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

$(BUILD)/%.elf: $(TEST_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $< -o $@

reset:
	$(AVRDUDE) -p $(MCU) -c $(PGMR) -P $(PORT) -n
