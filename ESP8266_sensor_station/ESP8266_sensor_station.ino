#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include "MHZ19.h"
#include "PMS.h"
#include <SoftwareSerial.h>
#include <Wire.h>
#include "Adafruit_SGP30.h"
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include "Adafruit_CCS811.h"
#include <U8g2lib.h>

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R3, /* reset=*/ U8X8_PIN_NONE);

const char* ssid = "yourssidhere";
const char* password = "yourpasswordhere";

#define RX_PIN_MHZ19 3                                          // Rx pin which the MHZ19 Tx pin is attached to
#define TX_PIN_MHZ19 2                                          // Tx pin which the MHZ19 Rx pin is attached to

SoftwareSerial mySerial(RX_PIN_MHZ19, TX_PIN_MHZ19);
MHZ19 myMHZ19;

#define RX_PIN_PMS 12                                          // Rx pin which the PMS Tx pin is attached to
#define TX_PIN_PMS 14                                          // Tx pin which the PMS Rx pin is attached to

SoftwareSerial mySerial1(RX_PIN_PMS, TX_PIN_PMS);
PMS pms(mySerial1);
PMS::DATA data;

Adafruit_SGP30 sgp;

#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme; // I2C

Adafruit_CCS811 ccs;

HTTPClient http;  //Declare an object of class HTTPClient
String stringVal;
String httpReq;
String payload;
int httpCode;

volatile uint32_t secCount = 0;
uint16_t interval1 = 60;
uint16_t interval2 = 300;
uint16_t resetInterval = 0;
uint16_t pmsWait = 30;
uint8_t check = 2;
uint8_t readSensor = 0; //1 == sensor data is being read (ensures the display doesn't interfere with its communication)

char str[10];
uint16_t len;

uint16_t MHZ_CO2 = 0;
uint16_t SGP_TVOC = 0;
uint16_t SGP_eCO2 = 0;
uint16_t SGP_rawH2 = 0;
uint16_t SGP_rawEthanol = 0;
uint16_t SGP_eCO2_base = 0;
uint16_t SGP_TVOC_base = 0;
float bmeTemperature = 0;
float bmeHumidity = 0;
float bmePressure = 0;
float CCS_temp = 0;
uint16_t CCS_eCO2 = 0;
uint16_t CCS_TVOC = 0;
uint16_t MICS5524_A0 = 0;
uint16_t PMS_PM1_0 = 0;
uint16_t PMS_PM2_5 = 0;
uint16_t PMS_PM10_0 = 0;

