/*
read ppm from scd30 and display on neopixel strip (stick/ring)
*/
#include <Sensirion_GadgetBle_Lib.h>

#include <Wire.h>

//#include <Adafruit_Sensor.h>
//#include "Adafruit_BME680.h"

#include <Adafruit_NeoPixel.h>
#include <ArduinoJson.h>

#include "SparkFun_SCD30_Arduino_Library.h" //Click here to get the library: http://librarymanager/All#SparkFun_SCD30
SCD30 airSensor;

#define LED_PIN 15 // strip is connected to pin
#define LED_COUNT 12 // number of pixels

#define SEALEVELPRESSURE_HPA (1013.25)

#define HIGH_CO2_BOUNDARY 1200
#define LOW_CO2_BOUNDARY 800

//Adafruit_BME680 bme; // I2C

Adafruit_NeoPixel strip = Adafruit_NeoPixel(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);

static int64_t lastMmntTime = 0;
static int startCheckingAfterUs = 1900000;

GadgetBle gadgetBle = GadgetBle(GadgetBle::DataType::T_RH_CO2);

float hum_weighting = 0.25; // so hum effect is 25% of the total air quality score
float gas_weighting = 0.75; // so gas effect is 75% of the total air quality score

float hum_score, gas_score;
float gas_reference = 250000;
float hum_reference = 40;
int getgasreference_count = 0;
int ppm;
int hum;
int temp;

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
	
  if (airSensor.begin() == false)
  {
    Serial.println("Air sensor not detected. Please check wiring. Freezing...");
    while (1)
      ;
  }  
/*
  if (!bme.begin()) {
    Serial.println("Could not find a valid BME680 sensor, check wiring!");
    while (1);
  }
*/
  // Initialize the GadgetBle Library
  gadgetBle.begin();
  Serial.print("Sensirion GadgetBle Lib initialized with deviceId = ");
  Serial.println(gadgetBle.getDeviceIdString());
/*
  // Set up oversampling and filter initialization
  bme.setTemperatureOversampling(BME680_OS_8X);
  bme.setHumidityOversampling(BME680_OS_2X);
  bme.setPressureOversampling(BME680_OS_4X);
  bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
  bme.setGasHeater(320, 150); // 320*C for 150 ms
*/
	
  // set temperature offset for co2-sensor
  airSensor.setTemperatureOffset(4);	
	
  strip.begin();
  strip.setBrightness(64); // set brightness
  strip.show(); // Initialize all pixels to 'off'
}

void loop()
{
  
  if (airSensor.dataAvailable())
  {
    	/*
    Serial.print("co2(ppm):");
    Serial.print(airSensor.getCO2());

    Serial.print(" temp(C):");
    Serial.print(airSensor.getTemperature(), 1);

    Serial.print(" humidity(%):");
    Serial.print(airSensor.getHumidity(), 1);
	*/
    ppm=airSensor.getCO2();
    hum=airSensor.getHumidity();
    temp=airSensor.getTemperature();

    //Serial.println();

    gadgetBle.writeCO2(ppm);
    gadgetBle.writeTemperature(temp);
    gadgetBle.writeHumidity(hum);

    gadgetBle.commit();

    StaticJsonBuffer<200> jsonBuffer;

    JsonObject& root = jsonBuffer.createObject();
    root["sensor"] = "S00001";
    root["co2"] = airSensor.getCO2();
    root["temp"] = airSensor.getTemperature();
    root["humidity"] = airSensor.getHumidity();

    root.printTo(Serial);
    Serial.println("\n");
  }
  
  gadgetBle.handleEvents();
	
	
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

/*
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
  float air_quality_score = hum_score + gas_score;

  Serial.println("Air Quality = "+String(air_quality_score,1)+"% derived from 25% of Humidity reading and 75% of Gas reading - 100% is good quality air");
  Serial.println("Humidity element was : "+String(hum_score/100)+" of 0.25");
  Serial.println("     Gas element was : "+String(gas_score/100)+" of 0.75");
  if (bme.readGas() < 120000) Serial.println("***** Poor air quality *****");
  Serial.println();
  if ((getgasreference_count++)%10==0) GetGasReference(); 
  Serial.println(CalculateIAQ(air_quality_score));
  Serial.println("------------------------------------------------");
*/
    int c;

    //int qs=air_quality_score;
    //int qs=161;

    //if (qs>200) { c=strip.Color(255,0,0); } // red
    //if (qs>150 && qs<200) { c=strip.Color(255,165,0); } // yellow
    //if (qs<150) { c=strip.Color(0,128,0); } // green
    
    if (ppm>HIGH_CO2_BOUNDARY) { c=strip.Color(255,0,0); }
    if (ppm>LOW_CO2_BOUNDARY && ppm <= HIGH_CO2_BOUNDARY) { c=strip.Color(255,165,0); }
    if (ppm<=LOW_CO2_BOUNDARY) { c=strip.Color(0,128,0); }    

    int ct=map(ppm, 350, HIGH_CO2_BOUNDARY, 1, 12);
    //int ct=map(qs, 0, 300, 12, 1);
  
    for(uint16_t i=0; i<strip.numPixels(); i++) {
      if (i<=ct) {
        strip.setPixelColor(i, c);
      } else {
        strip.setPixelColor(i, strip.Color(0,0,0));
      }
    }
    strip.show();    
    
    delay(2000);
}

 /*void GetGasReference(){
  // Now run the sensor for a burn-in period, then use combination of relative humidity and gas resistance to estimate indoor air quality as a percentage.
  Serial.println("Getting a new gas reference value");
  int readings = 10;
  for (int i = 1; i <= readings; i++){ // read gas for 10 x 0.150mS = 1.5secs
    gas_reference += bme.readGas();
  }
  gas_reference = gas_reference / readings;
}*/

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
