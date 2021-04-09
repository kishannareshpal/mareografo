/*
 * Universidade Save - Maxixe (UniSave)
 * 
 * Projecto de Bolsas de Iniciação Científica 2018:
 * = Mareógrafo: Medidor do Nível do rio autónomo.
 * 
 * Created 16 December 2018
 * By Kishan Nareshpal Jadav (kishan_jadav@hotmail.com | https://github.com/kishannareshpal)
 * & Gorka Solana Arteche (gorsol@gmail.com | https://github.com/gorsol)
 * 
 * Modified 12 January 2019
 * By Kishan Nareshpal Jadav
 * 
 * Modified 09 April 2021
 * By Kishan Nareshpal Jadav - Changed Universidade Pedagógica da Maxixe to Universidade Save.
 * 
 */

// NOTES
//  (1) O FONA quanto está conectado com o painel solar, faz o arduino aquecer.
// END OF NOTES

// PIN INSTRUCTIONS - Last revision: 12/01/2019 -----------------------------------------------------
// Please follow these instructions for assembly before uploading and running this code to the arduino
/* - For: FONA GSM SHIELD 808 (v2)
   *  1. Insert Vodacom Moçambique SIM CARD (without pin code) into the shield's sim tray.
   *  2. Connect the 3.7V 3000mAh to the shield battery connector.
   *  3. Leave the [CHRG]/[RUN] switch in [CHRG]. Why put on [CHRG]? Because we are charing the 3.7V Battery with the big 6V one.m,
   *  4. Connect the shiled to the arduino.   
   *  And that's i
   *  
   *  == MORE FONA INFO ==
   *  (a) When the light (NET) blinks for:
   *    - 64ms ON, 800ms OFF :: The module is running but hasn't made connection to the cellular network yet.
   *    - 64ms ON, 3sec OFF :: The module has made contact with the cellular network and can sends/receive voice and SMS
   *    - 64ms ON, 300ms OFF :: The GPRS data connection you requested is active.
   *  ====================
   */

/* - For: MAXBOTIX ULTRASONIC SENSOR MB7092 XL
   *  1. Connect the ground and power pins from sensor to arduino.
   *  2. Connect the AN pin from sensor to Analog pin 0 on Arduino.
   *  And that's it!
   */

/* - For: BME680 Multi Ambient sensor
   *  1. Connect the power(VIN) and ground(GND) to the arduino.
   *  2. Connect the SCK from sensor to pin 13 on the arduino.
   *  3. Connect the SDO from sensor to pin 12 on the arduino.
   *  4. Connect the SDI from sensor to pin 11 on the arduino.
   *  5. Connect the CS  from sensor to pin 10 on the arduino.
   *  And that's it!
   */

/* - For: SOLAR PANEL
   *  1. Connect the power and ground battery pins from the panel to the 6V 4.5Ah big battery
   *  2. Connect the arduino power cable to the arduino.
   *  And that's it!
   */

// ------------------------------------------------
// -- THE SWEET CODE - Last revision: 12/01/2019 --
// ------------------------------------------------
#include <SoftwareSerial.h>  // Used by: fona
#include <Wire.h>            // Used by: bme
#include <SPI.h>             // Used by: bme
#include <Adafruit_Sensor.h> // Used by: bme
#include "Adafruit_BME680.h" // Used by: bme
#include "Maxbotix.h"        // Used by: maxbotix
#include "Adafruit_FONA.h"   // Used by: fona

#define BAUDRATE 4800

/* BME680 Definitions */
#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10
#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME680 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK);

/* Maxbotix Definitions */
Maxbotix rangeSensorAD(A0, Maxbotix::AN, Maxbotix::XL);

/* FONA Denfinitions */
#define FONA_RX 2
#define FONA_TX 3
#define FONA_RST 4
SoftwareSerial fonaSS = SoftwareSerial(FONA_TX, FONA_RX);
SoftwareSerial *fonaSerial = &fonaSS;
Adafruit_FONA fona = Adafruit_FONA(FONA_RST);
char fonaNotificationBuffer[64]; // for notifications from the FONA
char smsBuffer[4];
bool shouldProceed;

void setup()
{
  // General Setup
  Serial.begin(BAUDRATE);
  while (!Serial)
    ;
  Serial.println(F("Mareografo - Serial  initalized successfully."));

  // FONA specific setup
  fonaSerial->begin(BAUDRATE);
  if (!fona.begin(*fonaSerial))
  {
    Serial.println(F("Could not find a valid FONA 808 sensor, check wiring!"));
    while (1)
      ;
  }

  Serial.println(F("Enabling GPS..."));
  fona.enableGPS(true);
  fonaSerial->print("AT+CNMI=2,1\r\n"); //set up the FONA to send a +CMTI notification when an SMS is received

  getFonaReady();

  // BME680 specific setup
  if (!bme.begin())
  {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
    while (1)
      ;
  }

  // Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms

  // Maxbotix specific setup :: No further setup is required for maxbotix
} /* ./end of setup */

void loop()
{
  String my_data = "?id=20181812"; // traccar device id
  char data[84];
  my_data += mareografo_distance(); // retrieves the distance in cm
  my_data += mareografo_GPS();      // retrieves the gps location: Latitude, Longitude
  my_data += mareografo_BME(0);     // retrieves temperature in celsius
  if (!bme.performReading())
  {
    //    Serial.println("Failed to perform reading :(");
    return;
  }

  my_data += "&temp=" + String(bme.temperature);

  my_data.toCharArray(data, sizeof(data)); // prepare the data
  postData(data);                          // send the data to traccar

  delay(5000);
}
