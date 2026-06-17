# Functionality of the Sensors
## Pyroscience Pico O2
- this sensor is equiped with a UART interface
- connect it to a UART-USB-Adapter
- you can send and recive informations according to the [Comunication Protocoll](https://www.pyroscience.com/en/products/all-meters/pico-o2-sub?file=files/website_data/Downloads/Manuals/Meters/PyroScience%20Unified%20Protocol.pdf&cid=19799)
- you can use the Pyro DeveloperTool to configurate and calibrate the device, see live values and execute measurements

## Atlas scientifig K0.1 | K1.0 | K10
- this sensor is equiped with a UART interface and an I2C interface
  - we are using the UART interface
- connect it to a UART-USB-Adapter
- you can send and recive informations according to [EZO ec datasheet](https://files.atlas-scientific.com/EZO-EC-complete-datasheet.pdf)

## BlueRobotics TSYS01
- this sensor is equiped with a I2C bus
- the sensor is the slave, so you need to connected it to an existing I2C Bus
- you can send and recive informations according to the [datasheet](https://www.te.com/commerce/DocumentDelivery/DDEController?Action=showdoc&DocId=Data+Sheet%7FTSYS01%7FA%7Fpdf%7FEnglish%7FENG_DS_TSYS01_A.pdf%7FG-NICO-018)

## Keller Series 20

---

# Interfaces used for communication
## UART
- We are using the PmodUSBUART as Interface
- you can find it on [reichelt](https://www.reichelt.de/de/de/shop/produkt/pmod_usb-uart_usb-zu-uart-schnittstelle-243339)
- the manual is also available as [pdf at reichelt](https://cdn-reichelt.de/documents/datenblatt/B300/DIGILIENT_PMODUSBUART_ENG_TDS.pdf)
- there is no driver necessary when using Win10 or Win11
- after connecting to the PC check for the associated com port at the device monitor
- using a serial terminal (e.g. [Putty](https://putty.org/index.html) to establish a connection
- check the commands at the reference manual of the sensor (see above)

## I2C
- We are using the [WCH341A Chip](https://wch-ic.com/products/CH341.html), provided on a [platine by berrybase](https://www.berrybase.de/usb-i2c-iic-spi-uart-ttl-isp-all-in-one-konverter-mit-ch341a-chipsatz)
- this interface has multiple capabilitys (I2C, UART, ...)
- You need to install a driver on windows, provided bei the manufactor [WCH](https://www.wch-ic.com)
  - for UART install the [serial driver](https://www.wch-ic.com/downloads/CH341SER_EXE.html) (the rest works as described above)
  - for I2C install the [parallel driver](https://www.wch-ic.com/downloads/CH341PAR_EXE.html)
- There is a tool (no installation needed) for communication dedicated to this Chipset, called [CH341A_tool](https://github.com/tomek-o/CH341A-tool)
- You can search for connected participans on I2C and read/write on the bus
- check the reference manual of the sensor for commands to be used (see above)

---

# Commissioning and calibration
## Pyroscience Pico O2
- This sensor needs to be initalized with the values provided by the sensor cap
- We use the Pyro DeveloperTool (provided by PyroScience)
<br><br>
- attach the sensorcap to the pico
- connect the pico with your PC
  - you can use the provided USB-Cable
  - you can also use any other USB-to-UART interface

- start the pyro DeveloperTool
- klick on settings on the top left corner
- enter the code of the sensor-cap label
- select if you want to use fixed enviromental values or measured ones
  - in this project we use fixed values, because this will be set during the sensorhandling process
  - set the ambientpreasure to 1013 mBar
  - set the salinity to 0 PSU
  - set the Temperature to 20 °C
<br><br>**TODO: images**<br><br>
- The sensor is now able to measure and you should get first rough values in the diagram
<br><br>**TODO: images**<br><br>
- there are diferent ways to execute a **Calibration**:
    - Calibrate in ambient air
    - Calibrate in water
      - as a single point calibration
      - as a two point calibration
<br><br>
- As we want to use the sensor in water and both (0% and 100% saturation) are in our interest we recommend to perform a two point calibration in Water
- for 0% oxygen you can use a solution of natriumdisulfit in water
- for 100% oxygen you can use water wich was enriched with air
  - you can use a bubbler
  - you can just it from one tumbler to another with plenty of space between them (so that the air is mixed in)
<br><br>
- The calibration is similar in both points
- put the sensor in the correct fluid
- wait until you get a stable value (check at the live diagram)
<br><br>**TODO: images**<br><br>
- Start the calibration process by clicking on "Calibration"
- choose wich point you want to calibrate
  - 0%
    - set to **TODO: check and doc correct settings**
  - 100%
    - set to **TODO: check and doc correct settings**
- This will take a few seconds
- afterwards the value is adjusted and you should have current values around your calibrationpoint
<br><br>**TODO: images**<br><br>
- after calibration you have to save the values to the flash memory
- <br><br>**TODO: images**<br><br>

## Atlas scientifig K0.1 | K1.0 | K10
## BlueRobotics TSYS01
## Keller Series 20
