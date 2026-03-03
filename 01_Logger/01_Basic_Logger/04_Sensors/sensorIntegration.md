# Functionality of the Sensors
## Pyroscience Pico O2
- this sensor is equiped with a UART interface
- connect it to a UART-USB-Adapter
- you can send and recive informations according to ... by using for example putty
- you can use the Pyro DeveloperTool to configurate and calibrate the device, see live values and execute measurements

## Atlas scientifig K0.1 | K1.0 | K10
- this sensor is equiped with a UART interface
- connect it to a UART-USB-Adapter
- you can send and recive informations according to ... by using for example putty
---

# Calibration
## Pyroscience Pico O2
- This sensor needs to be initalized with the values provided by the sensor cap
- We use the Pyro DeveloperTool
---
- attach the sensorcap to the pico
- connect the pico with your PC
  - you can use the provided USB-Cable
  - you can also use any other USB-to-UART interface
---
- start the pyro DeveloperTool
- klick on settings on the top left corner
- enter the code of the sensor-cap label
- select if you want to use fixed enviromental values or measured ones
  - in this project we use fixed values, because this will be set during the sensorhandling process
  - set the ambientpreasure to 1013 mBar
  - set the salinity to 0 PSU
  - set the Temperature to 20 °C
---
- The sensor is now able to measure and you should get first rough values in the diagram
------
- there are diferent ways to execute a Calibration:
    - Calibrate in ambient air
    - Calibrate in water
      - as a single point calibration
      - as a two point calibration
- As we want to use the sensor in water and both (0% and 100% saturation) are in our interest we recommend to perform a two point calibration in Water
- for 0% oxygen you can use a solution of natriumdisulfit in water
- for 100% oxygen you can use water wich was enriched with air
  - you can use a bubbler
  - you can just it from one tumbler to another with plenty of space between them (so that the air is mixed in)
---
- The calibration is similar in both points
- put the sensor in the correct fluid
- wait until you get a stable value
- Start the calibration process by clicking on "Calibration"
- choose wich point you want to calibrate
  - 0%
    - set to
  - 100%
    - set to
- This will take a few seconds
- afterwards the value is adjusted and you should have current values around your calibrationpoint
- after calibration you have to save the values to the flash memory
