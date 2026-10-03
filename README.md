## Building

Set the port that the MCU programmer is on:

```
$ export PORT = /dev/ttyUSB0          # Linux
$ export PORT = /dev/cu.usbserial-10  # MacOS
> set PORT = COM3                     # Windows
```

For CH340, it may be possible to do this:

```
$ export PORT = ch340
```

Now we can build and flash:

```
$ make
$ make flash
```

If this fails, it may be necessary to lower the baud rate:

```
$ export BAUD = 115200
```

## Tests

A list of tests is available from:

```
$ make test
Available test:
 - test_first
```

Build and flash a test by 'making' its name with the flash_ prefix:

```
$ make flash_test_first
```

Connect to the interactive prompt using updi_console.py. This depends
on pyserial.

```
$ python3 updi_console.py /dev/cu.serialusb-10
>>>  Exit: ^E+e  Reset: ^E+r  ^E: ^E+^E
[L]ist tests  [R]un tests
```

Press L to list the tests or R to run the tests. Press Ctrl+E,
then E to exit the console.
