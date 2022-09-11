//#include <TSC2004.h>


#define RELEASE_MODE

#ifdef RELEASE_MODE
#include "secrets.h"
#else
  static constexpr char* my_ssid = "honeypot2";
  static constexpr char* my_password = "1234";
#endif


#include <SdFat.h>
#include <SdFatConfig.h>
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

// For sorted file list
#include "sorted_file_list.h"

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
#define MAIN_FONT_SIZE 2
#define BTN_FONT_SIZE 1
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
uint btnY;

struct pixel_pos {
  int x;
  int y;
} currentCharPos;

enum OP_MODE {
  OP_ONLINE,
  OP_OFFLINE,
};

enum LANG_MODE {
  SRC_FIRST,
  DEST_FIRST,
};

OP_MODE my_op_mode = OP_ONLINE;
LANG_MODE my_lang_mode = SRC_FIRST;

static const String srcLang = "en";
static const String destLang = "tl";
#define SRC_LANG_CAPTION "Text to Translate:"
#define DEST_LANG_CAPTION "Teksto upang isalin:"
#define SRC_LANG_TRANS_LABEL "TRANSLATION"
#define DEST_LANG_TRANS_LABEL "PAGSASALIN"

const char* btn_labels[8] = {"Tran","Rset","Save","Mode",
  "Prev","Next","File","Mode"};

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

// Source is a char array so we can implement a 
// simple text editor
char srcString[MAX_SRC_STRING + 1]; // one for null character
String strTranslation;

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
#define MAX_LINE_LEN 255
char fBuffer[MAX_LINE_LEN + 1]; // room for \0

// Pause before showing translation in offline mode
const int  tr_delay = 3 * 1000; 

// Used to get the previous word
// Single-linked list stack
struct WordOffset {
  int offset;
  WordOffset* prevWord;
};

WordOffset* wordStackTop = NULL;

#define SAVED_TRANSLATIONS_FILE "00_saved.txt"
SortedFileList fileList;

int addCurrentWordOffset(int offset) {
  WordOffset* newWordOffset = new WordOffset();
  
  newWordOffset->offset = offset;
  newWordOffset->prevWord = wordStackTop;
  
  wordStackTop = newWordOffset;
}

void deleteWordOffsetStack() {
  WordOffset* tmp = wordStackTop;

  while(tmp != NULL) {
    WordOffset* del = tmp;
    tmp = tmp->prevWord;

    delete del;
  }

  wordStackTop = NULL;
}

void initSourceString() {
  
  // Initialize source string
  for(int x = 0; x < MAX_SRC_STRING; x++) {
    srcString[x] = ' ';
  }

  // srcString is size MAX_SRC_STRING + 1
  srcString[MAX_SRC_STRING] = '\0';
}

bool connectToWiFi(bool backup) {
  
  if(backup) {
    WiFi.begin(backup_ssid, backup_password);
  } else {
    WiFi.begin(my_ssid, my_password);
  }

  uint attempts = 0;
  Serial.println(WiFi.macAddress());
  tft.print("Connecting to ");
  tft.print(backup ? backup_ssid : my_ssid);

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
  tft.setTextSize(MAIN_FONT_SIZE);
}
void drawButtonLabels() {
  tft.setTextSize(BTN_FONT_SIZE);
  
  // Which set of button button labels to display
  int labelSet = (OP_ONLINE == my_op_mode ? 0 : 4);
  
  btnY = SCREEN_HEIGHT - (2 * BORDER_PAD) - CHAR_HEIGHT;
  uint btnX = BORDER_PAD; // For first button;
  uint btnW = (3 * CHAR_WIDTH) + (4 * CHAR_PAD);
  uint btnH = CHAR_HEIGHT + (2 * CHAR_PAD);

  tft.fillRect(btnX, btnY, btnW, btnY, RECT_COLOR);
  tft.setCursor(btnX + 3 * CHAR_PAD, btnY +  4 * CHAR_PAD);
  tft.print(btn_labels[0 + labelSet]);

  btnX += 2 * btnW; 

  tft.fillRect(btnX, btnY, btnW, btnY, RECT_COLOR);
  tft.setCursor(btnX + 3 * CHAR_PAD, btnY + 4 * CHAR_PAD);
  tft.print(btn_labels[1 + labelSet]); 
  
  // Now start from the right side,
  // subtract pad and width of button
  btnX = SCREEN_WIDTH  - BORDER_PAD - btnW;

  tft.fillRect(btnX, btnY, btnW, btnY, RECT_COLOR);
  tft.setCursor(btnX + 3* CHAR_PAD, btnY + 4 * CHAR_PAD);
  tft.print(btn_labels[3 + labelSet]);

  btnX -= 2 * btnW; 

  tft.fillRect(btnX, btnY, btnW, btnY, RECT_COLOR);
  tft.setCursor(btnX + 3 * CHAR_PAD, btnY + 4 * CHAR_PAD);
  tft.print(btn_labels[2 + labelSet]);   

  tft.setTextSize(MAIN_FONT_SIZE);
}

