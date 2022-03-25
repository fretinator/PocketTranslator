//#include <TSC2004.h>

#include <BlockDriver.h>
#include <FreeStack.h>
#include <MinimumSerial.h>
#include <SdFat.h>
#include <SdFatConfig.h>
#include <SysCall.h>
#include <sdios.h>

#include <Adafruit_GFX.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_ILI9341.h>
#include <Adafruit_NeoPixel.h>
#include <BBQ10Keyboard.h>

// For API Call
#include "translation_api.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <esp_system.h>

//#define STMPE_CS 32 //6
#define TFT_CS 15 //9
#define TFT_DC 33 //10
#define SD_CS 14 //5
#define NEOPIXEL_PIN 27 //11

/*
#define TS_MINX 150
#define TS_MINY 130
#define TS_MAXX 3800
#define TS_MAXY 4000

#define TOUCH_DELAY 50
*/

// Homescreen related
#define BACK_COLOR ILI9341_WHITE
#define SRC_SECTION_LABEL "ENTER ENGLISH PHRASE"
#define TRANS_SECTION_LABEL "TRANSLATION"
#define BTN1_LABEL "Tra"
#define BTN2_LABEL "Rst"
#define BTN3_LABEL "Sav"
#define BTN4_LABEL "Lod"
#define FONT_SIZE 2
#define CHAR_WIDTH 10
#define CHAR_HEIGHT 16
#define LINE_PAD 4
#define CHAR_PAD 2
#define SRC_LINES 4
#define BORDER_PAD 4
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define BACK_COLOR ILI9341_BLACK
#define SRC_TEXT_COLOR ILI9341_WHITE
#define RECT_COLOR ILI9341_BLUE
#define CURSOR_COLOR ILI9341_GREEN

int translationPosY;

struct pixel_pos {
  int x;
  int y;
} currentCharPos;

int currentCharacter = 0;

const int FIRST_LINE_Y = BORDER_PAD + CHAR_HEIGHT + 2 * LINE_PAD;
const int FIRST_CHAR_X = 3 * BORDER_PAD;

// CONTROL CHARACTER VALUES
#define KEY_BS     8
#define KEY_B1     6
#define KEY_B2    17
#define KEY_B3     7
#define KEY_B4    18
#define KEY_LEFT   3
#define KEY_UP     1
#define KEY_RIGHT  4
#define KEY_DOWN   2
#define KEY_JPRESS 5
#define KEY_ENTER 10

#define CONTROL_KEYS_HANDLED 11
uint controlCharVals[CONTROL_KEYS_HANDLED] = {KEY_BS, KEY_B1, KEY_B2, KEY_B3,
  KEY_B4, KEY_LEFT, KEY_UP, KEY_RIGHT, KEY_DOWN, KEY_JPRESS, KEY_ENTER};

// If I decide to save translations
#define MAX_FILENAME 25 //24 + 1 for null
char fName[MAX_FILENAME];

// Source-string related
#define MAX_SRC_STRING 100
#define MAX_LINES 4 
const uint CHARS_PER_LINE = MAX_SRC_STRING / MAX_LINES;
char srcString[MAX_SRC_STRING + 1]; // one for null character
//TSC2004 ts;
Adafruit_ILI9341 tft(TFT_CS, TFT_DC);
Adafruit_NeoPixel pixels(1, NEOPIXEL_PIN, NEO_GRB + NEO_KHZ800);
BBQ10Keyboard keyboard;

// File system object.
SdFat sd;
boolean sdOK = true;
//boolean touchAvailable = true;

// Directory file.
SdFile root;

// Use for file creation in folders.
SdFile entry;

void initSourceString() {
  
  // Initialize source string
  for(int x = 0; x < MAX_SRC_STRING; x++) {
    srcString[x] = ' ';
  }

  // srcString is size MAX_SRC_STRING + 1
  srcString[MAX_SRC_STRING] = '\0';
}