void ICACHE_RAM_ATTR onTime() {
  secCount++;
  Serial.println(secCount);

  if (!(secCount % interval1) && check != 2) {
    check = 1;
  }

  if (!(secCount % interval2)) {
    check = 2;
  }

  static uint8_t showPage = 0;
  if (secCount % 5 == 0) {
    showPage++;
    if (showPage == 4) {
      showPage = 0;
    }
  }

  if (!readSensor) {
    u8g2.clearBuffer();
    switch (showPage) {
      case 0:
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(16, 0, "PM1.0");
        u8g2.setFont(u8g2_font_10x20_tf);
        len = snprintf(NULL, 0, "%d", PMS_PM1_0);
        sprintf(str, "%d", PMS_PM1_0);
        u8g2.drawStr((63 - (len * 10)) / 2, 11, str);
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(16, 27, "ug/m3");

        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(16, 45, "PM2.5");
        u8g2.setFont(u8g2_font_10x20_tf);
        len = snprintf(NULL, 0, "%d", PMS_PM2_5);
        sprintf(str, "%d", PMS_PM2_5);
        u8g2.drawStr((63 - (len * 10)) / 2, 56, str);
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(16, 72, "ug/m3");
        u8g2.setFont(u8g2_font_6x10_tf);

        u8g2.drawStr(13, 90, "PM10.0");
        u8g2.setFont(u8g2_font_10x20_tf);
        len = snprintf(NULL, 0, "%d", PMS_PM10_0);
        sprintf(str, "%d", PMS_PM10_0);
        u8g2.drawStr((63 - (len * 10)) / 2, 101, str);
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(16, 117, "ug/m3");
        break;

      case 1:
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(10, 0, "MHZ CO2");
        u8g2.setFont(u8g2_font_10x20_tf);
        len = snprintf(NULL, 0, "%d", MHZ_CO2);
        sprintf(str, "%d", MHZ_CO2);
        u8g2.drawStr((63 - (len * 10)) / 2, 11, str);
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(22, 27, "ppm");

        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(7, 45, "SGP eCO2");
        u8g2.setFont(u8g2_font_10x20_tf);
        len = snprintf(NULL, 0, "%d", SGP_eCO2);
        sprintf(str, "%d", SGP_eCO2);
        u8g2.drawStr((63 - (len * 10)) / 2, 56, str);
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(22, 72, "ppm");
        u8g2.setFont(u8g2_font_6x10_tf);

        u8g2.drawStr(7, 90, "CCS eCO2");
        u8g2.setFont(u8g2_font_10x20_tf);
        len = snprintf(NULL, 0, "%d", CCS_eCO2);
        sprintf(str, "%d", CCS_eCO2);
        u8g2.drawStr((63 - (len * 10)) / 2, 101, str);
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(22, 117, "ppm");
        break;

      case 2:
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(7, 0, "SGP TVOC");
        u8g2.setFont(u8g2_font_10x20_tf);
        len = snprintf(NULL, 0, "%d", SGP_TVOC);
        sprintf(str, "%d", SGP_TVOC);
        u8g2.drawStr((63 - (len * 10)) / 2, 11, str);
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(22, 27, "ppb");

        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(7, 45, "CCS TVOC");
        u8g2.setFont(u8g2_font_10x20_tf);
        len = snprintf(NULL, 0, "%d", CCS_TVOC);
        sprintf(str, "%d", CCS_TVOC);
        u8g2.drawStr((63 - (len * 10)) / 2, 56, str);
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(22, 72, "ppb");
        u8g2.setFont(u8g2_font_6x10_tf);

        u8g2.drawStr(7, 90, "MICS5524");
        u8g2.setFont(u8g2_font_10x20_tf);
        len = snprintf(NULL, 0, "%d", MICS5524_A0);
        sprintf(str, "%d", MICS5524_A0);
        u8g2.drawStr((63 - (len * 10)) / 2, 101, str);
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(22, 117, "ppm");
        break;

      case 3:
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(7, 0, "BME temp");
        u8g2.setFont(u8g2_font_10x20_tf);
        len = snprintf(NULL, 0, "%.1f", bmeTemperature);
        sprintf(str, "%.1f", bmeTemperature);
        u8g2.drawStr((63 - (len * 10)) / 2, 11, str);
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawUTF8(25, 27, "°C");

        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(10, 45, "BME hum");
        u8g2.setFont(u8g2_font_10x20_tf);
        len = snprintf(NULL, 0, "%.0f", bmeHumidity);
        sprintf(str, "%.0f", bmeHumidity);
        u8g2.drawStr((63 - (len * 10)) / 2, 56, str);
        u8g2.setFont(u8g2_font_7x13_tf);
        u8g2.drawStr(28, 72, "%");
        u8g2.setFont(u8g2_font_6x10_tf);

        u8g2.drawStr(7, 90, "BME pres");
        u8g2.setFont(u8g2_font_10x20_tf);
        len = snprintf(NULL, 0, "%.0f", bmePressure);
        sprintf(str, "%.0f", bmePressure);
        u8g2.drawStr((63 - (len * 10)) / 2, 101, str);
        u8g2.setFont(u8g2_font_6x10_tf);
        u8g2.drawStr(22, 117, "hPa");
        break;
    }
    u8g2.sendBuffer();
  }

  if ((resetInterval != 0) && (secCount >= 60 * resetInterval)) {
    ESP.deepSleep(5 * 60e6);
  }
}

