/*
read ppm from scd30 and display on neopixel strip (stick/ring)
*/

#include <Wire.h>
#include <WiFi.h>

#include <Adafruit_Sensor.h>
#include "Adafruit_BME680.h"

#include <Adafruit_NeoPixel.h>

#include "SparkFun_SCD30_Arduino_Library.h" //Click here to get the library: http://librarymanager/All#SparkFun_SCD30
SCD30 airSensor;

#define LED_PIN 15 // strip is connected to pin
#define LED_COUNT 16 // number of pixels

#define SEALEVELPRESSURE_HPA (1013.25)

#define HIGH_CO2_BOUNDARY 2000
#define LOW_CO2_BOUNDARY 1000

Adafruit_BME680 bme; // I2C

Adafruit_NeoPixel strip = Adafruit_NeoPixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

// Replace with your network credentials
const char* ssid     = "co2ampel";

// Set web server port number to 80
WiFiServer server(80);

float hum_weighting = 0.25; // so hum effect is 25% of the total air quality score
float gas_weighting = 0.75; // so gas effect is 75% of the total air quality score

float hum_score, gas_score;
float gas_reference = 250000;
float hum_reference = 40;
int getgasreference_count = 0;
int ppm;
int hum;
int temp;
float air_quality_score;
byte error, address;
bool isAQSensorAvailable;

// Variable to store the HTTP request
String header;

struct Button {
  const uint8_t PIN;
  uint32_t numberKeyPresses;
  bool pressed;
};

Button button1 = {0, 0, false};

void IRAM_ATTR isr() {
  button1.numberKeyPresses += 1;
  button1.pressed = true;
}

void setup()
{
  Serial.begin(115200);
 
  Wire.begin();
  delay(1000); // give sensors some time to power-up

  attachInterrupt(button1.PIN, isr, FALLING);  

  isAQSensorAvailable = false;
  address=0x76; // bme680-sensor
  Wire.beginTransmission(address);
  error = Wire.endTransmission();
  if (error==0) isAQSensorAvailable=true;

  Serial.println(isAQSensorAvailable);

  if (airSensor.begin() == false)
  {
    Serial.println("Air sensor not detected. Please check wiring. Freezing...");
    while (1)
      ;
  }  

  if (isAQSensorAvailable) {

    if (!bme.begin(0x76)) {
      Serial.println("Could not find a valid BME680 sensor, check wiring!");
      while (1);
    }
  
  // Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms
  }

  // Create Wi-Fi network with SSID and password
  Serial.print("Setting AP (Access Point)…");
  // Remove the password parameter, if you want the AP (Access Point) to be open
  WiFi.softAP(ssid); 

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  
  server.begin();
  
  strip.begin();
  strip.setBrightness(64); // set brightness
  strip.show(); // Initialize all pixels to 'off'
}