bool connectToWiFi() {
  WiFi.begin(TranslationAPI::my_ssid, TranslationAPI::my_password);
  uint attempts = 0;

  tft.print("Connecting to ");
  tft.print(TranslationAPI::my_ssid);

  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
      delay(500);
      tft.print(".");
      attempts++;
  }

  if(WiFi.status() != WL_CONNECTED) {
    tft.print(" Connection failed!");
    return false;
  }

 tft.println("\nWiFi connected! ");
 tft.print("IP address: ");
 tft.print(WiFi.localIP().toString());
 delay(2000);
 return true;
}

void initScreen() {  
  Serial.println("Opening screen...");
  tft.begin();
  tft.setRotation(1);
  tft.fillScreen(BACK_COLOR);
  tft.setTextColor(SRC_TEXT_COLOR);  
  tft.setTextSize(2);
}
void drawButtonLabels() {
  uint btnY = SCREEN_HEIGHT - (2 * BORDER_PAD) - CHAR_HEIGHT;
  uint btnX = BORDER_PAD; // For first button;
  uint btnW = (3 * CHAR_WIDTH) + (4 * CHAR_PAD);
  uint btnH = CHAR_HEIGHT + (2 * CHAR_PAD);

  tft.fillRect(btnX, btnY, btnW, btnY, RECT_COLOR);
  tft.setCursor(btnX + CHAR_PAD, btnY + CHAR_PAD);
  tft.print(BTN1_LABEL);

  btnX += 2 * btnW; 

  tft.fillRect(btnX, btnY, btnW, btnY, RECT_COLOR);
  tft.setCursor(btnX + CHAR_PAD, btnY + CHAR_PAD);
  tft.print(BTN2_LABEL);  
}

void drawHomeScreen() {

  // Clear screen
  tft.fillScreen(BACK_COLOR);

  tft.setCursor(BORDER_PAD, BORDER_PAD);
  tft.print("Text to translate:");

  // Draw a rectangle around the source input aread 
  // Top of rectangle is bottom of above row of text
  // plus line_pad
  int rectTop = tft.getCursorY() + CHAR_HEIGHT + LINE_PAD;
  int rectHeight = MAX_LINES * CHAR_HEIGHT + (MAX_LINES + 2) * LINE_PAD;
  int rectWidth = SCREEN_WIDTH - (2 * BORDER_PAD);
  tft.drawRect(BORDER_PAD, rectTop, rectWidth, rectHeight, RECT_COLOR);

  translationPosY = rectTop + rectHeight + 2 * LINE_PAD;

  drawButtonLabels();
  moveCursor(0);
}

void setup()
{
  Serial.begin(9600);

  Serial.println("Starting setup...");

  Wire.begin();

  initSourceString();

  /*
    Serial.println("Starting touch...");
  
  pinMode(STMPE_CS, OUTPUT);


  if(!ts.begin()) {
    Serial.println("Touch not available.");
    touchAvailable= false;
  }
  */

  Serial.println("Starting NeoPixel...");
  pixels.begin();
  pixels.setBrightness(10);
  
  Serial.println("Starting keyboard...");
  keyboard.begin();
  keyboard.setBacklight(0.5f);

  initScreen();

  if(!connectToWiFi()) {
    // No use going on
    while(1);
  }

  esp_random();

  drawHomeScreen();

  //tft.print("Hello FeatherWing!\n");
  //tft.print("Touch to paint, type to... type\n");

  // List SD card files if available
  //pinMode(33, OUTPUT);
  //digitalWrite(33, HIGH);

  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);


  Serial.println("Opening SD Card...");
  if(!sd.begin(SD_CS)) {
    Serial.println("SD not available.");
    sdOK = false;
  } else {
    Serial.println("Attempting to open SD root filesystem...");
    if(!root.open("/")) {
      Serial.println("Error opening root filesystem.");
    } else {
      
      Serial.println("Listing files on SD card...");
      //tft.print("SD Card contents:\n");

      
      while (entry.openNext(&root, O_RDONLY)) {
        entry.getName(fName, MAX_FILENAME);
        Serial.print("\t");
        Serial.println(fName);    
        //tft.println(fName);
        entry.close();
      }
    }
  }


  Serial.println("Initialization complete.");
}

