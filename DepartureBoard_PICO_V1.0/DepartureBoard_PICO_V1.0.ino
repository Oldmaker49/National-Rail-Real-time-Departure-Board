
/* 

FILE: DepartureBoard_PICO_V1.0
By: Oldmaker49
June 2025
MIT License
Copyright (c) 2025 Oldmaker49
*/

#include <string>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <TFT_eSPI.h> 
#include <SPI.h>
#include <User_Setup.h> // Setup for the Pico-ResTouch-LCD-3.5 display: Use Resources/TFT_eSPI_User_Setup.h
#include "Free_Fonts.h"
#include <time.h>
#include <Button2.h>


// WIFI CREDENTIALS
#define WIFI_SSID "YOUR SSID"
#define WIFI_PASSWORD "YOUR PASSWORD"


// NATIONAL RAIL  DETAILS
#define NATIONAL_RAIL_TOKEN "nnnnnnnn-nnnn-nnnn-nnnn-nnnnnnnnnnnn" // National Rail Darwin Token
//#define NATIONAL_RAIL_CRS "BHM" // Station code: https://www.nationalrail.co.uk/stations_destinations/48541.aspx
#define NATIONAL_RAIL_ROWS 15 // DO NOT CHANGE THIS!! maximum number of departures to be displayed 
#define UPDATE_INTERVAL 30000 // Data update interval 30s

//defines for station select functions
#define STATIONS_LIST 24  // 24 is the maximum. This can be changed but ensure that the const String stationCRS[STATIONS_LIST] array has the correct number of entries
#define BUTTON_PIN 1 // GPIO pin of the button used to select and scroll the list of stations

unsigned long api_previous_check = -50000; // Force update immediately
int apiRetries = 0;
//int services_total = 0; // Total number of services returned
//int service_rounds = 0; // Keep count of how many loops a row has been shown
//int service_display = 1; // service_display, which service to display in the bottom row
int departureRows = NATIONAL_RAIL_ROWS;

//LCD Display constants
//WaveShre PICO touch 320 x 480 pixels - potrait orientation
// Plat Destination  Plat  Exp
int PlatHeader_Xpos = 220;
int Time_Xpos = 0;  // X position of start of Time columb
int Dest_Xpos = 50; // X position of Start of Destination column
int Plat_Xpos = 240; // X position of start of Platform column
int Exp_Xpos  = 260; // X position of start of Expected Column
int Disp_Width = 320;

//XML tags used to separate of the data to display

const String servicesTag = "<lt8:trainServices>";//length 19 - Start of data 
const String serviceTag = "<lt8:service>";//length 13 - start of each train service data
const String stdTag = "<lt4:std>"; //length 9 - start of the sheduled departure time for a service
const String endStdTag = "</lt4:std>";//length 10 - end of 
const String etdTag = "<lt4:etd>"; //length 9
const String endEtdTag = "</lt4:etd>";//length 10
const String destTag = "<lt5:destination><lt4:location><lt4:locationName>"; // length  49
const String endDestTag = "</lt4:locationName><lt4:crs>";// length 28
const String platTag = "<lt4:platform>";//length 14
const String endPlatTag = "</lt4:platform>";//length 15 
const String endOfServiceTag = "</lt8:service>";
const String endOfTrainServicesTag = "</lt8:trainServices>";//length 20
const String endSoapEnvelopeTag = "</soap:Envelope>";
const String crsTag = "<lt4:crs>"; //length 9
const String endOfCRSTag  = "</lt4:crs>"; //length 10

// Strings for the departure data returned
String  depTime[NATIONAL_RAIL_ROWS]; // scheduled departure time
String  depDest[NATIONAL_RAIL_ROWS]; // departure destination
String  depPlat[NATIONAL_RAIL_ROWS]; // platform name or number - can be blank
String  depExp[NATIONAL_RAIL_ROWS];  // expected departure - "on-time", "delayed", "cancelled", or hh:mm

String XMLresponse;// contians only information related to train services derived fromm fullXMLresponse
String fullXMLresponse;// response from National Rail Web Server
String xmlCRS; // the station code for the data returned 
bool forceUpdate;// 1 if data update required immediately
//Time variables
struct tm timeinfo; // structure to hold the time iformation obtained from NTP server
char timeNow[6];
const char* TZ_UK = "GMT0BST,M3.5.0/1,M10.5.0";// rule for UK timezone GMT/BST adjustment
time_t t; // current UNIX time


