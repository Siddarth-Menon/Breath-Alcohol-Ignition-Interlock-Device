# Breath-Alcohol-Ignition-Interlock-Device

## Software Used

1. Keil uVision
2. Philips Flash Utility
3. Realterm (For testing)

## Hardware Used

1. LPC2148 (ARM Development Board)
2. GPRS Modem (MGSM900)
3. MQ-3 Alcohol Gas Sensor Module
4. 16x2 LCD
5. GPS Module
6. Relay Module (the one used for this project has an opto-coupler)
7. Potentiometer

## Working

- It basically works by using GSM communication and UART protocol. LPC2148 has 2 UARTS, for this project we have used UART0, but it is completely possible to use UART1 by just
modifying the code.
- The external potentiometer is used to adjust the contrast of the 16x2 LCD.
- GPS module is used to fetch the location and time.
- GPRS modem is used to send the SMS to the police station.
- Relay module is used to cut the ignition.

**Steps**
 
1. The reset button of the microcontroller board is first pressed to run the program which has been flashed. (Basically the ON button for the device) 
2. When the 16x2 LCD displays the message "Alcohol Test", the device is ready and the driver has to do the Breath Alcohol Test by blowing to the MQ-3 sensor.
3. If the concentration of alcohol present in the breath is below the threshold level (can be adjusted), the LCD displays the message "Passed", the pins of the board connected 
to the relay is then made low and the relay is activated which then connects the 2 ignition wires. Now the driver can start the ignition.
4. If the driver fails the Breath alcohol test, i.e the alcohol concentration is above the threshold then the LCD diaplays the message "Ur Drunk" and the pins of the board
connected to the relay is made high and the relay gets deactivated which then cuts the ignition by disconnecting the wires.
5. Now if the driver tries to start the car two things happen.
6. The GPS transmits the required data (GPS coordinates and time) to the LPC2148 board, the board then communicates with the GPRS modem. 
7. the GPRS modem gets activated and sends an SMS consisting of the time (UTC) and exact location (Latitude, Longitude, Altitude)  of the driver to the police station or
higher authorities so that they can take the required action.  

## Applications

Can be used by rental car companies to moniter the people using their cars by asking them to the breath alcohol test every 30 minutes while they are driving. This will reduce 
the number of accidents in the public and damage losses to the companies. It can also be used by TAXI companies to moniter their taxi drivers.