void setup()
{
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);
  pinMode(15, OUTPUT);
  digitalWrite(15, HIGH);

  //Initialize Ticker every 1s
  timer1_attachInterrupt(onTime); // Add ISR Function
  timer1_enable(TIM_DIV256, TIM_EDGE, TIM_LOOP);
  /* Dividers:
    TIM_DIV1 = 0,   //80MHz (80 ticks/us - 104857.588 us max)
    TIM_DIV16 = 1,  //5MHz (5 ticks/us - 1677721.4 us max)
    TIM_DIV256 = 3  //312.5Khz (1 tick = 3.2us - 26843542.4 us max)
    Reloads:
    TIM_SINGLE  0 //on interrupt routine you need to write a new value to start the timer again
    TIM_LOOP  1 //on interrupt the counter will start with the same value again
  */

  // Arm the Timer for our 1s Interval
  timer1_write(312500);

  Serial.begin(9600);
  mySerial1.begin(9600);
  mySerial.begin(9600);                               // (Uno example) device to MH-Z19 serial start

  //myMHZ19.printCommunication();
  myMHZ19.begin(mySerial);                                // *Serial(Stream) refence must be passed to library begin().
  myMHZ19.autoCalibration();                              // Turn auto calibration ON (OFF autoCalibration(false))

  pms.passiveMode();    // Switch to passive mode

  u8g2.begin();
  u8g2.setFont(u8g2_font_6x10_tf);
  u8g2.setFontRefHeightExtendedText();
  u8g2.setDrawColor(1);
  u8g2.setFontPosTop();
  u8g2.setFontDirection(0);

  sgp.begin();
  // If you have a baseline measurement from before you can assign it to start, to 'self-calibrate'
  //sgp.setIAQBaseline(0x8E68, 0x8F41);  // Will vary for each sensor!

  bme.begin(0x76);
  bme.setSampling(Adafruit_BME280::MODE_FORCED,
                  Adafruit_BME280::SAMPLING_X1,
                  Adafruit_BME280::SAMPLING_X1,
                  Adafruit_BME280::SAMPLING_X1,
                  Adafruit_BME280::FILTER_OFF);

  ccs.begin();
  //calibrate temperature sensor
  uint16_t i = 0;
  while (!ccs.available()) {
    if (i > 20) {
      break;
    }
    i++;
    delay(100);
  }
  float temp = ccs.calculateTemperature();
  ccs.setTempOffset(temp - 25.0);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(10);
    i++;
    if (i > 1000) {
      Serial.println("WiFi was not connected.");
      break;
    }
  }
}

void loop()
{
  if (check == 1) {
    readSensor = 1;
    basicParameters();
    delay(100);
    MH_Z19B();
    delay(100);
    SGP30();
    delay(100);
    BME_280();
    delay(100);
    CCS_811();
    delay(100);
    MICS5524();
    delay(100);
    http.end();

    readSensor = 0;
    check = 0;
  }

  if (check == 2) {
    readSensor = 1;
    basicParameters();
    delay(100);
    MH_Z19B();
    delay(100);
    SGP30();
    delay(100);
    BME_280();
    delay(100);
    CCS_811();
    delay(100);
    MICS5524();
    delay(100);
    PMS7003(); //readSensor is LOW while waiting inside this function
    delay(100);
    http.end();

    readSensor = 0;
    check = 0;
  }

  static uint32_t checkBtn = 0;
  if (WiFi.status() == WL_CONNECTED && (secCount != checkBtn)) {
    checkBtn = secCount;

    http.begin("http://yourserverhere:8080/yourkeyhere/get/V6");  //Specify request destination
    httpCode = http.GET();                                                                  //Send the request
    if (httpCode > 0) { //Check the returning code
      payload = http.getString();   //Get the request response payload
      payload.remove(0, 2);
      if (payload.toInt()) {
        pinMode(15, OUTPUT);
        digitalWrite(15, HIGH);
      } else {
        pinMode(15, OUTPUT);
        digitalWrite(15, LOW);
      }
    }

    http.begin("http://yourserverhere:8080/yourkeyhere/get/V7");  //Specify request destination
    httpCode = http.GET();                                                                  //Send the request
    if (httpCode > 0) { //Check the returning code
      payload = http.getString();   //Get the request response payload
      payload.remove(0, 2);
      if (payload.toInt()) {
        pinMode(13, OUTPUT);
        digitalWrite(13, LOW);
      } else {
        pinMode(13, OUTPUT);
        digitalWrite(13, HIGH);
      }
    }
  }

}