void drawHomeScreen() {

  // Clear screen
  tft.fillScreen(BACK_COLOR);

  tft.setCursor(BORDER_PAD, BORDER_PAD);
  
  if(OP_ONLINE == my_op_mode || !sdOK) {
    tft.print((SRC_FIRST == my_lang_mode) ? SRC_LANG_CAPTION :
      DEST_LANG_CAPTION);
  } else {
    
    entry.getName(fName, MAX_FILENAME);

    if(strncmp(fName, "00_", 3) == 0) {
      tft.print("Saved Phrases:");
    } else {
      tft.print(fName);
    }
  }


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

 // Try slower speed for non-hardware CS pin
  Serial.println("Open SD Card  ");
  if(!sd.begin(SD_CS, SD_SCK_MHZ(25))) {
    Serial.println("SD not available.");
    sdOK = false;
  } else {
    Serial.println("Attempting to open SD root filesystem...");
    if(!root.open("/")) {
      Serial.println("Error opening root filesystem.");
      sdOK = false;
    } else {
      sdOK = true;
      Serial.println("Listing files on SD card...");
      //tft.print("SD Card contents:\n");

      
      while (entry.openNext(&root, O_RDONLY)) {
        entry.getName(fName, MAX_FILENAME);
        Serial.print("\t");
        Serial.println(fName); 
        fileList.add(fName);   
        //tft.println(fName);
        entry.close();
      }
      fileList.dump(Serial);
    }
  }
  esp_random();

  if(!connectToWiFi(false)) {
    if(!connectToWiFi(true)) {
      if(sdOK) {
        my_op_mode = OP_OFFLINE;
        fileList.rewind();
        entry.open(fileList.next(), O_RDONLY);
        handleReset();
        return handleNextWord();
      } else {
        tft.println("WiFi and SD card failed, program aborting.");
        while(1);
      }
    }
  }



  drawHomeScreen();

  //tft.print("Hello FeatherWing!\n");
  //tft.print("Touch to paint, type to... type\n");

 


}

void handleKey(char key) {
  uint keyVal = (uint)key;

  if(isControlValue(keyVal)) {
    handleControlValue(keyVal);
  } else {
    handleCharacter(key);
  }
}

void clearTranslationArea() {
   // Should be where translation text prints
   tft.fillRect(BORDER_PAD, translationPosY, 
   SCREEN_WIDTH - BORDER_PAD,
    btnY - translationPosY, 
    BACK_COLOR);
}

void printTranslation(String translation, bool clearArea) {
    if(clearArea) {
      clearTranslationArea();
    }
    
    tft.setCursor(BORDER_PAD, translationPosY);
    tft.print(translation);
}

