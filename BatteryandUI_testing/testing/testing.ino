#include <Wire.h>
#include <TFT_eSPI.h>  
#define debug  Serial
TFT_eSPI    tft = TFT_eSPI(); 
TFT_eSprite Display = TFT_eSprite(&tft);
uint16_t TheColor;
// start with some initial colors
uint16_t MinTemp = 25;
uint16_t MaxTemp = 38;
 
// variables for interpolated colors
byte red, green, blue;
 
// variables for row/column interpolation
byte i, j, k, row, col, incr;
float intPoint, val, a, b, c, d, ii;
byte aLow, aHigh;
 
// size of a display "pixel"
byte BoxWidth = 3;
byte BoxHeight = 3;
 
int x, y;
char buf[20];

// int battPin = A0

//uint32_t targetTime = 0; 
void setup() {
   Wire.begin();
   Wire.setClock(2000000); //Increase I2C clock speed to 2M
   debug.begin(115200); //Fast debug as possible
   // pinMode(battPin, INPUT);
   
   tft.begin();
   tft.setRotation(3);
   tft.fillScreen(TFT_BLACK);
   Display.createSprite(TFT_HEIGHT, TFT_WIDTH);
   Display.fillSprite(TFT_BLACK); 
   tft.setTextColor(TFT_WHITE, TFT_BLACK);
   
   DrawLegend();

//   targetTime = millis() + 1000;
}

void loop() {
  // put your main code here, to run repeatedly:
  BatteryIndicator();
}

void BatteryIndicator(){
  // int voltReading = analogRead(battPin);
  // int volts = (voltReading/1023)*100;
  int volts = 100;

  if (volts == 100)
   {
     tft.setCursor(270,0);
     tft.print(volts);
     tft.print("%");
     delay(1000);    
   }
   else if (volts < 10)
   {
     tft.setCursor(272,0);
     tft.print(volts);
     tft.print("%");
     delay(1000); 
   }
   else
   {
     tft.setCursor(271,0);
     tft.print(volts);
     tft.print("%");
     delay(1000);     
   }
     
  
  
}

uint16_t GetColor(float val) {
 
  /*
    pass in value and figure out R G B
    several published ways to do this I basically graphed R G B and developed simple linear equations
    again a 5-6-5 color display will not need accurate temp to R G B color calculation
 
    equations based on
    http://web-tech.ga-usa.com/2012/05/creating-a-custom-hot-to-cold-temperature-color-gradient-for-use-with-rrdtool/index.html
 
  */
 
  red = constrain(255.0 / (c - b) * val - ((b * 255.0) / (c - b)), 0, 255);
 
  if ((val > MinTemp) & (val < a)) {
    green = constrain(255.0 / (a - MinTemp) * val - (255.0 * MinTemp) / (a - MinTemp), 0, 255);
  }
  else if ((val >= a) & (val <= c)) {
    green = 255;
  }
  else if (val > c) {
    green = constrain(255.0 / (c - d) * val - (d * 255.0) / (c - d), 0, 255);
  }
  else if ((val > d) | (val < a)) {
    green = 0;
  }
 
  if (val <= b) {
    blue = constrain(255.0 / (a - b) * val - (255.0 * b) / (a - b), 0, 255);
  }
  else if ((val > b) & (val <= d)) {
    blue = 0;
  }
  else if (val > d) {
    blue = constrain(240.0 / (MaxTemp - d) * val - (d * 240.0) / (MaxTemp - d), 0, 240);
  }
 
  // use the displays color mapping function to get 5-6-5 color palet (R=5 bits, G=6 bits, B-5 bits)
  return Display.color565(red, green, blue);
 
}

void DrawLegend() {
 
  //color legend with max and min text
  j = 0;
 
  float inc = (MaxTemp - MinTemp ) / 160.0;
 
  for (ii = MinTemp; ii < MaxTemp; ii += inc) {
    tft.drawFastHLine(260, 200 - j++, 30, GetColor(ii));
  }
 
  tft.setTextSize(1);
  tft.setCursor(260, 20);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  sprintf(buf, "%2d/%2d", MaxTemp, (int) (MaxTemp * 1.12) + 32);
  tft.print(buf);
 
  tft.setTextSize(1);
  tft.setCursor(260, 210);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  sprintf(buf, "%2d/%2d", MinTemp, (int) (MinTemp * 1.12) + 32);
  tft.print(buf);
 
}
