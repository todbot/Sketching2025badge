
# Installing firmware on Sketching2025badge board

Use any Windows, MacOS, Linux, RaspberryPi host with USB-to-serial adapter.

## Hardware prep

The Sketching2025badge board has the UPDI programming port brought out as a 4-pin
SMD header, which a 4-pin pin header can be soldered, or a 4-pin 0.1" spacing
pogo-pin jig clamp can be attached.

The pins on the TouchWheelSAO board are laid out for use with a
[USB-to-serial adapter wired as a UPDI programmer](https://learn.adafruit.com/adafruit-attiny817-seesaw/advanced-reprogramming-with-updi)
but you can also use a standard UPDI programmer too.

Here is what a programmer setup could look like, using:
* [cheap USB-to-serial dongle](https://amzn.to/47sMaxz)
* [Pogo pin probe clip](https://www.digikey.com/short/3c99n5p9)

[image tbd]

<a href="./firmware-programming1.jpg"><img src="./firmware-programming1.jpg" width=400></a>


## Software, Compiling, Uploading

### GUI IDE install

If installing with the Arduino GUI, you need to install the latest Arduino IDE 
from [arduino.cc/en/software/](https://www.arduino.cc/en/software/#ide).

The badge is based on the AVR ATtiny816 chip.  
This is supported by the [megaTInyCore](https://github.com/SpenceKonde/megaTinyCore) 
In the Arduino IDE, add this core by going to Settings / Additional Board Manager URLs and 
add `http://drazzy.com/package_drazzy.com_index.json` to the list. 
Then in Boards Manager, add "megaTinyCore".

Pick the board type in Tools / Board as "megaTinyCore" -> "ATtiny3226/3216/../816/../406"

In the Tools menu, set the chip parameters as:

- Chip: "ATtiny816"
- Clock: "10 MHz internal"
- Printf: "minimal"
- Programmer: "Serial UDPI - SLOW 57600"
- (leave everything else as default)


### Arduino-CLI install

Instead of installing the Arduino IDE, we can also use [arduino-cli](https://arduino.github.io/arduino-cli/1.0/)

#### Install arduino-cli

The [arduino-cli install page](https://arduino.github.io/arduino-cli/1.0/installation/#latest-release)
has pre-built binaries for common platforms.  And if on MacOS, you can do `brew install arduino-cli`

#### Install megatinycore 

The badge uses the ATtiny816 chip.  This is supported by the MegaTinyAVR core. 

```sh
arduino-cli core install megatinycore:megaavr  --additional-urls https://drazzy.com/package_drazzy.com_index.json
 ```

#### (optional) Show some details on the core:

```sh
arduino-cli board details -b megaTinyCore:megaavr:atxy6
```


#### Compile:

```sh
arduino-cli compile --verbose --fqbn megaTinyCore:megaavr:atxy6:chip=816,printf=minimal \
            --build-path=$(pwd)/build Sketching2025badge_attiny816
```


#### Upload

```sh
arduino-cli upload --verbose --fqbn megaTinyCore:megaavr:atxy6:chip=816  \
            --port /dev/ttyUSB0 --programmer serialupdi57k  \
            --input-dir ./build

```

#### Shell script for batch programming

Also see firmware/batch-program.sh

```sh
#!/bin/bash

PORT=/dev/ttyUSB0
HEX_FILE=./build/Sketching2025badge_attiny816.ino.hex

while [ 1 ] ; do
  read -rsn1 -p"Press any key program firmware"; echo
  arduino-cli upload --verbose --fqbn megaTinyCore:megaavr:atxy6:chip=816  \
              --port $PORT --programmer serialupdi57k  \
              --input-file=$HEX_FILE
  echo "--------------------"
done

```