WiFiClientSecure client;

TFT_eSPI tft = TFT_eSPI(); 

Button2 button;// invoke the button functions from button2.h

//variables for station select functions
bool firstTime;

int stationIndex = 0;//pointer to current position in the stations list

// STATION LIST ////////////
// define station names and associated CRS code
// the number of entries defined must match the value of STATIONS_LIST (default 24)
const String stationCRS[STATIONS_LIST] =  {"BTH",     "BRI",                  "BHM",                  "CDF",            "CRE",  "EDB",      "EXD",             "GLC",            "LIV",              "EUS",          "PAD",              "STP",              "WAT",            "MAN",                  "NCL",      "NRW",     "NOT",       "OXF",   "RDG",    "RUG",  "SAL",      "SHF",      "SHR",       "YRK"};
const String stationName[STATIONS_LIST] = {"Bath Spa","Bristol Temple Meads", "Birmingham New Street","Cardiff Central","Crewe","Edinburgh","Exeter St Davids","Glasgow Central","Liverpool Lime St","London Euston","London Paddington","London St Pancras","London Waterloo","Manchester Piccadilly","Newcastle","Norwich","Nottingham","Oxford","Reading","Rugby","Salisbury","Sheffield","Shrewsbury","York"};

int listPosition = 0;// the current position in the stations list
String stringStationCRS;// used within the message to obtain the real-time data for that station from the Nationl Rail web server
String stringStationName;// string containg the name of the station selected
bool triggerDisplayStations;// allows the station list to be displayed once
bool triggerDisplayDepartureHeadings;// allows the fixed departure board headings to be displayed once
int mode = 0; // 0  = display station list, 1 = save station and display departures 
bool triggerGetData;
//
// SUROUTINES ////////////////////

// Routine to determine the number of services to display. The number of services can be less than [NATIONAL_RAIL_ROWS]
// This can happen for stations with infrequent services or during the night when there are fewer departures
int getDepartureRows(){
  int indexEndOfData = XMLresponse.length();
  int indexServiceTag;
  int numberOfServices = 0;
  int searchPosition = 0;
  int previousSearchPosition = 0;
  while (searchPosition < indexEndOfData){
    indexServiceTag =  XMLresponse.indexOf(serviceTag, searchPosition);
    searchPosition = indexServiceTag + 13;
    if(searchPosition > previousSearchPosition ){
     numberOfServices++;
     previousSearchPosition = searchPosition;
    }
    else break;
  }
    if (numberOfServices < NATIONAL_RAIL_ROWS){
        return numberOfServices;
    }
    else return NATIONAL_RAIL_ROWS;
   //Serial.print("number of Services: "); Serial.println(numberOfServices);
} 
void getCRS(){ // get the station CRS code for the data returned
  long int crsPosition = fullXMLresponse.indexOf(crsTag) + 9;
  long int endCRSposition =  fullXMLresponse.indexOf(endOfCRSTag);
  xmlCRS = fullXMLresponse.substring(crsPosition,endCRSposition);
}
void getSTD(){ // gets the sheduled departure time for each service
   long int stdPosition = XMLresponse.indexOf(servicesTag);
   int i;
  for( i = 0 ; i < departureRows; i++){
    long int indexStd = XMLresponse.indexOf(stdTag,stdPosition) + 9;
    long int indexEndStd = XMLresponse.indexOf(endStdTag,stdPosition);
    depTime[i] = XMLresponse.substring(indexStd,indexEndStd);
    stdPosition = indexEndStd + 11;
    //Serial.print("row:"); Serial.print(i); Serial.print(" STD: ");  Serial.println(depSTD[i]);
  }
  for (i = departureRows; i < NATIONAL_RAIL_ROWS; i++){
      depTime[i] = " "; // blank the data for any unused rows
    }
}

