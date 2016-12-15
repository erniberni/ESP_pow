# ESP_pow
Open software for reflashing the SONOFF_POW

The SONOFF products of ITEAD are well documented. The hardware is based on ESP8266. This programm shows the use of the power measuring circuit of the SONOF_POW. The software ist based on the HLW8012 library by Xose Pérez https://bitbucket.org/xoseperez/hlw8012

The ESP_pow sketch is build on top of the BasicOTA sketch for flashing over-the-air which is really helpful when working with circuits that are powered from mains. For the first program upload you need of course a USB-serial-adapter.

!!! DON'T CONNECT TO MAINS WHEN CONNECTED WITH THE USB-SERIAL-ADAPTER !!!

The program can run in 2 different modes: 
- when you press the button while the circuit is connecting to Wifi (the blue LED is blinking), the program changes to calibration mode
- in normal mode the program measures the voltage and current at the output and calculates the power. Measured values are written to a TCP client in the same network. I recommend this very simple nodejs TCP client in this repository. You have to start the client before starting this program.

Add in the program your Wifi SSID and password and the IP adress of the TCP client.

The measuring routines are interrupt based but the calibration routine works in non interrupt mode because calibration didn't work in interrupt mode.
For calibrating you need a well known (pure resistive) load, for example a light bulb with 60W. Better you also use a power measuring device showing voltage and power. You should measure voltage and power and fill the values in the measuring routine. While calibrating you will get new current, voltage and power multiplier that have to be exchanged in the code.   
