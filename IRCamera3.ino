#include <Adafruit_GFX.h>    // Core graphics library
#include "Adafruit_ILI9341.h"
#include "makerfabs_pin.h"
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_MLX90640.h>
#include "PCF8574.h"
#include "FS.h"
#include "SD_MMC.h"

PCF8574 pcf8574(PCF_ADD);

Adafruit_ILI9341 tft = Adafruit_ILI9341(LCD_CS, LCD_DC, LCD_MOSI, LCD_SCK, LCD_RST);

const int w = 32;     // image width in pixels
const int h = 24;     // height

Adafruit_MLX90640 mlx;
float frame[32*24]; // buffer for full frame of temperatures

#define TA_SHIFT 8 //Default shift for MLX90640 in open air

int xPos, yPos;                                // Abtastposition
int R_colour, G_colour, B_colour;              // RGB-Farbwert
int i, j;                                      // Zählvariable
float T_max, T_min;                            // maximale bzw. minimale gemessene Temperatur
float T_center;                                // Temperatur in der Bildschirmmitte

void setup() {
  Serial.begin(115200);

  Serial.println("Adafruit MLX90640 Simple Test");

  Wire.begin(ESP32_SDA, ESP32_SCL);

  key_init();

  if (!SD_MMC.begin("/sdcard", true, false))
    {
        Serial.println("Card Mount Failed");
        return;
    }
    uint8_t cardType = SD_MMC.cardType();

    if (cardType == CARD_NONE)
    {
        Serial.println("No SD_MMC card attached");
        return;
    }

uint64_t cardSize = SD_MMC.cardSize() / (1024 * 1024);
    Serial.printf("SD_MMC Card Size: %lluMB\n", cardSize);
    
  if (! pcf8574.begin()) {
    Serial.println("PCF8574 not found!");
    while (1) delay(10);
 }
  
  if (! mlx.begin(MLX90640_I2CADDR_DEFAULT, &Wire)) {
    Serial.println("MLX90640 not found!");
    while (1) delay(10);
  }
  Serial.println("Found Adafruit MLX90640");

  mlx.setMode(MLX90640_CHESS);
  mlx.setResolution(MLX90640_ADC_18BIT);
  mlx.setRefreshRate(MLX90640_2_HZ);

  pinMode(LCD_CS, OUTPUT);
  pinMode(LCD_DC, OUTPUT);
  pinMode(LCD_BL, OUTPUT);
  digitalWrite(LCD_BL, HIGH);
  
  tft.begin();
  tft.setRotation(1);

  tft.fillScreen(ILI9341_BLACK);
  tft.fillRect(0, 0, 319, 13, tft.color565(255, 0, 10));
  tft.setCursor(25, 3);
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_YELLOW, tft.color565(255, 0, 10));
  tft.print("Thermographie");    

  tft.drawLine(280, 220 - 0, 299, 220 - 0, tft.color565(255, 255, 255));
  tft.drawLine(280, 220 - 30, 299, 220 - 30, tft.color565(255, 255, 255));
  tft.drawLine(280, 220 - 60, 299, 220 - 60, tft.color565(255, 255, 255));
  tft.drawLine(280, 220 - 90, 299, 220 - 90, tft.color565(255, 255, 255));
  tft.drawLine(280, 220 - 120, 299, 220 - 120, tft.color565(255, 255, 255));
  tft.drawLine(280, 220 - 150, 299, 220 - 150, tft.color565(255, 255, 255));
  tft.drawLine(280, 220 - 180, 299, 220 - 180, tft.color565(255, 255, 255));

  tft.setCursor(40, 110);
  tft.setTextColor(ILI9341_WHITE, tft.color565(0, 0, 0));
  tft.print("T+ = ");
// drawing the colour-scale
    // ========================
 
    for (i = 0; i < 181; i++)
       {
        //value = random(180);
        
        getColour(i);
        tft.drawLine(260, 220 - i, 279, 220 - i, tft.color565(R_colour, G_colour, B_colour));
       } 
}