void handleKey(char key) {
  uint keyVal = (uint)key;

  if(isControlValue(keyVal)) {
    handleControlValue(keyVal);
  } else {
    handleCharacter(key);
  }
}

void submitTranslation() {
  String substr = String(srcString);
  substr.trim();

  Serial.print("String to submit is: ");
  Serial.println(substr);

  if(substr.length() > 0) {
    String result = TranslationAPI::getTranslation(substr);
    tft.setCursor(BORDER_PAD, translationPosY);
    tft.print(result);
  }
}

void handleReset() {
  currentCharacter = 0;
  initSourceString();
  drawHomeScreen();
}


void handleSave() {

}


void handleLoad() {

}

void handleButtonPress(uint btnVal) {
  switch(btnVal) {
    case KEY_B1:
      submitTranslation();

      break;
    case KEY_B2:
      handleReset();

      break;
    case KEY_B3:
      handleSave();

      break;
    case KEY_B4:
      handleLoad();

      break;
    default:
      Serial.print("Unknown btnVal: ");
      Serial.println(btnVal);
      break;
  }
}

// Include enter key and Joystick Enter
void handleArrowPress(uint arrowVal) {
  switch(arrowVal) {
    case KEY_LEFT:
      if(currentCharacter > 0) {
        currentCharacter--;
        moveCursor(currentCharacter);
      }
      
      break;

    case KEY_UP:
      if(currentCharacter >= CHARS_PER_LINE) {
        currentCharacter -= CHARS_PER_LINE;
        moveCursor(currentCharacter);
      }
      break;
    case KEY_DOWN:
      if(currentCharacter < (MAX_SRC_STRING - CHARS_PER_LINE)) {
        currentCharacter += CHARS_PER_LINE;
        moveCursor(currentCharacter);
      }
      break;

    case KEY_RIGHT:
      if(currentCharacter < MAX_SRC_STRING) {
         currentCharacter++;
         moveCursor(currentCharacter);
      }
      break;
  }
}
 
// Returns topLeft position of where to set cursor to draw character in
// source string
void setSourceCharPos(uint charNum) {
  if(charNum <= MAX_SRC_STRING) {
    int rowNum = charNum / CHARS_PER_LINE; 
    int charPos = charNum % CHARS_PER_LINE;

    currentCharPos.x = FIRST_CHAR_X + (charPos * (CHAR_WIDTH + CHAR_PAD));
    currentCharPos.y = FIRST_LINE_Y + (rowNum * (CHAR_HEIGHT + LINE_PAD));
  }
}


void getCharPos(uint charNum, int& charPosX, int& charPosY) {
    int rowNum = charNum / CHARS_PER_LINE; 
    int charPos = charNum % CHARS_PER_LINE;

    charPosX = FIRST_CHAR_X + (charPos * (CHAR_WIDTH + CHAR_PAD));
    charPosY = FIRST_LINE_Y + (rowNum * (CHAR_HEIGHT + LINE_PAD));  

}

// Draw character from source string at charNum position
// to matching position on screen
void drawCharacter(uint charNum) {
  if(charNum < MAX_SRC_STRING) {
    // Erase previous character
    setSourceCharPos(charNum);
    tft.fillRect(currentCharPos.x, currentCharPos.y, CHAR_WIDTH, 
      CHAR_HEIGHT, BACK_COLOR);
    tft.setCursor(currentCharPos.x, currentCharPos.y);
    tft.print(srcString[charNum]);
  }
}

