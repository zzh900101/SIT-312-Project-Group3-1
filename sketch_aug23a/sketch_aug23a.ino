#include <Wire.h>
#include "MLX90641_API.h"
#include "MLX9064X_I2C_Driver.h"
#include <TFT_eSPI.h>  
 
const byte MLX90641_address = 0x33;
#define TA_SHIFT 12 
#define debug  Serial
#define COLOR_DEPTH 8

uint16_t eeMLX90641[832];
float MLX90641To[192];
uint16_t MLX90641Frame[242];
paramsMLX90641 MLX90641;
int errorno = 0;
 
TFT_eSPI    tft = TFT_eSPI(); 
TFT_eSprite Display = TFT_eSprite(&tft);
 
unsigned long CurTime;
 
uint16_t TheColor;
// start with some initial colors
uint16_t Min_DetectTemp = 15;
uint16_t Max_DetectTemp = 40;
 
byte red, green, blue;
 
byte i, j, k, row, col, incr;
float intPoint, val, a, b, c, d, ii;
byte aLow, aHigh;
 
// size of a display "pixel"
byte BoxWidth = 3;
byte BoxHeight = 3;
 
int x, y;
char buf[20];
 
float HDTemp[6400];
 
void setup() {
    Wire.begin();
    Wire.setClock(2000000);
    debug.begin(115200);
 
 
    if (isConnected() == false) {
        debug.println("MLX90641 not detected");
        while (1);
    }
    //Get device parameters - We only have to do this once
    int status;
    status = MLX90641_DumpEE(MLX90641_address, eeMLX90641);
    errorno = status;
 
    if (status != 0) {
        debug.println("Failed to load system parameters");
       while(1);
    }
 
    status = MLX90641_ExtractParameters(eeMLX90641, &MLX90641);
    //errorno = status;
    if (status != 0) {
        debug.println("Parameter extraction failed");
        while(1);
    }
 

    MLX90641_SetRefreshRate(MLX90641_address, 0x05); //Set sensor  refresh rate(4=8HZ,5=16Hz,6=32Hz,7=64Hz)
 
    tft.begin();
    tft.setRotation(3);
    tft.fillScreen(TFT_BLACK);

    Display.setColorDepth(COLOR_DEPTH);
    
    Display.createSprite(TFT_HEIGHT, TFT_WIDTH);
    Display.fillSprite(TFT_BLACK); 
    tft.setTextColor(TFT_WHITE, TFT_BLACK);

    Get_abcd();
 
    Draw_LeftBar();    
}
void loop() {
      BatteryIndicator();

    // draw a large white border for the temperature area
    Display.fillRect(10, 10, 220, 220, TFT_WHITE);
    for (byte x = 0 ; x < 2 ; x++) {
        int status = MLX90641_GetFrameData(MLX90641_address, MLX90641Frame);
 
        float vdd = MLX90641_GetVdd(MLX90641Frame, &MLX90641);
        float Ta = MLX90641_GetTa(MLX90641Frame, &MLX90641);
 
        float tr = Ta - TA_SHIFT; //Reflected temperature based on the sensor ambient temperature
        float emissivity = 0.95;
 
        MLX90641_CalculateTo(MLX90641Frame, &MLX90641, emissivity, tr, MLX90641To);
    }
 
    interpolate_image(MLX90641To,12,16,HDTemp,80,80);
 
    //display the 80 x 80 array
    DisplayGradient();
 
    //Crosshair in the middle of the screen
    Crosshair_display();

    //Displaying the temp at the middle of the Screen
 
    Display.pushSprite(0, 0);//(x=0,y=0)
 
    tft.setRotation(3);
    tft.setTextColor(TFT_WHITE);
    tft.drawFloat(HDTemp[35 * 80 + 35], 2, 90, 20);        
}

