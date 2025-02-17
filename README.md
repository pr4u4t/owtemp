
| Supported Targets | ESP32 | ESP32-C2 | ESP32-C3 | ESP32-C5 | ESP32-C6 | ESP32-C61 | ESP32-H2 | ESP32-P4 | ESP32-S2 | ESP32-S3 |
| ----------------- | ----- | -------- | -------- | -------- | -------- | --------- | -------- | -------- | -------- | -------- |

# DS18B20 Temperature Sensor Program

This program provides a set of commands to interact with DS18B20 temperature sensors connected via a 1-Wire bus. The commands allow you to open a bus, scan for devices, list buses and devices, read temperatures, and manage device configurations.

## How to use example

This example can be used on boards with UART and USB interfaces. The sections below explain how to set up the board and configure the example.

### Using with UART

When UART interface is used, this example can run on any commonly available Espressif development board. UART interface is enabled by default (`CONFIG_ESP_CONSOLE_UART_DEFAULT` option in menuconfig). No extra configuration is required.

### Using with USB_SERIAL_JTAG

*NOTE: We recommend to disable the secondary console output on chips with USB_SERIAL_JTAG since the secondary serial is output-only and would not be very useful when using a console application. This is why the secondary console output is deactivated per default (CONFIG_ESP_CONSOLE_SECONDARY_NONE=y)*

On chips with USB_SERIAL_JTAG peripheral, console example can be used over the USB serial port.

* First, connect the USB cable to the USB_SERIAL_JTAG interface.
* Second, run `idf.py menuconfig` and enable `CONFIG_ESP_CONSOLE_USB_SERIAL_JTAG` option.

