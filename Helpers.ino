/*
 * Gets the FONA GPRS module ready to be used.
 */
void getFonaReady()
{
checkpoint:
  bool isReady = 0;
  char *bufPtr = fonaNotificationBuffer; //handy buffer pointer
  String lastWrite = "\n";
  String currentWrite = "\n";
  String readyness;

  while (!fona.available())
    ;

  do
  {
    *bufPtr = fona.read();
    Serial.write(*bufPtr);
    currentWrite = String(*bufPtr);

    if (currentWrite != "\n")
    {
      readyness += currentWrite;
      lastWrite = currentWrite;
    }
    else if (lastWrite == " ")
    {
      readyness += currentWrite;
      lastWrite = currentWrite;
    }
    else
    {
      if (readyness.indexOf("SMS Ready") > -1)
      {
        // when all modules are ready.. i check for "sms ready", because this is the last module that gets ready to use.
        fona.setGPRSNetworkSettings(F("internet")); // set the apn.

        // Optionally configure HTTP gets to follow redirects over SSL.
        // Default is not to follow SSL redirects, however if you uncomment
        // the following line then redirects over SSL will be followed.
        //fona.setHTTPSRedirect(true);

        if (!fona.enableGPRS(true))
        { // enable gprs
          //                    Serial.println(F("Falha ao ligar o GPRS."));
        }
        else
        {
          //                    Serial.println(F("GPRS Ligado com sucesso."));
        }

        if (!fona.enableNetworkTimeSync(true))
        { // enable network time synchronisation
          //                    Serial.println(F("Failed to enable"));
        }
        else
        {
          // read the time
          char buffer[25];
          fona.getTime(buffer, 23); // make sure replybuffer is at least 23 bytes!
                                    //                    Serial.print(F("Time = ")); Serial.println(buffer);
                                    //                    Serial.println(F("Network time synchronised."));
        }

        //                Serial.println(F("Inicialização completa!"));
        isReady = 1;
      }
      readyness = ""; // reset
    }
    delay(1);

  } while ((isReady == 0) && (*bufPtr++ != '\n'));

  if (isReady == 0)
    goto checkpoint;
  *bufPtr = 0; // flush
}

// Maxbotix
/*
 * Reads the sensor's distance in centimeters.
 */
String mareografo_distance()
{
  return "&Dist=" + String(rangeSensorAD.getRange());
}

// BME680
String mareografo_BME(int sensor)
{
  if (!bme.performReading())
  {
    Serial.println("Failed to perform reading :(");
    return;
  }

  //  "&temp=" + String(bme.temperature)

  switch (sensor)
  {
  case 0:
    return "&temp=" + String(bme.temperature);
    //
    //    case 1:
    //      return "&hum=" + String(bme.humidity);
    //
    //    case 2:
    //      return "&gas=" + String(bme.pressure/100.0);
    //
    //    case 3:
    //      return "&pressure=" + String(bme.pressure);
    //
    //    case 4:
    //      return "&alt=" + String(bme.readAltitude(SEALEVELPRESSURE_HPA));
  }
}

// FONA
/*
 * Post data onto a webserver
 * - Parameters:
 *   char args[]: the data to be sent, for example: ?id=2&q=hello+world
 * 
 * - Returns: true on successful post request; false on any failure
 */
bool postData(char data[])
{
  // Post data to website
  uint16_t statuscode;
  int16_t length;
  char url[33];
  String site = "http://trackibane.ddns.net:5055/";
  site.toCharArray(url, sizeof(url));

  //    Serial.println(url);
  //    Serial.println(data);

  Serial.println(F("****"));
  if (!fona.HTTP_POST_start(url, F("text/plain"), (uint8_t *)data, strlen(data), &statuscode, (uint16_t *)&length))
  {
    Serial.println("POST complete!");
    return false;
  }
  while (length > 0)
  {
    while (fona.available())
    {
      char c = fona.read();
      length--;
      if (!length)
        break;
    }
  }
  Serial.println(F("\n****"));
  fona.HTTP_POST_end();
  return true;
}

/*
 * Read GPS either from the sensor or from the GPRS
 *
 */
String mareografo_GPS()
{
  float latitude, longitude;
  String myData = "";

  // if you ask for an altitude reading, getGPS will return false if there isn't a 3D fix
  boolean gps_success = fona.getGPS(&latitude, &longitude);

  if (gps_success)
  {
    myData += "&lat=" + String(latitude, 6);
    myData += "&lon=" + String(longitude, 6);

    //    Serial.println("FONA GPS Sent to Traccar Succesfully!");
    return myData;
  }
  else
  {
    //    Serial.println("Waiting for FONA GPS 3D fix...");
  }

  // Check if fona has gprs location
  if ((fona.type() == FONA3G_A) || (fona.type() == FONA3G_E))
  {
    // Fona 3G doesnt have GPRSlocation :/
    return;
  }

  // Check for network, then GPRS
  //  Serial.println(F("Checking for Cell network..."));
  if (fona.getNetworkStatus() == 1)
  {
    // network & GPRS? Great! Print out the GSM location to compare
    boolean gsmloc_success = fona.getGSMLoc(&latitude, &longitude);

    if (gsmloc_success)
    {
      myData += "&lat=" + String(latitude, 6);
      myData += "&lon=" + String(longitude, 6);
      //      Serial.println("FONA GSMLoc Sent to Traccar Succesfully!");
      return myData;
    }
    else
    {
      //      Serial.println("GSM location failed... Retrying GPS");
      return myData;
    }
  }
  else
  {
    //    Serial.println("Network status is down. Retrying GPS");
    return myData;
  }
}