void Crosshair_display()
{
    Display.drawCircle(115, 115, 5, TFT_WHITE);
    Display.drawFastVLine(115, 105, 20, TFT_WHITE);
    Display.drawFastHLine(105, 115, 20, TFT_WHITE);
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

boolean isConnected() {
    Wire.beginTransmission((uint8_t)MLX90641_address);
    if (Wire.endTransmission() != 0) {
        return (false);    //Sensor did not ACK
    }
    return (true);
}
// function to display the results
void DisplayGradient() {
 
  tft.setRotation(4);
 
  for (row = 0; row < 70; row ++) {
    for (col = 0; col < 70; col++) {
      // finally we can draw each the 70 x 70 points, note the call to get interpolated color
      Display.fillRect((row * 3) + 15, (col * 3) + 15, BoxWidth, BoxHeight, Color_Display(HDTemp[row * 80 + col]));
    }
  }
}

uint16_t Color_Display(float val) {
 
  red = constrain(255.0 / (c - b) * val - ((b * 255.0) / (c - b)), 0, 255);
 
  if ((val > Min_DetectTemp) & (val < a)) {
    green = constrain(255.0 / (a - Min_DetectTemp) * val - (255.0 * Min_DetectTemp) / (a - Min_DetectTemp), 0, 255);
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
    blue = constrain(240.0 / (Max_DetectTemp - d) * val - (d * 240.0) / (Max_DetectTemp - d), 0, 240);
  }
 
  return Display.color565(red, green, blue);
 
}
 
void Get_abcd() {
  a = Min_DetectTemp + (Max_DetectTemp - Min_DetectTemp) * 0.2121;
  b = Min_DetectTemp + (Max_DetectTemp - Min_DetectTemp) * 0.3182;
  c = Min_DetectTemp + (Max_DetectTemp - Min_DetectTemp) * 0.4242;
  d = Min_DetectTemp + (Max_DetectTemp - Min_DetectTemp) * 0.8182;
}
float Get_Point(float *p, uint8_t rows, uint8_t cols, int8_t x, int8_t y)
{
    if (x < 0)
    {
        x = 0;
    }
    if (y < 0)
    {
        y = 0;
    }
    if (x >= cols)
    {
        x = cols - 1;
    }
    if (y >= rows)
    {
        y = rows - 1;
    }
    return p[y * cols + x];
}
 
void set_point(float *p, uint8_t rows, uint8_t cols, int8_t x, int8_t y, float f)
{
    if ((x < 0) || (x >= cols))
    {
        return;
    }
    if ((y < 0) || (y >= rows))
    {
        return;
    }
    p[y * cols + x] = f;
}

void interpolate_image(float *src, uint8_t src_rows, uint8_t src_cols,
                       float *dest, uint8_t dest_rows, uint8_t dest_cols)
{
    float mu_x = (src_cols - 1.0) / (dest_cols - 1.0);
    float mu_y = (src_rows - 1.0) / (dest_rows - 1.0);
 
    float adj_2d[16];
 
    for (uint8_t y_idx = 0; y_idx < dest_rows; y_idx++)
    {
        for (uint8_t x_idx = 0; x_idx < dest_cols; x_idx++)
        {
            float x = x_idx * mu_x;
            float y = y_idx * mu_y;
            get_adjacents_2d(src, adj_2d, src_rows, src_cols, x, y);
 
            float frac_x = x - (int)x; 
            float frac_y = y - (int)y; 
            float out = bicubicInterpolate(adj_2d, frac_x, frac_y);
            set_point(dest, dest_rows, dest_cols, x_idx, y_idx, out);
        }
    }
}
 
float cubicInterpolate(float p[], float x)
{
    float r = p[1] + (0.5 * x * (p[2] - p[0] + x * (2.0 * p[0] - 5.0 * p[1] + 4.0 * p[2] - p[3] + x * (3.0 * (p[1] - p[2]) + p[3] - p[0]))));
    return r;
}
 
// p is a 16-point 4x4 array of the 2 rows & columns left/right/above/below
float bicubicInterpolate(float p[], float x, float y)
{
    float arr[4] = {0, 0, 0, 0};
    arr[0] = cubicInterpolate(p + 0, x);
    arr[1] = cubicInterpolate(p + 4, x);
    arr[2] = cubicInterpolate(p + 8, x);
    arr[3] = cubicInterpolate(p + 12, x);
    return cubicInterpolate(arr, y);
}
 
void get_adjacents_1d(float *src, float *dest, uint8_t rows, uint8_t cols, int8_t x, int8_t y)
{
    dest[0] = Get_Point(src, rows, cols, x - 1, y);
    dest[1] = Get_Point(src, rows, cols, x, y);
    dest[2] = Get_Point(src, rows, cols, x + 1, y);
    dest[3] = Get_Point(src, rows, cols, x + 2, y);
}
 
void get_adjacents_2d(float *src, float *dest, uint8_t rows, uint8_t cols, int8_t x, int8_t y)
{
    float arr[4];
    for (int8_t delta_y = -1; delta_y < 3; delta_y++)
    {                                          // -1, 0, 1, 2
        float *row = dest + 4 * (delta_y + 1); // index into each chunk of 4
        for (int8_t delta_x = -1; delta_x < 3; delta_x++)
        { // -1, 0, 1, 2
            row[delta_x + 1] = Get_Point(src, rows, cols, x + delta_x, y + delta_y);
        }
    }
}
 
void Draw_LeftBar() {
 
  j = 0;
 
  float inc = (Max_DetectTemp - Min_DetectTemp ) / 160.0;
 
  for (ii = Min_DetectTemp; ii < Max_DetectTemp; ii += inc) {
    tft.drawFastHLine(260, 200 - j++, 30, Color_Display(ii));
  }
 
  tft.setTextSize(1);
  tft.setCursor(260, 20);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print("HOT");
 
  tft.setTextSize(1);
  tft.setCursor(260, 210);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.print("COLD");
}
