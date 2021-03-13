/* 
 * Created 02/26/2021 by Andrew Ryan
 * GNU Public License
 * 
 */

#include <LiquidCrystal.h>
#include <EEPROM.h>
#define RELAY_PUMP_PIN A1 // Arduino pin connected to relay which connected to fan, temp to LED
#define BLUE 6
#define GREEN 5
#define RED 3
int tempPin = A0;
LiquidCrystal lcd(7, 8, 9, 10, 11, 12);

int TEMP_TARGET; // upper threshold of temperature, change to your desire value
const int buttonPinUp = 2; //pin location for up button
const int buttonPinDown = 4;  //pin location for down button
boolean pumpStatus = false;

unsigned long previousRGB = 0; //store original time for refresh
unsigned long previousTemp = -5000; //store original time for refresh


void setup()
{
    //Serial.begin(9600); // initialize serial
    //load target temp from eeprom
    TEMP_TARGET = EEPROM.read(0);
    if (((TEMP_TARGET >= 100) || (TEMP_TARGET <= 9))){
      TEMP_TARGET = 70;
    }

    //init buttons
    pinMode(buttonPinUp, INPUT);
    pinMode(buttonPinDown, INPUT);
    pinMode(RELAY_PUMP_PIN, OUTPUT);
    
    //init splashscreen
    lcd.begin(16, 2);
    lcd.setCursor(3, 0);
    lcd.print("Lets make");
    lcd.setCursor(5, 1);
    lcd.print("booze");
    delay(3000);
    lcd.clear();
    lcd.begin(16, 2);
    lcd.setCursor(0, 0);
    lcd.print("8======D");
    lcd.setCursor(13, 1);
    lcd.print("lol");
    delay(2500);
    lcd.clear();

    //rgb init
    pinMode(RED, OUTPUT);
    pinMode(GREEN, OUTPUT);
    pinMode(BLUE, OUTPUT);
    


}
void loop()
{
  unsigned long now = millis();

  if (pumpStatus == true && (now - previousRGB >= 35)){
    fade();
    previousRGB = now;
    
  }

  if (now - previousTemp >= 5000){
    tempCheck();
    previousTemp = now;
  }

  //call temp change routine
  if ((digitalRead(buttonPinUp) == HIGH) || (digitalRead(buttonPinDown) == HIGH)){
    changeTemp();
  }
}

void tempCheck(){
   int avgTemp;    // temperature in F
  
   avgTemp = getTemp2();
   printTemps(avgTemp);
   evaluatePumpStatus(avgTemp, TEMP_TARGET);
}


//get average tempfrom probe
float getAverage(){

int total = 0;                  // the running total
int average = 0;                // the average
const int numReadings = 20;  //readings to average when gathering temp


for (int thisReading = 0; thisReading < numReadings; thisReading++) {
     total = total + analogRead(tempPin);
  };

  return (total/numReadings);
}


//gather average temp and convert to kelvin->  farenheit
//int getTemp(){
//  double tempK = log(10000.0 * ((1024.0 / getAverage() - 1)));
//  tempK = 1 / (0.001129148 + (0.000234125 + (0.0000000876741 * tempK * tempK )) * tempK );       //  Temp Kelvin
//  return (((tempK - 273.15) * 9.0)/ 5.0 + 32.0); // Convert Celcius to Fahrenheit and return value;
//}

int getTemp2(){
  double tempK = log(10000.0 * ((1023.0 / getAverage() - 1)));
  tempK = (1.0 / (1.009249522e-03 + 2.378405444e-04*tempK + 2.019202697e-07*tempK*tempK*tempK));       //  Temp Kelvin
  return (((tempK - 273.15) * 9.0)/ 5.0 + 32.0); // Convert Celcius to Fahrenheit and return value;
}

//print current target temp and actual temp
void printTemps(int avgTemp){
  lcd.setCursor(0, 0);
  lcd.print("TT:   F");
  lcd.setCursor(4, 0);
  lcd.print(TEMP_TARGET);
  lcd.setCursor(0, 1);
  lcd.print("AT:   F");
  lcd.setCursor(4, 1);
  lcd.print(avgTemp);
}

//determine if pump needs to turn on or off, built with 2 degree buffer
void evaluatePumpStatus(int tempIn, int target){
  if(tempIn > (target + 2)){
      digitalWrite(RELAY_PUMP_PIN, HIGH); // turn on
      pumpStatus = true;
      lcd.setCursor(9, 0);
      lcd.print("PUMPING");
    } 
    else if(tempIn <= (target - 2)){
      digitalWrite(RELAY_PUMP_PIN, LOW); // turn off
      pumpStatus = false;
      setColourRgb(0, 0, 0);
      lcd.setCursor(9, 0);
      lcd.print("       ");
    };
}

//user initiate temp change routine
void changeTemp(){
  unsigned long initSetMillis;
  unsigned long termSetMillis;
  int newTemp = TEMP_TARGET;
  unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
  unsigned long debounceDelay = 250;
  
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SET NEW TEMP: ");
  lcd.setCursor(0, 1);
  lcd.print(newTemp);
  lcd.blink();
  
  initSetMillis = millis();
  termSetMillis = initSetMillis + 5000;
  lastDebounceTime = millis();
  
  while (initSetMillis < termSetMillis){
  
    if (digitalRead(buttonPinUp) == HIGH){      
      if ((millis() - lastDebounceTime) > debounceDelay) {
        setColourRgb(255, 0, 0);
        newTemp += 1;
        if (newTemp >= 100)
          newTemp = 10;
        lcd.setCursor(0, 1);
        lcd.print(newTemp);
        termSetMillis = millis() + 5000;
        lastDebounceTime = millis();
      }
    }
    
    if (digitalRead(buttonPinDown) == HIGH){

      if ((millis() - lastDebounceTime) > debounceDelay) {
        setColourRgb(0, 0, 255);
        newTemp -= 1;
        if (newTemp <= 9)
          newTemp = 99;
        lcd.setCursor(0, 1);
        lcd.print(newTemp);
        termSetMillis = millis() + 5000;
        lastDebounceTime = millis();
      }
    }
    initSetMillis = millis();
  }
  lcd.noBlink();
  TEMP_TARGET = newTemp;
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("SAVING NEW TEMP");
  EEPROM.write(0, TEMP_TARGET);
  delay(1000);
  lcd.clear();
  setColourRgb(0, 0, 0);
}


void fade()
{
 
  static unsigned int rgbColour[3] = {255,0,0}; //Start on red
  static int incColour = 1;
  static int decColour = 0;
  static int i = -1;
     
      // cross-fade the two colours.
      i++;
      if(i > 254) {
        i=0;
        decColour ++;
        if(decColour >2) decColour = 0;     
        if (decColour == 2)  incColour = 0;
        else incColour = decColour +1;
      }

       
        rgbColour[decColour] -= 1;
        rgbColour[incColour] += 1;
       
        setColourRgb(rgbColour[0], rgbColour[1], rgbColour[2]);   
   }

void setColourRgb(unsigned int red, unsigned int green, unsigned int blue) {
  analogWrite(RED, red);
  analogWrite(GREEN, green);
  analogWrite(BLUE, blue);
 }
