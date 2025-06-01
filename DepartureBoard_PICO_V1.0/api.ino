void API_update_data() {
  HTTPClient https;
  
  Serial.println("Getting data update");
  https.begin(client, "https://lite.realtime.nationalrail.co.uk/OpenLDBWS/ldb12.asmx");
  https.addHeader("Content-Type", "text/xml");
  
   int httpResponseCode = https.POST("<soapenv:Envelope xmlns:soapenv=\"http://schemas.xmlsoap.org/soap/envelope/\" xmlns:typ=\"http://thalesgroup.com/RTTI/2013-11-28/Token/types\" xmlns:ldb=\"http://thalesgroup.com/RTTI/2021-11-01/ldb/\"><soapenv:Header><typ:AccessToken><typ:TokenValue>" + String(NATIONAL_RAIL_TOKEN) + "</typ:TokenValue></typ:AccessToken></soapenv:Header><soapenv:Body><ldb:GetDepartureBoardRequest><ldb:numRows>" + String(NATIONAL_RAIL_ROWS) + "</ldb:numRows><ldb:crs>" + stringStationCRS + "</ldb:crs></ldb:GetDepartureBoardRequest></soapenv:Body></soapenv:Envelope>");
  

  //Serial.print(httpResponseCode);Serial.println(httpResponseCode);
  Serial.println(httpResponseCode);
  // If successful...
  if (httpResponseCode == 200) {
    
    // Get response and end connection
      fullXMLresponse = https.getString();// Full response from server
    //Serial.println(response.length());
    //Serial.println(response);
      https.end();
      client.stop();
    // extract sub String of data required for trainservice departures
      long int XMLendIndex = fullXMLresponse.length();
      long int indexStartTrainServices = fullXMLresponse.indexOf(servicesTag); //start of trainServices data in the repsponse 
      long int indexEndTrainServices = fullXMLresponse.indexOf(endOfTrainServicesTag) + 20;// end of the trainSErvice data in the resposnse
      XMLresponse = fullXMLresponse.substring(indexStartTrainServices, indexEndTrainServices);// extract the trainServices data
    //
      getCRS();// get the CRS ( station code) of the data returned
      Serial.print("xmlCRS  : ");Serial.print(xmlCRS); Serial.print("  ");Serial.print("stringStationCRS  : ");Serial.println(stringStationCRS);
      fullXMLresponse = "";//delete the full XML string 

    // check that data string is complete
        if(XMLresponse.endsWith(endOfTrainServicesTag) && xmlCRS.equals(stringStationCRS)){// check that the traiservices data string are valid
    //Serial.println(XMLresponse);
        Serial.print("XMLresponse length: "); Serial.println(XMLresponse.length());
        Serial.println("Data OK - Updating");
    //int n = getNumberOfServices();
    //Serial.print("number of services: "); Serial.println(getNumberOfServices());
          departureRows = getDepartureRows(); // ontain the number of departures in the data - can be less that the number requested (default 15)
          Serial.print("number of Services: "); Serial.println(departureRows);
          getSTD();
          getDest();
          getPlat();
          getETD();
          XMLresponse = "";// null the response String
          displayData();
          printData();
          displayTime();
          forceUpdate = 0;
          apiRetries = 0;
          
    }
    else apiRetry();// if string is imcomplete then try again
    }
    
    XMLresponse = "";// delete string
 
  
}//end of API_updateData

void apiRetry(){
  if(apiRetries < 4){// limit of 3 retries
    forceUpdate =1;
    apiRetries++;
    }
    else forceUpdate = 0;
}