void submitTranslation() {
  String substr = String(srcString);
  substr.trim();

  Serial.print("String to submit is: ");
  Serial.println(substr);

  if(substr.length() > 0) {
    if(SRC_FIRST == my_lang_mode) {
      strTranslation = TranslationAPI::getTranslation(substr,
        srcLang, destLang);
    } else {
      strTranslation = TranslationAPI::getTranslation(substr,
      destLang, srcLang);
    }
    
    printTranslation(strTranslation, true);
  }
}

void handleReset() {
  currentCharacter = 0;
  initSourceString();
  drawHomeScreen();
}


void handleSave() {
  String line = "";
  Serial.println("Attempting to save.");
  delay(500);

  pixels.show();

  changeSourceCaption("Saving...");
  Serial.print("Source string: ");
  Serial.println(srcString);

  Serial.print("Translation string: ");
  Serial.println(strTranslation);
  if(strlen(srcString) > 0 && strTranslation.length() > 0) { 
    // For consistency, always write source language first
    if(SRC_FIRST == my_lang_mode) {
      line = srcString;
      line += "\t";
      line += strTranslation;
    } else {
      line = strTranslation;
      line += "\t";
      line += srcString;
    }

    if(!writeLine(line)) {
      changeSourceCaption("Saving failed.");
    } else {
      changeSourceCaption("Saving completed.");
    }
  } else {
    changeSourceCaption("Blank phrases not saved");
  }

  delay(1000);

  changeSourceCaption((SRC_FIRST == my_lang_mode) ? SRC_LANG_CAPTION : DEST_LANG_CAPTION);
  handleReset();
}

/*
  Now support 4 modes:
    1. ONLINE   - SOURCE  LANGUAGE FIRST
    2. ONLINE   - DEST    LANGUAGE FIRST
    3. OFFLINE  - SOURCE  LANGUAGE FIRST
    4. OFFLINE  - DEST    LANGUAGE FIRST
*/
void handleModeSwitch() {
  if(OP_ONLINE == my_op_mode) {
    if(SRC_FIRST == my_lang_mode) {
      my_lang_mode = DEST_FIRST;
      handleReset();
    } else {
      my_op_mode = OP_OFFLINE;
      my_lang_mode = SRC_FIRST;
      entry.close();

      //root.rewind();
      // Need to open first file on card and display first word
      fileList.rewind();
      entry.open(fileList.next(), O_RDONLY);
      handleReset();
      // Don't NEED to return,
      // but just in case more code is added
      return handleNextWord();
    }
  } else {
    if(SRC_FIRST == my_lang_mode) {
          my_lang_mode = DEST_FIRST;
          return handleNextWord();
      } else {    
        my_op_mode = OP_ONLINE;
        deleteWordOffsetStack();
        my_lang_mode = SRC_FIRST;
        return handleReset();
      }
  }
}



/* 
** To get to the beginning of the previous word,
** we have to pop 1 offset from the WordOffset Stack, but
** go two offset back in the file
**  1. The 1st offset is to get to the beginning of current word.
**  2. The 2nd offset is to get to the beginning of previous word. 
*/
void handlePrevWord() {
  // Not that we do not implement previous file function if at top
  if(wordStackTop != NULL && wordStackTop->prevWord != NULL) {
    WordOffset* temp = wordStackTop->prevWord;
    int totalOffset = wordStackTop->offset;
    delete wordStackTop;

    wordStackTop = temp;
    totalOffset += wordStackTop->offset; // Was previous word offset

    entry.seekCur(totalOffset);
    handleNextWord();
  }
}

void handleNextFile() {
  entry.close();
  deleteWordOffsetStack();
  /*
  if(!entry.openNext(&root, O_RDONLY)) {
    root.rewind();
    entry.openNext(&root, O_RDONLY);
  }*/
  entry.open(fileList.next(), O_RDONLY);
  handleNextWord();
}

