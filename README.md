## National Rail Departure Board showing real-time departure information from any rail station within Great Britain on a 3.5 inch 320 x 480 pixel LCD display. Uses Raspberry Pi PICO 2W microprocessor.

## Overview
Designed to emulate the look of a typical station departure board (Figure 2).
Pulls data from National Rail's Darwin LDB Webservice data feed.
Departures for up to 15 services can be displayed.
The station can be selected from a user defined list(in the code) of up to 24 stations (Figure 1).
The display also shows the current time derived from NTP server.


![Fig 1 Station select](https://github.com/user-attachments/assets/802f1857-f84a-4aa2-9587-57f4bb0ed4c9)


Fig 1 Station Select  

![Fig 2 Departure display](https://github.com/user-attachments/assets/786e5795-acbf-438a-ab24-8c3502997f99)


Fig 2 Departure Display

## Hardware Requirements
Raspberry Pi Pico 2W microprocessor
Waveshare 3.5inch Pico-Res Touch 480 x 320 display https://www.waveshare.com/pico-restouch-lcd-3.5.htm
Push-button (n.o. spst) for scrolling and selecting the Station from the Station List. The button is soldered to the GPIO 01 pad and the GND pad on the reverse of the display board beneath the Pico plug-in connector as shown in Figure 3 below.



![Fig 3 Button wiring](https://github.com/user-attachments/assets/e94acb21-60b9-4bef-a041-824eb4372410)


Fig 3 Push-button Wiring

## Obtaining access to UK National Rail data feed
A user token is required to access the National Rail data feed.
To get the token: Visit: https://www.nationalrail.co.uk/100296.aspx
Scroll down to the Darwin Data Feeds table. Find the row labelled "LDB Webservice (PV)" and click register here. Follow the registration steps. Wait for a bit and you should get an email confirming your account is activated. The email also contains your token.

## Downloading and installing the Code
Download and unzip the files. The code can be found in the National-Rail-Real-time-Departure-Board-main folder. 
The code is in the DepartureBoard_PICO_V1.0 folder and consists of the main code file: DepartureBoard_PICO_V1.0.ino
and associated code files: api.ino, stationSelect.ino.
The TFT_eSPI.h graphics library requires the Free_Fonts.h file.

## Code Setup
Compiled with Arduino IDE 2.3.6 and Raspberry Pi Pico https://github.com/earlephilhower/arduino-pico v4.5.1
Required Libraries: TFT_eSPI graphics library v2.5.4 by Bodmer : https://github/bodmer.TFT_eSPI
The User_Setup file (TFT_eSPI_User_Setup.h) in the resources folder should be saved as User_Setup.h in the TFT_eSPI directory.
Button2 by Lennart Hennigs v2.3.3: https://github.com/LennartHennigs/Button2

Plus: WiFi.h; WiFiClient.h; WiFiClientSecure.h

Modify the following options within the departureboad_PICO.ino file to suit. 
WIFI_SSID, WIFI_PASSWORD, NATIONAL_RAIL_TOKEN .
The individual station codes (CRS) and Station Names need to be defined in the code for each of the station options to be available to be selected for display. Up to 24 stations can be defined. The nuber of stations in the list is defined by modifying  STATIONS_LIST in the code to the number required default (maximum)  24.

NOTE: the code uses the Arduino String() functions to extract the departure data from the XML feed. Therefore this code is not fully C++ compliant.

## Operating Instructions
Connect the power supply. While connecting to WiFi the display shows “Connecting.. “ . Once connected the list of Stations previously entered into the code will be displayed. The current selection is highlighted. short-click the button to scroll down the list and long-click to select the station required. The departures for that station will then be displayed. long-click to return to the Station List and make another selection.
Note: the number of departures displayed will be determined by the number of departures in a 2.5 hour time period from the current time and can be less than 15 when departures are infrequent.