uint32_t getAbsoluteHumidity(float temperature, float humidity) {
  // approximation formula from Sensirion SGP30 Driver Integration chapter 3.15
  const float absoluteHumidity = 216.7f * ((humidity / 100.0f) * 6.112f * exp((17.62f * temperature) / (243.12f + temperature)) / (273.15f + temperature)); // [g/m^3]
  const uint32_t absoluteHumidityScaled = static_cast<uint32_t>(1000.0f * absoluteHumidity); // [mg/m^3]
  return absoluteHumidityScaled;
}

void basicParameters() {
  if (WiFi.status() == WL_CONNECTED) {
    http.begin("http://yourserverhere:8080/yourkeyhere/get/V1");  //Specify request destination
    httpCode = http.GET();                                                                  //Send the request
    if (httpCode > 0) { //Check the returning code
      payload = http.getString();   //Get the request response payload
      payload.remove(0, 2);
      interval1 = payload.toInt();
    }

    http.begin("http://yourserverhere:8080/yourkeyhere/get/V2");  //Specify request destination
    httpCode = http.GET();                                                                  //Send the request
    if (httpCode > 0) { //Check the returning code
      payload = http.getString();   //Get the request response payload
      payload.remove(0, 2);
      interval2 = payload.toInt();
    }

    http.begin("http://yourserverhere:8080/yourkeyhere/get/V3");  //Specify request destination
    httpCode = http.GET();                                                                  //Send the request
    if (httpCode > 0) { //Check the returning code
      payload = http.getString();   //Get the request response payload
      payload.remove(0, 2);
      resetInterval = payload.toInt();
    }

    http.begin("http://yourserverhere:8080/yourkeyhere/get/V4");  //Specify request destination
    httpCode = http.GET();                                                                  //Send the request
    if (httpCode > 0) { //Check the returning code
      payload = http.getString();   //Get the request response payload
      payload.remove(0, 2);
      pmsWait = payload.toInt();
    }

    stringVal = WiFi.RSSI();
    httpReq = String("http://yourserverhere:8080/yourkeyhere/update/V5?value=" + stringVal);
    http.begin(httpReq);  //Specify request destination
    httpCode = http.GET();
  }
}

void MH_Z19B() {
  MHZ_CO2 = myMHZ19.getCO2();
  Serial.print("MH-Z19B CO2 (ppm): ");
  Serial.println(MHZ_CO2);

  if (WiFi.status() == WL_CONNECTED && MHZ_CO2 != 5000) {
    stringVal = MHZ_CO2;
    httpReq = String("http://yourserverhere:8080/yourkeyhere/update/V10?value=" + stringVal);
    http.begin(httpReq);  //Specify request destination
    httpCode = http.GET();
  }

  //uint8_t temp = myMHZ19.getTemperature();
  //Serial.print("MH-Z19B Temp: ");
  //Serial.println(temp);
}