String readLine() {
  int charsRead = 0;
  int start_pos = entry.curPosition();

  while(!isLineTerminator(entry.peek())
  && charsRead <= MAX_LINE_LEN && entry.available()) {
     fBuffer[charsRead] = entry.read();
     charsRead++; // Yes could be done in one line
  }

  fBuffer[charsRead] = '\0';

  // Now remove line terminator(s)
  while(entry.available() && isLineTerminator(entry.peek())) {
    entry.read(); // discard
    Serial.println("Discarded terminator");
  }
  
  // We want a negative offset for handlePrevWord()
  addCurrentWordOffset(start_pos - entry.curPosition()); 
  return String(fBuffer);
}

bool writeLine(String line) {
  
  if(!entry.open(SAVED_TRANSLATIONS_FILE, O_WRITE | O_APPEND | O_AT_END)) {
    Serial.println("Saving is not implemented");
    return false;
  }

  entry.getName(fName, MAX_FILENAME);
  Serial.print("File to write: ");
  Serial.println(fName);
  Serial.print("Line to write: ");
  Serial.println(line);

  entry.seekCur(EOF);
  int peekVal = entry.peek();

  // Make sure we are either we are at beginning 
  // of file or previous line had line terminator
  if(!isLineTerminator(peekVal) && peekVal != -1) {
    entry.write('\n');
  }

  for(int x = 0; x < line.length();x++) {
    entry.write(line[x]);
  }
  entry.write('\n');
  entry.flush();

  entry.close();

  return true;
}

void changeSourceCaption(String caption) {
  tft.setCursor(BORDER_PAD, BORDER_PAD);
  tft.fillRect(BORDER_PAD, BORDER_PAD, SCREEN_WIDTH - 2 * BORDER_PAD, CHAR_HEIGHT,
    BACK_COLOR);
  tft.print(caption);  
}


bool isLineTerminator(char c) {
  return '\r' == c || '\n' == c;
}

// When coming from a file and you
// have already populate the source string
void displaySourceString() {
  for(int x = 0; x < MAX_SRC_STRING && srcString[x] != '\0'; x++) {
    setSourceCharPos(x);
    tft.setCursor(currentCharPos.x, currentCharPos.y);
    tft.print(srcString[x]);
  }
}

void handleNextWord() {
  String srcLangString;
  String destLangString;


  String curLine = readLine();

  curLine.trim();
  Serial.print("Current line: ");
  Serial.println(curLine);

  if(curLine.length() == 0) {
    return handleNextFile();
  }

  int splitPos = curLine.indexOf('\t');

  if(splitPos <= 0) {
    // Source word at least 1 letter
    // Assume bad file or end of file
    // and move on
    return handleNextFile();
  }

  srcLangString = curLine.substring(0, splitPos);
  destLangString = curLine.substring(splitPos + 1);

  if(destLangString.length() == 0) {
    // Maybe the next word is OK?
    return handleNextWord();
  }

  initSourceString();

  Serial.print("Language mode, 0 = Source First, 1 = Dest First: ");
  Serial.println(my_lang_mode);

  if(SRC_FIRST == my_lang_mode) {
    strTranslation = destLangString;


    strncpy(srcString, srcLangString.c_str(), MAX_SRC_STRING);
    srcString[MAX_SRC_STRING] = '\0';
  }  else {
    strTranslation = srcLangString;

    strncpy(srcString , destLangString.c_str(), MAX_SRC_STRING);
    srcString[MAX_SRC_STRING] = '\0';    
  }

  
 
  drawHomeScreen();
  
  displaySourceString();

  delay(tr_delay);

  printTranslation(strTranslation, false);
}

void handleButtonPress(uint btnVal) {
  switch(btnVal) {
    case KEY_B1:
      if(OP_ONLINE == my_op_mode) {
        submitTranslation();
      } else {
        handlePrevWord();
      }

      break;
    case KEY_B2:
      if(OP_ONLINE == my_op_mode) {
        handleReset();
      } else {
        
        handleNextWord();
      }

      break;
    case KEY_B3:
      if(OP_ONLINE == my_op_mode) {
        handleSave();
      } else {
        handleNextFile();
      }

      break;
    case KEY_B4:
      handleModeSwitch();

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