// Draw a line under where next character will go
void setCursorPos(uint characterPos) {

  setSourceCharPos(characterPos);

  // Daw line 2 pixels under character
 int startx = currentCharPos.x;
  // Daw line 2 pixels under character
  int starty = currentCharPos.y + CHAR_HEIGHT + 2; 

  tft.drawLine(startx, starty, startx + CHAR_WIDTH, starty, CURSOR_COLOR);
}

void moveCursor(uint newCursorChar) {
  eraseCurrentCursor();
  setCursorPos(newCursorChar);
}

boolean eraseCurrentCursor() {
  int startx = currentCharPos.x;
  // Daw line 2 pixels under character
  int starty = currentCharPos.y + CHAR_HEIGHT + 2; 

  tft.drawLine(startx, starty, startx + CHAR_WIDTH, starty, BACK_COLOR);
}

bool isControlValue(uint charVal) {
  for(int x = x; x < CONTROL_KEYS_HANDLED; x++) {
    if(controlCharVals[x] == charVal) {
      return true;
    }
  }
  return false;
}

void doErase() {
  if(currentCharacter > 0) {
    currentCharacter--;
    moveCursor(currentCharacter);
    srcString[currentCharacter] = ' ';
    drawCharacter(currentCharacter);
    }
}


void handleEnterKey() {
// If we are no already on last line
 if(currentCharacter < (MAX_SRC_STRING - CHARS_PER_LINE)) {
        // Move to beginning of next line
        int curRow =  currentCharacter / CHARS_PER_LINE; 
        curRow++;
        currentCharacter = curRow * CHARS_PER_LINE;
        moveCursor(currentCharacter);
      }  
}

void handleControlValue(uint charVal) {
  switch(charVal) {
    
    case KEY_BS:
      doErase();
      break;
       
    case KEY_B1:
    case KEY_B2:
    case KEY_B3:
    case KEY_B4:
      handleButtonPress(charVal);
      break; 
    
    case KEY_LEFT:
    case KEY_UP:
    case KEY_DOWN:
    case KEY_RIGHT:
      handleArrowPress(charVal);
      break;

    case KEY_ENTER:
    case KEY_JPRESS:
      handleEnterKey();
      break;
  }
}

// in general, just print it.
void handleCharacter(char key) {
  Serial.print("Handling character: ");
  Serial.println(key);
  
  srcString[currentCharacter] = key;
  drawCharacter(currentCharacter);

  currentCharacter++;

  if(currentCharacter > MAX_SRC_STRING) {
    currentCharacter = 0;
  }

  moveCursor(currentCharacter);
}

void loop()
{

// Paint touch
 /* 
  if(touchAvailable && !ts.bufferEmpty()) {
    Serial.println("In Touch handler.");
    TS_Point p = ts.getPoint();

    Serial.print("p.x:");
    Serial.println(p.x);

    Serial.print("p.y:");
    Serial.println(p.y);

    p.x = map(p.x, TS_MINY, TS_MAXY, tft.height(), 0);
    p.y = map(p.y, TS_MINX, TS_MAXX, 0, tft.width());
    
    pixels.setPixelColor(0, pixels.Color(255, 0, 255));
    pixels.show(); 
    
    tft.fillCircle(p.y, p.x, 3, ILI9341_MAGENTA);
    
    pixels.clear();
    pixels.show();     
    delay(TOUCH_DELAY);
  }
*/

  // Print keys to screen
  if (keyboard.keyCount()) {
    
 
    
    const BBQ10Keyboard::KeyEvent k_evt = keyboard.keyEvent();
    if (k_evt.state == BBQ10Keyboard::StatePress) {
      unsigned int keyVal = (unsigned int)k_evt.key;
      Serial.print("Int value of key: ");
      Serial.println(keyVal);
    }

    if (k_evt.state == BBQ10Keyboard::StateRelease) {
     // pixels.setPixelColor(0, pixels.Color(0, 255, 0));
     // pixels.show(); 
    
      handleKey(k_evt.key);
      //tft.print(k_evt.key);      
      //pixels.clear();
      //pixels.show(); 
    }
  }
}