void getETD(){// Expected departure - one of - "On time", "Delayed", "Cancelled", "hh:mm"
  long int etdPosition = XMLresponse.indexOf(servicesTag);
  int i;
  for( i = 0 ; i < departureRows; i++){
    
    long int indexEtd = XMLresponse.indexOf(etdTag,etdPosition) + 9;
    long int indexEndEtd = XMLresponse.indexOf(endEtdTag,etdPosition);
    depExp[i] = XMLresponse.substring(indexEtd,indexEndEtd);
    etdPosition = indexEndEtd + 11;
  }
   for (i = departureRows; i < NATIONAL_RAIL_ROWS; i++){
      depExp[i] = " ";
    }
}

// gets the platform number or name - Deals with the instance when a service is cancelled and no platorm XMl tags
// that deliniate the plaform string exist.
  void getPlat(){
  long int searchPosition = XMLresponse.indexOf(servicesTag);
  long int serviceStartPos = 0;
  long int serviceEndPos = 0;
  int i;
  for( i = 0 ; i < departureRows; i++){
    long int indexStartOfService = XMLresponse.indexOf(serviceTag, searchPosition) + 13; 
    long int indexEndOfService = XMLresponse.indexOf(endOfServiceTag, indexStartOfService);
    long int indexPlat = XMLresponse.indexOf(platTag, indexStartOfService) + 14;

    if (indexPlat > indexStartOfService && indexPlat < indexEndOfService){// platform for this service
    long int indexEndPlat = XMLresponse.indexOf(endPlatTag,indexPlat);
    depPlat[i] = XMLresponse.substring(indexPlat,indexEndPlat);
    searchPosition = indexEndOfService + 13 ;
    }
    else{
    depPlat[i] = " ";
    searchPosition = indexEndOfService + 13;
    };
    //Serial.print("row:"); Serial.print(i); Serial.print(" Plat: ");  Serial.println(depPlat[i]);
  }
  for (i = departureRows; i < NATIONAL_RAIL_ROWS; i++){
    depPlat[i] = " ";
    }
}

void getDest(){// gets the Departure Destination string
  long int destPosition = XMLresponse.indexOf(servicesTag);
  int i;
  for( i = 0 ; i < departureRows; i++){
    long int indexDest = XMLresponse.indexOf(destTag,destPosition) + 49;
    //Serial.print("indexDest : "); Serial.println(indexDest);
    destPosition = indexDest;
    long int indexEndDest = XMLresponse.indexOf(endDestTag,destPosition);
    depDest[i] = XMLresponse.substring(indexDest,indexEndDest);
    destPosition = indexEndDest + 28;
    //Serial.print("row:"); Serial.print(i); Serial.print(" Dest: ");  Serial.println(depDest[i]);
  }
  for(i = departureRows; i < NATIONAL_RAIL_ROWS; i++){
      depDest[i] = " ";
    }
}

bool ifServices(){
  long int XMLlength = XMLresponse.length();
  long int indexServices = XMLresponse.indexOf(servicesTag);
  if (indexServices < XMLlength){
    return true;
  }
  else{return false;}
}

void displayTime(){// real-time cloch displayed at the bottonm of the display
  time(&t);
  localtime_r(&t, &timeinfo);
  Serial.print("Current time: ");
  Serial.println(asctime(&timeinfo));
  strftime(timeNow, 6, "%H:%M", &timeinfo);
  //timeNow = 
  Serial.println(timeNow);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.setTextFont(FONT7);
  tft.setTextPadding(tft.textWidth("     ", FONT7));
  tft.drawCentreString(timeNow, 160, 400, FONT7 );
  
}

void displayData(){// displays the rows for each departure 
          Serial.println("Updating Display");
          tft.setTextColor(TFT_ORANGE, TFT_BLACK);
          tft.setTextFont(FONT2);
          // Time     Destination   Plat  Expected 
          for(int i = 0; i < NATIONAL_RAIL_ROWS; i++ ){
          int row_Ypos = (i + 1) * 20;
          tft.setTextPadding(Dest_Xpos - Time_Xpos);
          tft.drawString(depTime[i], Time_Xpos, row_Ypos, FONT2);
          //Serial.print(depTime[i]);Serial.print("  ");
          tft.setTextPadding(PlatHeader_Xpos - Dest_Xpos);
          tft.drawString(depDest[i], Dest_Xpos, row_Ypos, FONT2);
          //Serial.print(depDest[i]);Serial.print("  ");
          tft.setTextPadding(Exp_Xpos - PlatHeader_Xpos );
          tft.drawRightString(depPlat[i], Plat_Xpos, row_Ypos, FONT2);
          //Serial.print(depPlat[i]);Serial.print("  ");
          tft.setTextPadding(Disp_Width - Exp_Xpos);
          tft.drawString(depExp[i], Exp_Xpos, row_Ypos, FONT2);
          //Serial.println(depExp[i]);
          }
}

