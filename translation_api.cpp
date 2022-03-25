#include "translation_api.h"

#include <XXH64.h>
#include <XXH32.h>
#include <XxHash_arduino.h>
#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
#include <WiFi.h>
#include <HTTPClient.h>

static const uint MAX_RESULT = 255;
static const size_t JSON_CAPACITY = 2048;

#define GUID_SIZE 32
static char guidBuffer[17];
static char strGUID[GUID_SIZE + 1];
static XXH64 xx64;

// The text to translate goes into the submitJSON buffer between the 2 JSON templates
static String SRC_JSON_TEMPLATE_BEGIN = "[\r\t\t{\r\t\t\t\"Text\": \"";
static String SRC_JSON_TEMPLATE_END = "\"\r\t\t}\r]";

String TranslationAPI::getTranslation(String srcString) {
  if (WiFi.status() == WL_CONNECTED) {
      getGUID();
      HTTPClient http; //Object of class HTTPClient
      http.begin(H_API_BEGIN_URL);
      http.addHeader(H_RAPID_API_HOST_LABEL, H_RAPID_API_HOST_VALUE);
      http.addHeader(H_RAPID_API_KEY_LABEL, H_RAPID_API_KEY_VALUE);
      http.addHeader(H_CONTENT_TYPE_LABEL, H_CONTENT_TYPE_VALUE);
      http.addHeader(X_CLIENT_TRACE_ID_LABEL, strGUID);
      
      int httpCode = http.POST(getSubmitJSON(srcString));
      
      Serial.println(httpCode);
      
      if (httpCode > 0) 
      {

        return parseJSON(http.getString());

      }
      http.end(); //Close connection

      return String("Error calling Translation API");
    }
}

void TranslationAPI::getGUID() {
  String inputBuff = String(millis());
  uint uRandom;

  strGUID[0] = '\0';

  while(inputBuff.length() < GUID_SIZE) {
    uRandom = random(100000L);
    inputBuff += String(uRandom);
  }

  Serial.println(inputBuff);

  xx64.hash(inputBuff.substring(0,16).c_str(), guidBuffer);
  strncpy(strGUID, guidBuffer, 16);

  xx64.hash(inputBuff.substring(16,32).c_str(), guidBuffer);
  strncat(strGUID, guidBuffer, 16);

  strGUID[GUID_SIZE] = '\0'; // for safety

  Serial.print("Our new GUID is: ");
  Serial.println(strGUID);
  Serial.println(strlen(strGUID));
}


String TranslationAPI::getSubmitJSON(String srcString) {
  return SRC_JSON_TEMPLATE_BEGIN + srcString + SRC_JSON_TEMPLATE_END;
 }

String TranslationAPI::parseJSON(String json) {
  String ret;
  DynamicJsonDocument doc(JSON_CAPACITY);
  // Extract values
  DeserializationError error = deserializeJson(doc, json);

  if (error) {
    ret = error.f_str();
    ret += F("\ndeserializeJson() failed: ");
    Serial.println(ret);


  } else {
    // Get transated text
    ret = doc[0]["translations"][0]["text"].as<String>();
    Serial.print("Translation: ");
    Serial.println(ret);
  }

  return ret;
}
 