void SGP30() {
  //If you have a temperature / humidity sensor, you can set the absolute humidity to enable the humditiy compensation for the air quality signals
  //float temperature = 22.1; // [°C]
  //float humidity = 45.2; // [%RH]
  //sgp.setHumidity(getAbsoluteHumidity(temperature, humidity));

  if (! sgp.IAQmeasure()) {
    Serial.println("sgp.IAQmeasure() Measurement failed");
    return;
  }
  SGP_TVOC = sgp.TVOC;
  SGP_eCO2 = sgp.eCO2;
  Serial.print("SGP30 TVOC ");
  Serial.print(SGP_TVOC);
  Serial.print(" ppb\t");
  Serial.print("eCO2 ");
  Serial.print(SGP_eCO2);
  Serial.println(" ppm");

  if (WiFi.status() == WL_CONNECTED) {
    stringVal = SGP_TVOC;
    httpReq = String("http://yourserverhere:8080/yourkeyhere/update/V20?value=" + stringVal);
    http.begin(httpReq);  //Specify request destination
    httpCode = http.GET();

    stringVal = SGP_eCO2;
    httpReq = String("http://yourserverhere:8080/yourkeyhere/update/V21?value=" + stringVal);
    http.begin(httpReq);  //Specify request destination
    httpCode = http.GET();
  }

  if (! sgp.IAQmeasureRaw()) {
    Serial.println("sgp.IAQmeasureRaw() Raw Measurement failed");
    return;
  }
  SGP_rawH2 = sgp.rawH2;
  SGP_rawEthanol = sgp.rawEthanol;
  Serial.print("Raw H2 ");
  Serial.print(SGP_rawH2);
  Serial.print(" \t");
  Serial.print("Raw Ethanol ");
  Serial.print(SGP_rawEthanol);
  Serial.println("");

  if (WiFi.status() == WL_CONNECTED) {
    stringVal = SGP_rawH2;
    httpReq = String("http://yourserverhere:8080/yourkeyhere/update/V22?value=" + stringVal);
    http.begin(httpReq);  //Specify request destination
    httpCode = http.GET();

    stringVal = SGP_rawEthanol;
    httpReq = String("http://yourserverhere:8080/yourkeyhere/update/V23?value=" + stringVal);
    http.begin(httpReq);  //Specify request destination
    httpCode = http.GET();
  }

  if (! sgp.getIAQBaseline(&SGP_eCO2_base, &SGP_TVOC_base)) {
    Serial.println("Failed to get baseline readings");
    return;
  }
  Serial.print("****Baseline values: eCO2: 0x");
  Serial.print(SGP_eCO2_base, HEX);
  Serial.print(" & TVOC: 0x");
  Serial.println(SGP_TVOC_base, HEX);

  if (WiFi.status() == WL_CONNECTED) {
    stringVal = SGP_eCO2_base;
    httpReq = String("http://yourserverhere:8080/yourkeyhere/update/V24?value=" + stringVal);
    http.begin(httpReq);  //Specify request destination
    httpCode = http.GET();

    stringVal = SGP_TVOC_base;
    httpReq = String("http://yourserverhere:8080/yourkeyhere/update/V25?value=" + stringVal);
    http.begin(httpReq);  //Specify request destination
    httpCode = http.GET();
  }
}

void BME_280() {
  bme.takeForcedMeasurement();
  bmeTemperature = bme.readTemperature();
  bmeHumidity = bme.readHumidity();
  bmePressure = bme.readPressure() / 100.0F;

  Serial.print("BME280 Temperature: ");
  Serial.print(bmeTemperature);
  Serial.println("°C");
  Serial.print("BME280 Humidity: ");
  Serial.print(bmeHumidity);
  Serial.println("%");
  Serial.print("BME280 Pressure: ");
  Serial.print(bmeTemperature);
  Serial.println(" hPa");

  if (WiFi.status() == WL_CONNECTED) {
    stringVal = String(bmeTemperature, 3);
    httpReq = String("http://yourserverhere:8080/yourkeyhere/update/V30?value=" + stringVal);
    http.begin(httpReq);  //Specify request destination
    httpCode = http.GET();

    stringVal = String(bmeHumidity, 3);
    httpReq = String("http://yourserverhere:8080/yourkeyhere/update/V31?value=" + stringVal);
    http.begin(httpReq);  //Specify request destination
    httpCode = http.GET();

    stringVal = String(bmePressure, 3);
    httpReq = String("http://yourserverhere:8080/yourkeyhere/update/V32?value=" + stringVal);
    http.begin(httpReq);  //Specify request destination
    httpCode = http.GET();
  }
}