For more details about connecting and configuring USB_SERIAL_JTAG (including pin numbers), see the IDF Programming Guide:
* [ESP32-C3 USB_SERIAL_JTAG](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c3/api-guides/usb-serial-jtag-console.html)
* [ESP32-C6 USB_SERIAL_JTAG](https://docs.espressif.com/projects/esp-idf/en/stable/esp32c6/api-guides/usb-serial-jtag-console.html)
* [ESP32-S3 USB_SERIAL_JTAG](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/usb-serial-jtag-console.html)
* [ESP32-H2 USB_SERIAL_JTAG](https://docs.espressif.com/projects/esp-idf/en/stable/esp32h2/api-guides/usb-serial-jtag-console.html)

### Using with USB CDC (USB_OTG peripheral)

USB_OTG peripheral can also provide a USB serial port which works with this example.

* First, connect the USB cable to the USB_OTG peripheral interface.
* Second, run `idf.py menuconfig` and enable `CONFIG_ESP_CONSOLE_USB_CDC` option.

For more details about connecting and configuring USB_OTG (including pin numbers), see the IDF Programming Guide:
* [ESP32-S2 USB_OTG](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s2/api-guides/usb-otg-console.html)
* [ESP32-S3 USB_OTG](https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-guides/usb-otg-console.html)

### Other configuration options

This program has an option to store the command history in Flash. This option is enabled by default.

To disable this, run `idf.py menuconfig` and disable `CONFIG_CONSOLE_STORE_HISTORY` option.

### Configure the project

```
idf.py menuconfig
```

* Enable/Disable storing command history in flash and load the history in a next example run. Linenoise line editing library provides functions to save and load
  command history. If this option is enabled, initializes a FAT filesystem and uses it to store command history.
  * `Example Configuration > Store command history in flash`

* Accept/Ignore empty lines inserted into the console. If an empty line is inserted to the console, the Console can either ignore empty lines (the example would continue), or break on empty lines (the example would stop after an empty line).
  * `Example Configuration > Ignore empty lines inserted into the console`

### Build and Flash

Build the project and flash it to the board, then run monitor tool to view serial output:

```
idf.py -p PORT flash monitor
```

(Replace PORT with the name of the serial port to use.)

(To exit the serial monitor, type ``Ctrl-]``.)

See the Getting Started Guide for full steps to configure and use ESP-IDF to build projects.

## Example Output

Enter the `help` command get a full list of all available commands. The following is a sample session of the Console Example where a variety of commands provided by the Console Example are used. Note that GPIO15 is connected to GND to remove the boot log output.

```
This is an example of ESP-IDF console component.
Type 'help' to get the list of commands.
Use UP/DOWN arrows to navigate through command history.
Press TAB when typing command name to auto-complete.
Ctrl+C will terminate the console environment.
esp32> help
help
help  [<string>]
  Print the summary of all registered commands if no arguments are given,
  otherwise print summary of given command.
      <string>  Name of command

free
  Get the current size of free heap memory

heap
  Get minimum size of free heap memory that was available during program
  execution

version
  Get version of chip and SDK

restart
  Software reset of the chip

...
...

esp32> free
257200
esp32> deep_sleep -t 1000
I (146929) cmd_system_sleep: Enabling timer wakeup, timeout=1000000us
I (619) heap_init: Initializing. RAM available for dynamic allocation:
I (620) heap_init: At 3FFAE2A0 len 00001D60 (7 KiB): DRAM
I (626) heap_init: At 3FFB7EA0 len 00028160 (160 KiB): DRAM
I (645) heap_init: At 3FFE0440 len 00003BC0 (14 KiB): D/IRAM
I (664) heap_init: At 3FFE4350 len 0001BCB0 (111 KiB): D/IRAM
I (684) heap_init: At 40093EA8 len 0000C158 (48 KiB): IRAM

This is an example of ESP-IDF console component.
Type 'help' to get the list of commands.
Use UP/DOWN arrows to navigate through command history.
Press TAB when typing command name to auto-complete.
esp32> join --timeout 10000 test_ap test_password
I (182639) connect: Connecting to 'test_ap'
I (184619) connect: Connected
esp32> free
212328
esp32> restart
I (205639) cmd_system_common: Restarting
I (616) heap_init: Initializing. RAM available for dynamic allocation:
I (617) heap_init: At 3FFAE2A0 len 00001D60 (7 KiB): DRAM
I (623) heap_init: At 3FFB7EA0 len 00028160 (160 KiB): DRAM
I (642) heap_init: At 3FFE0440 len 00003BC0 (14 KiB): D/IRAM
I (661) heap_init: At 3FFE4350 len 0001BCB0 (111 KiB): D/IRAM
I (681) heap_init: At 40093EA8 len 0000C158 (48 KiB): IRAM

This is an example of ESP-IDF console component.
Type 'help' to get the list of commands.
Use UP/DOWN arrows to navigate through command history.
Press TAB when typing command name to auto-complete.
Ctrl+C will terminate the console environment.
esp32>

```

## Troubleshooting

### Line Endings

The line endings in the Console Example are configured to match particular serial monitors. Therefore, if the following log output appears, consider using a different serial monitor (e.g. Putty for Windows) or modify the example's [UART configuration](#Configuring-UART).

```
This is an example of ESP-IDF console component.
Type 'help' to get the list of commands.
Use UP/DOWN arrows to navigate through command history.
Press TAB when typing command name to auto-complete.
Your terminal application does not support escape sequences.
Line editing and history features are disabled.
On Windows, try using Putty instead.
esp32>
```

### No USB port appears

On Windows 10, macOS, Linux, USB CDC devices do not require additional drivers to be installed.

If the USB serial port doesn't appear in the system after flashing the example, check the following:

* Check that the USB device is detected by the OS.
  VID/PID pair for ESP32-S2 is 303a:0002.

  - On Windows, check the Device Manager
  - On macOS, check USB section in the System Information utility
  - On Linux, check `lsusb` output

* If the device is not detected, check the USB cable connection (D+, D-, and ground should be connected)

## Breakdown

### Configuring UART

The ``initialize_console_library()`` function in the example configures some aspects of UART relevant to the operation of the console.

- **Line Endings**: The default line endings are configured to match those expected/generated by common serial monitor programs, such as `screen`, `minicom`, and the `esp-idf-monitor` included in the SDK. The default behavior for these commands are:
    - When 'enter' key is pressed on the keyboard, `CR` (0x13) code is sent to the serial device.
    - To move the cursor to the beginning of the next line, serial device needs to send `CR LF` (0x13 0x10) sequence.


Each time a new command line is obtained from `linenoise`, it is written into history and the history is saved into a file in flash memory. On reset, history is initialized from that file.

## Commands

### ds18b20_bus_open

Open a bus identified by a GPIO number for DS18B20 devices.

```sh
ds18b20_bus_open --bus_gpio=<int>
```

- `--bus_gpio=<int>`: Bus GPIO number.

Example:
```sh
esp32s3> ds18b20_bus_open --bus_gpio=17
Opened bus with GPIO 17 as 0
```

### ds18b20_bus_scan

Scan a bus identified by an ID for DS18B20 devices. The bus must be opened prior to scanning.

```sh
ds18b20_bus_scan --bus_id=<int>
```

- `--bus_id=<int>`: Bus ID.

Example:
```sh
esp32s3> ds18b20_bus_scan --bus_id=0
Device iterator created, start searching...
Found a DS18B20[0], address: 650458D446414F28
Searching done, 1 DS18B20 device(s) found
```
### ds18b20_bus_list

List all 1-Wire buses.

```sh
ds18b20_bus_list
```
Example:
```sh
esp32s3> ds18b20_bus_list
Bus[0]: GPIO 17
```
### ds18b20_read

Read the temperature from a DS18B20 device, this value is processed by simple low pass filter. Device must be enabled using `ds18b20_bus_device_set`

```sh
ds18b20_read --bus_id=<int> --device_id=<int>
```
- `--bus_id=<int>`: Bus ID.
- `--device_id=<int>`: Device ID.

```sh
esp32s3> ds18b20_read --bus_id=0 --device_id=0
temperature read from Bus[0].DS18B20[0]: 25.38C, time: 0ms
```

### ds18b20_bus_list_devices

List devices on a bus identified by an ID.

```sh
ds18b20_bus_list_devices --bus_id=<int>
```

- `--bus_id=<int>`: Bus ID.

Example:
```sh
esp32s3> ds18b20_bus_list_devices --bus_id=0
DS18B20[0]: address: 650458D446414F28, enabled: 0
```
### ds18b20_buses_save

Save the buses configuration across restarts.

```sh
ds18b20_buses_save
```
Example:
```sh
esp32s3> ds18b20_buses_save
```
### ds18b20_bus_device_set

Enable or disable a particular DS18B20 device on a bus. This enables reading and processing device reading in background by MeasurmentTask and ProcessingTask (low pass filter).

```sh
ds18b20_bus_device_set --bus_id=<int> --device_id=<int> --enable=<bool>
```

- `--bus_id=<int>`: Bus ID.
- `--device_id=<int>`: Device ID.
- `--enable=<bool>`: Enable or disable the device.

Example:
```sh
esp32s3> ds18b20_bus_device_set --bus_id=0 --device_id=0 --enable=true
Bus[0].DS18B20[0] enabled
```
### ds18b20_read_direct

Read the temperature directly from a DS18B20 device. **Warning** this command suspends MeasurmentTask for the time of read.

```sh
ds18b20_read_direct --bus_id=<int> --device_id=<int>
```

- `--bus_id=<int>`: Bus GPIO number.
- `--device_id=<int>`: Device number.

Example:
```sh
esp32s3> ds18b20_read_direct --bus_id=0 --device_id=0
temperature read from Bus[0].DS18B20[0]: 23.12C, time: 1460ms
```

## Usage

1. Open a bus using `ds18b20_bus_open`.
2. Scan the bus for devices using `ds18b20_bus_scan`.
3. List devices on a bus using `ds18b20_bus_list_devices`.
4. Enable device using `ds18b20_bus_device_set` to use `ds18b20_read`.
5. Read the temperature from a device using `ds18b20_read` or `ds18b20_read_direct`.
6. Save the buses configuration using `ds18b20_buses_save`.

Example:
```sh
esp32s3> ds18b20_bus_open --bus_gpio=17
Opened bus with GPIO 17 as 0
esp32s3> ds18b20_bus_scan --bus_id=0
Device iterator created, start searching...
Found a DS18B20[0], address: 650458D446414F28
Searching done, 1 DS18B20 device(s) found
esp32s3> ds18b20_bus_device_set --bus_id=0 --device_id=0 --enable=true
Bus[0].DS18B20[0] enabled
esp32s3> ds18b20_read_direct --bus_id=0 --device_id=0
temperature read from Bus[0].DS18B20[0]: 24.22C, time: 846ms
esp32s3> ds18b20_read --bus_id=0 --device_id=0
temperature read from Bus[0].DS18B20[0]: 24.15C, time: 0ms
esp32s3> ds18b20_buses_save
esp32s3> restart
I (749) cmd_ds18b20: Restoring bus identified by gpio: 17
I (749) cmd_ds18b20: Device iterator created, start searching...
I (859) cmd_ds18b20: Found a DS18B20[0], address: 650458D446414F28
I (859) cmd_ds18b20: Searching done, 1 DS18B20 device(s) found
I (859) cmd_ds18b20: Restored bus identified by gpio: 17 as 0
```

# TODO:
## 1. Testing
### Unit Tests
- Based on the Unity framework included in ESP-IDF.
- Tests individual functions, such as GPIO control, memory operations, and FreeRTOS functionality.
- Executed either on simulated hardware or directly on ESP32 using `idf.py test`.

### Integration Tests
- Verifies the interaction between different modules, such as peripheral drivers and communication stacks.
- Can be performed as hardware tests or using `pytest` with serial/TCP communication.
- Validation of peripherals: 1-Wire.

### System Tests
- Full application tests on hardware.
- Stability tests, such as detecting memory leaks (`heap_caps_get_free_size`).
- Measuring system response time (RTOS) to interrupts.
- Long-term stress testing.

## 2. CI/CD for ESP32 with FreeRTOS

### CI/CD Pipeline (GitHub Actions/GitLab CI)

#### Phase 1: Static Analysis and Code Formatting
- `clang-format` for code style enforcement.
- `cppcheck` for detecting issues.
- `idf.py lint` (for ESP-IDF).

#### Phase 2: Compilation and Unit Tests (on host)
- `idf.py build` for code validation.
- `idf.py test` to execute unit tests.
- `pytest` for Python-based testing (e.g., log analysis, communication).

#### Phase 3: Hardware Testing
- Use `pytest-embedded` to execute tests on actual ESP32 hardware.
- Automatic flashing (`idf.py flash`).
- Log collection via UART (`pytest --target esp32`).
- Long-term stability testing (stress tests).

## 3. Code improvements
- Add possibility to suspend/resume MeasurmentTask
- Allow device and bus deletion
- Add synchronization primitives around main configuration structure

## License

This project is licensed under the MIT License.