void printData(){
  for (int i = 0; i < departureRows; i++){
  Serial.print(depTime[i]); Serial.print("  "); Serial.print(depDest[i]); Serial.print("  "); Serial.print(depPlat[i]); Serial.print("  ");Serial.println(depExp[i]);
 }
}

void setClock() {// initialise the NTP clock
  NTP.begin("pool.ntp.org", "time.nist.gov");
  setenv("TZ",TZ_UK, 1);
  tzset();
  Serial.print("Waiting for NTP time sync: ");
  time(&t);
  while (t < 1748736000) {// 1 June 2025
    delay(500);
    Serial.print(".");
    time(&t);
  }
  Serial.println("");
  //struct tm timeinfo;
  localtime_r(&t, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}


 void stringReserve(){// the Strings for the XMl response returned can be large - 
  fullXMLresponse.reserve(18000);
  XMLresponse.reserve(18000);

  for (int i = 0; i < NATIONAL_RAIL_ROWS; i++){
    depDest[i].reserve(30);
    depTime[i].reserve(8);
    depExp[i].reserve(12);
    depPlat[i].reserve(8);
  }
 }

 
// Does the device have a working wifi connection?
bool Network_wifi_check() {
  if (WiFi.status() != WL_CONNECTED) {
    return false;
  } else {
    return true;
  }
}

void displayNetworkStatus(){
  if (mode == 1){
    if(Network_wifi_check()){
      tft.setTextColor(TFT_GREEN); tft.setTextFont(FONT4);
      tft.drawString("!",10,460,FONT4);
      }
      else{
        tft.setTextColor(TFT_ORANGE); tft.setTextFont(FONT4);
        tft.drawString("!",10,460,FONT4);
      }
  }
}
void setup() {
  Serial.begin(115200);
  tft.begin();
  tft.setRotation(2);// Portrait
  //Turn on LCD baklight
  pinMode(TFT_BL, INPUT);
  digitalWrite(TFT_BL, LOW);
  tft.fillScreen(TFT_BLACK);

  Serial.print("Connecting to ");
  tft.setTextFont(FONT4);
  tft.setTextColor(TFT_ORANGE,TFT_BLACK);
  tft.drawCentreString("Connecting.....", 160, 240, FONT4);
    Serial.println(WIFI_SSID);
    
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("WiFi connected.");
    //Serial.println("IP address: ");
  // SSL setup
  client.setInsecure();
  
  setClock();
  tft.fillScreen(TFT_BLACK);
// define the button functions
  button.begin(BUTTON_PIN);
  button.setLongClickDetectedHandler(longClick);// long press to enter/leave station select mode
  button.setLongClickTime(500);
  button.setClickHandler(released);// short click to scroll list of stations
  
  //display list of stations on startup and enter station select mode (mode = 0)
  tft.setTextColor(TFT_WHITE,TFT_BLACK);
  tft.setTextFont(FONT2);
  for (int i = 0; i < STATIONS_LIST; i++){
    tft.drawString(stationName[i], 10, 20*i, FONT2);
  }
  // highlight first station in list in Orange text
  tft.setTextColor(TFT_ORANGE, TFT_BLACK);
  listPosition  = 0;// current position in list
  tft.drawString(stationName[listPosition], 10, 20*listPosition);
  firstTime = 1;
  mode = 0;// display station list on startup
  
}

void loop() {
     button.loop();// check pushbutton state
     //displayNetworkStatus();// use this if you want to display an indication of the status of the network connection
                              // at the bottom of the departure display screen

     if(!firstTime && mode == 1){
        
        displayDepartureHeadings();

      unsigned long currentMillis = millis();
        if (((currentMillis - api_previous_check) >= UPDATE_INTERVAL) || forceUpdate){
          
          api_previous_check = currentMillis;
          API_update_data();// 
          
        }
     }

     if(firstTime){
      triggerDisplayStations =1;
      firstTime = 0;
      }
      if(firstTime || mode == 0){
      displayStationsList();
      }

     }