void loop() {
  delay(500);
  if (mlx.getFrame(frame) != 0) {
    Serial.println("Failed");
    return;
  }

  frame[1*32 + 21] = 0.5 * (frame[1*32 + 20] + frame[1*32 + 22]);    // eliminate the error-pixels
    frame[4*32 + 30] = 0.5 * (frame[4*32 + 29] + frame[4*32 + 31]);    // eliminate the error-pixels
    
    T_min = frame[0];
    T_max = frame[0];

    for (i = 1; i < 768; i++)
       {
        if((frame[i] > -41) && (frame[i] < 301))
           {
            if(frame[i] < T_min)
               {
                T_min = frame[i];
               }

            if(frame[i] > T_max)
               {
                T_max = frame[i];
               }
           }
        else if(i > 0)   // temperature out of range
           {
            frame[i] = frame[i-1];
           }
        else
           {
            frame[i] = frame[i+1];
           }
       }

  T_center = frame[11* 32 + 15];    

    // drawing the picture
    // ===================

    for (i = 0 ; i < 24 ; i++)
       {
        for (j = 0; j < 32; j++)
           {
            frame[i*32 + j] = 180.0 * (frame[i*32 + j] - T_min) / (T_max - T_min);
                       
            getColour(frame[i*32 + j]);
            
            tft.fillRect(217 - (32 - j) * 5, 35 + (24 - i) * 5, 5, 5, tft.color565(R_colour, G_colour, B_colour));
           }
       }
    
    //tft.drawLine(217 - 15*7 + 3.5 - 5, 11*7 + 35 + 3.5, 217 - 15*7 + 3.5 + 5, 11*7 + 35 + 3.5, tft.color565(255, 255, 255));
    //tft.drawLine(217 - 15*7 + 3.5, 11*7 + 35 + 3.5 - 5, 217 - 15*7 + 3.5, 11*7 + 35 + 3.5 + 5,  tft.color565(255, 255, 255));
 
    //tft.fillRect(260, 25, 37, 10, tft.color565(0, 0, 0));
    //tft.fillRect(260, 205, 37, 10, tft.color565(0, 0, 0));    
    //tft.fillRect(115, 220, 37, 10, tft.color565(0, 0, 0));    

    tft.setTextColor(ILI9341_WHITE, tft.color565(0, 0, 0));
    tft.setCursor(300, 25);
    tft.print(T_max, 1);
    tft.setCursor(300, 220);
    tft.print(T_min, 1);
    tft.setCursor(300, 120);
    tft.print(T_center, 1);

    //tft.setCursor(300, 25);
    //tft.print("C");
    //tft.setCursor(300, 205);
    //tft.print("C");
    //tft.setCursor(155, 220);
    //tft.print("C");

    PCF8574::DigitalInput val = pcf8574.digitalReadAll();
    if (val.B_START == LOW) {
      tft.setCursor(50, 220);
      tft.print("Button Pressed");
      save_image(SD_MMC);
    } else {
      tft.fillRect(50, 220, 150, 240, tft.color565(0, 0, 0));
    }

}

// ===============================
// ===== determine the colour ====
// ===============================

void getColour(float j)
   {
    if (j >= 0 && j < 30)
       {
        R_colour = 0;
        G_colour = 0;
        B_colour = 20 + (120.0/30.0) * j;
       }
    
    if (j >= 30 && j < 60)
       {
        R_colour = (120.0 / 30) * (j - 30.0);
        G_colour = 0;
        B_colour = 140 - (60.0/30.0) * (j - 30.0);
       }

    if (j >= 60 && j < 90)
       {
        R_colour = 120 + (135.0/30.0) * (j - 60.0);
        G_colour = 0;
        B_colour = 80 - (70.0/30.0) * (j - 60.0);
       }

    if (j >= 90 && j < 120)
       {
        R_colour = 255;
        G_colour = 0 + (60.0/30.0) * (j - 90.0);
        B_colour = 10 - (10.0/30.0) * (j - 90.0);
       }

    if (j >= 120 && j < 150)
       {
        R_colour = 255;
        G_colour = 60 + (175.0/30.0) * (j - 120.0);
        B_colour = 0;
       }

  if (j >= 150 && j <= 180)
  {
     R_colour = 255;
     G_colour = 235 + (20.0/30.0) * (j - 150.0);
     B_colour = 0 + 255.0/30.0 * (j - 150.0);
  }
}