void loop()
{
  
  if (airSensor.dataAvailable())
  {
      
    Serial.print("co2(ppm):");
    Serial.print(airSensor.getCO2());

    ppm=airSensor.getCO2();
    hum=airSensor.getHumidity();
    temp=airSensor.getTemperature();

    Serial.println();

  }
 
  if (button1.pressed) {
    Serial.printf("Button 1 has been pressed %u times\n", button1.numberKeyPresses);
    button1.pressed = false;

    // calibration
    airSensor.setAltitudeCompensation(0); // Altitude in m ü NN
    airSensor.setForcedRecalibrationFactor(400); // fresh air
    Serial.print("SCD30 calibration, please wait 30 s ...");
      
    strip.clear();
    for ( int i = 0; i < LED_COUNT; i++) { // blue for calibration
      strip.setPixelColor(i, 0, 0, 255);
    }
    strip.show();
      
    delay(30000);
      
    Serial.println("Calibration done");
    Serial.println("Rebooting device");
    ESP.restart();
      
    delay(5000);
  }

  if (isAQSensorAvailable) {
    if (! bme.performReading()) {
      Serial.println("Failed to perform reading :(");
      return;
    }
    Serial.print("Temperature = ");
    Serial.print(bme.temperature);
    Serial.println(" *C");

    Serial.print("Pressure = ");
    Serial.print(bme.pressure / 100.0);
    Serial.println(" hPa");

    Serial.print("Humidity = ");
    Serial.print(bme.humidity);
    Serial.println(" %");

    Serial.print("Approx. Altitude = ");
    Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
    Serial.println(" m");

    Serial.print("        Gas = ");
    Serial.print(bme.readGas());
    Serial.println("R\n");

   //Calculate humidity contribution to IAQ index
    float current_humidity = bme.readHumidity();
    if (current_humidity >= 38 && current_humidity <= 42)
      hum_score = 0.25*100; // Humidity +/-5% around optimum 
    else
    { //sub-optimal
      if (current_humidity < 38) 
        hum_score = 0.25/hum_reference*current_humidity*100;
      else
      {
        hum_score = ((-0.25/(100-hum_reference)*current_humidity)+0.416666)*100;
      }
    }
  
    //Calculate gas contribution to IAQ index
    float gas_lower_limit = 5000;   // Bad air quality limit
    float gas_upper_limit = 50000;  // Good air quality limit 
    if (gas_reference > gas_upper_limit) gas_reference = gas_upper_limit; 
    if (gas_reference < gas_lower_limit) gas_reference = gas_lower_limit;
    gas_score = (0.75/(gas_upper_limit-gas_lower_limit)*gas_reference -(gas_lower_limit*(0.75/(gas_upper_limit-gas_lower_limit))))*100;
  
    //Combine results for the final IAQ index value (0-100% where 100% is good quality air)
    air_quality_score = hum_score + gas_score;

    Serial.println("Air Quality = "+String(air_quality_score,1)+"% derived from 25% of Humidity reading and 75% of Gas reading - 100% is good quality air");
    Serial.println("Humidity element was : "+String(hum_score/100)+" of 0.25");
    Serial.println("     Gas element was : "+String(gas_score/100)+" of 0.75");
    if (bme.readGas() < 120000) Serial.println("***** Poor air quality *****");
    Serial.println();
    if ((getgasreference_count++)%10==0) GetGasReference(); 
    Serial.println(CalculateIAQ(air_quality_score));
    Serial.println("------------------------------------------------");
  }
    int c;
    
    if (ppm>HIGH_CO2_BOUNDARY) { c=strip.Color(255,0,0); }
    if (ppm>LOW_CO2_BOUNDARY && ppm <= HIGH_CO2_BOUNDARY) { c=strip.Color(255,165,0); }
    if (ppm<=LOW_CO2_BOUNDARY) { c=strip.Color(0,128,0); }    

    int ct=map(ppm, 350, HIGH_CO2_BOUNDARY, 1, strip.numPixels());
  
    for(uint16_t i=0; i<strip.numPixels(); i++) {
      if (i<=ct) {
        strip.setPixelColor(i, c);
      } else {
        strip.setPixelColor(i, strip.Color(0,0,0));
      }
    }
    strip.show();    

    WiFiClient client = server.available(); // Listen for incoming clients

 if (client) {                             // If a new client connects,
 
    Serial.println("New Client.");          // print a message out in the serial port
    String currentLine = "";                // make a String to hold incoming data from the client
    while (client.connected()) {  // loop while the client's connected
      if (client.available()) {             // if there's bytes to read from the client,
        char c = client.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        header += c;
        if (c == '\n') {                    // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0) {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();
            
            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the table 
            client.println("<style>body { text-align: center; font-family: \"Trebuchet MS\", Arial;}");
            client.println("table { border-collapse: collapse; width:35%; margin-left:auto; margin-right:auto; }");
            client.println("th { padding: 12px; background-color: #0043af; color: white; }");
            client.println("tr { border: 1px solid #ddd; padding: 12px; }");
            client.println("tr:hover { background-color: #bcbcbc; }");
            client.println("td { border: none; padding: 12px; }");
            client.println(".sensor { color:white; font-weight: bold; background-color: #bcbcbc; padding: 1px; }");
            
            // Web Page Heading
            client.println("</style></head><body><h1>CO2-Ampel</h1>");
            client.println("<table><tr><th>Messpunkt</th><th>Messwert</th></tr>");

            client.println("<tr><td>CO2</td><td><span class=\"sensor\">");
            client.println(airSensor.getCO2());
            client.println(" ppm</span></td></tr>"); 
            
            client.println("<tr><td>Temperatur</td><td><span class=\"sensor\">");
            client.println(airSensor.getTemperature());
            client.println(" *C</span></td></tr>"); 

            client.println("<tr><td>Luftfeuchtigkeit</td><td><span class=\"sensor\">");
            client.println(airSensor.getHumidity());
            client.println(" %</span></td></tr>"); 

            if (isAQSensorAvailable) {
              client.println("<tr><td>Luftdruck</td><td><span class=\"sensor\">");
              client.println(bme.pressure / 100.0);
              client.println(" hPa</span></td></tr>"); 

              client.println("<tr><td>Gas</td><td><span class=\"sensor\">");
              client.println(bme.readGas());
              client.println(" R</span></td></tr>"); 

              client.println("<tr><td>IAQ</td><td><span class=\"sensor\">");
              client.println(air_quality_score);
              client.println(" %</span></td></tr>"); 

              client.println("<tr><td colspan=2><span class=\"sensor\">");
              client.println(CalculateIAQ(air_quality_score));
              client.println("</span></td></tr>"); 
            }
            
            client.println("</body></html>");
            
            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          } else { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        } else if (c != '\r') {  // if you got anything else but a carriage return character,
          currentLine += c;      // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
    
  delay(2000);
}

void GetGasReference(){
  // Now run the sensor for a burn-in period, then use combination of relative humidity and gas resistance to estimate indoor air quality as a percentage.
  Serial.println("Getting a new gas reference value");
  int readings = 10;
  for (int i = 1; i <= readings; i++){ // read gas for 10 x 0.150mS = 1.5secs
    gas_reference += bme.readGas();
  }
  gas_reference = gas_reference / readings;
}

String CalculateIAQ(float score){
  String IAQ_text = "Air quality is ";
  score = (100-score)*5;
  if      (score >= 301)                  IAQ_text += "Hazardous";
  else if (score >= 201 && score <= 300 ) IAQ_text += "Very Unhealthy";
  else if (score >= 176 && score <= 200 ) IAQ_text += "Unhealthy";
  else if (score >= 151 && score <= 175 ) IAQ_text += "Unhealthy for Sensitive Groups";
  else if (score >=  51 && score <= 150 ) IAQ_text += "Moderate";
  else if (score >=  00 && score <=  50 ) IAQ_text += "Good";
  return IAQ_text;
}