void CCS_811() {
  if (ccs.available()) {
    CCS_temp = ccs.calculateTemperature();
    if (!ccs.readData()) {
      CCS_eCO2 = ccs.geteCO2();
      CCS_TVOC = ccs.getTVOC();
      Serial.print("CCS-811 CO2: ");
      Serial.print(CCS_eCO2);
      Serial.print("ppm, TVOC: ");
      Serial.print(CCS_TVOC);
      Serial.print("ppb   Temp:");
      Serial.println(CCS_temp);

      if (WiFi.status() == WL_CONNECTED) {
        stringVal = CCS_eCO2;
        httpReq = String("http://yourserverhere:8080/yourkeyhere/update/V40?value=" + stringVal);
        http.begin(httpReq);  //Specify request destination
        httpCode = http.GET();

        stringVal = CCS_TVOC;
        httpReq = String("http://yourserverhere:8080/yourkeyhere/update/V41?value=" + stringVal);
        http.begin(httpReq);  //Specify request destination
        httpCode = http.GET();

        stringVal = String(CCS_temp, 3);
        httpReq = String("http://yourserverhere:8080/yourkeyhere/update/V42?value=" + stringVal);
        http.begin(httpReq);  //Specify request destination
        httpCode = http.GET();
      }
    }
    else {
      Serial.println("CCS-811 ERROR!");
      while (1);
    }
  }
}

void MICS5524() {
  MICS5524_A0 = analogRead(A0);

  Serial.print("MICS5524 analogRead = ");
  Serial.println(MICS5524_A0);

  if (WiFi.status() == WL_CONNECTED) {
    stringVal = MICS5524_A0;
    httpReq = String("http://yourserverhere:8080/yourkeyhere/update/V50?value=" + stringVal);
    http.begin(httpReq);  //Specify request destination
    httpCode = http.GET();
  }
}

void PMS7003() {
  Serial.println("Waking up, wait pmsWait seconds for stable readings...");
  pms.wakeUp();
  readSensor = 0;
  delay(pmsWait * 1000);
  readSensor = 1;

  Serial.println("Send read request...");
  pms.requestRead();

  Serial.println("Wait max. 1 second for read...");
  for (uint8_t i = 0; i < 5; i++) {
    if (pms.readUntil(data)) {
      PMS_PM1_0 = data.PM_AE_UG_1_0;
      PMS_PM2_5 = data.PM_AE_UG_2_5;
      PMS_PM10_0 = data.PM_AE_UG_10_0;

      Serial.print("PM 1.0 (ug/m3): ");
      Serial.println(PMS_PM1_0);
      Serial.print("PM 2.5 (ug/m3): ");
      Serial.println(PMS_PM2_5);
      Serial.print("PM 10.0 (ug/m3): ");
      Serial.println(PMS_PM10_0);

      if (WiFi.status() == WL_CONNECTED) {
        stringVal = PMS_PM1_0;
        httpReq = String("http://yourserverhere:8080/yourkeyhere/update/V60?value=" + stringVal);
        http.begin(httpReq);  //Specify request destination
        httpCode = http.GET();

        stringVal = PMS_PM2_5;
        httpReq = String("http://yourserverhere:8080/yourkeyhere/update/V61?value=" + stringVal);
        http.begin(httpReq);  //Specify request destination
        httpCode = http.GET();

        stringVal = PMS_PM10_0;
        httpReq = String("http://yourserverhere:8080/yourkeyhere/update/V62?value=" + stringVal);
        http.begin(httpReq);  //Specify request destination
        httpCode = http.GET();
      }

      break;
    } else {
      Serial.println("No data.");
    }
  }

  Serial.println("Going to sleep for 60 seconds.");
  delay(1000);
  pms.sleep();
}