void key_init()
{
    for (int i = 0; i < 8; i++)
    {
        pcf8574.pinMode(i, INPUT);
    }
    pinMode(B_L, INPUT);
    pinMode(B_R, INPUT);
}

int key_scanf()
{
    PCF8574::DigitalInput val = pcf8574.digitalReadAll();
    if (val.B_START == LOW)
        Serial.println("B_START PRESSED");
    if (val.B_SELECT == LOW)
        Serial.println("B_SELECT PRESSED");
    if (val.B_UP == LOW)
        Serial.println("B_UP PRESSED");
    if (val.B_DOWN == LOW)
        Serial.println("B_DOWN PRESSED");
    if (val.B_LEFT == LOW)
        Serial.println("B_LEFT PRESSED");
    if (val.B_RIGHT == LOW)
        Serial.println("B_RIGHT PRESSED");
    if (val.B_A == LOW)
        Serial.println("B_A PRESSED");
    if (val.B_B == LOW)
        Serial.println("B_B PRESSED");
    if (digitalRead(B_L) == LOW)
        Serial.println("B_L PRESSED");
    if (digitalRead(B_R) == LOW)
        Serial.println("B_R PRESSED");

    return 0;
}

void save_image(fs::FS &fs)
{
  byte VH, VL;
  int i, j = 0;
  
  File file = fs.open("/image.bmp", FILE_WRITE);
    if (!file)
    {
        Serial.println("Failed to open file for writing");
        return;
    }

    unsigned char bmFlHdr[14] = {
    'B', 'M', 0, 0, 0, 0, 0, 0, 0, 0, 54, 0, 0, 0
  };
  // 54 = std total "old" Windows BMP file header size = 14 + 40
  
  unsigned char bmInHdr[40] = {
    40, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 16, 0
  };   
  // 40 = info header size
  //  1 = num of color planes
  // 16 = bits per pixel
  // all other header info = 0, including RI_RGB (no compr), DPI resolution

  unsigned long fileSize = 2ul * h * w + 54; // pix data + 54 byte hdr
  
  bmFlHdr[ 2] = (unsigned char)(fileSize      ); // all ints stored little-endian
  bmFlHdr[ 3] = (unsigned char)(fileSize >>  8); // i.e., LSB first
  bmFlHdr[ 4] = (unsigned char)(fileSize >> 16);
  bmFlHdr[ 5] = (unsigned char)(fileSize >> 24);

  bmInHdr[ 4] = (unsigned char)(       w      );
  bmInHdr[ 5] = (unsigned char)(       w >>  8);
  bmInHdr[ 6] = (unsigned char)(       w >> 16);
  bmInHdr[ 7] = (unsigned char)(       w >> 24);
  bmInHdr[ 8] = (unsigned char)(       h      );
  bmInHdr[ 9] = (unsigned char)(       h >>  8);
  bmInHdr[10] = (unsigned char)(       h >> 16);
  bmInHdr[11] = (unsigned char)(       h >> 24);

  file.write(bmFlHdr, sizeof(bmFlHdr));
  file.write(bmInHdr, sizeof(bmInHdr));

  for (i = h; i > 0; i--) {
    for (j = 0; j < w; j++) {

      // uint16_t rgb = readPixA(j,i); get pix color in rgb565 format

      getColour(frame[i*32 + j]);

uint16_t rgb = 0;
  rgb = R_colour >> 3; // red: use 5 highest bits (discard three LSB)
  rgb = (rgb << 6) | G_colour >> 2; // green: use 6 highest bits (discard two LSB)
  rgb = (rgb << 5) | B_colour >> 3; // blue: use 5 highest bits (discard three LSB)
      
      VH = (rgb & 0xFF00) >> 8; // High Byte
      VL = rgb & 0x00FF;        // Low Byte
      
      //RGB565 to RGB555 conversion... 555 is default for uncompressed BMP
      //this conversion is from ...topic=177361.0 and has not been verified
      VL = (VH << 7) | ((VL & 0xC0) >> 1) | (VL & 0x1f);
      VH = VH >> 1;
      
      //Write image data to file, low byte first
      file.write(VL);
      file.write(VH);
    }
  }
  //Close the file
  file.close();
}
