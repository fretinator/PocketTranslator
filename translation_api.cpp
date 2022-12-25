#include <ArduinoJson.h>
#include <ArduinoJson.hpp>


#include "translation_api.h"

#include <XxHash_arduino.h>
#include <UrlEncode.h>

#include <WiFi.h>
#include <HTTPClient.h>

static const uint MAX_RESULT = 255;
static const size_t JSON_CAPACITY = 2048;

#define GUID_SIZE 32
static char guidBuffer[17];
static char strGUID[GUID_SIZE + 1];
//static Xxh32stream xx32;

// The text to translate goes into the submitJSON buffer between the 2 JSON templates
static String SRC_JSON_TEMPLATE_BEGIN = "[\r\t\t{\r\t\t\t\"Text\": \"";
static String SRC_JSON_TEMPLATE_END = "\"\r\t\t}\r]";

String TranslationAPI::getTranslation(String srcString, String fromLang, 
    String toLang) {
  if (WiFi.status() == WL_CONNECTED) {
      HTTPClient http;
      
      getGUID();
      
      String begin_url(H_API_BEGIN_URL);
#ifndef RELEASE_MODE
      begin_url += "&from=" + fromLang + "&to=" + toLang;
#endif
        
      Serial.print("API CALL URL: ");
      Serial.println(begin_url);
      
      Serial.print("RapidAPI KEY: ");
      Serial.println(H_RAPID_API_KEY_VALUE);

      http.begin(begin_url);
      http.addHeader(H_RAPID_API_HOST_LABEL, H_RAPID_API_HOST_VALUE);
      http.addHeader(H_RAPID_API_KEY_LABEL, H_RAPID_API_KEY_VALUE);
      http.addHeader(H_CONTENT_TYPE_LABEL, H_CONTENT_TYPE_VALUE);
#ifndef RELEASE_MODE      
      http.addHeader(X_CLIENT_TRACE_ID_LABEL, strGUID);
//#else
//      http.addHeader(H_ACCEPT_ENCODING, H_ACCEPT_ENCODING_VALUE);
#endif

      String postVal = getSubmission(srcString, fromLang, toLang);
      Serial.print("Translation POST Value is: ");
      Serial.println(postVal);

      int httpCode = http.POST(postVal);
      
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

  xxh32(guidBuffer, inputBuff.substring(0,16).c_str());
  strncpy(strGUID, guidBuffer, 16);

  xxh32(guidBuffer, inputBuff.substring(16,32).c_str());
  strncat(strGUID, guidBuffer, 16);

  strGUID[GUID_SIZE] = '\0'; // for safety

  Serial.print("Our new GUID is: ");
  Serial.println(strGUID);
  Serial.println(strlen(strGUID));
}


String TranslationAPI::getSubmission(String srcString, String fromLang, String toLang) {
#ifndef RELEASE_MODE  
  return SRC_JSON_TEMPLATE_BEGIN + srcString + SRC_JSON_TEMPLATE_END;
#else
  return "q=" + urlEncode(srcString) + "&format=text&target=" + toLang + "&source=" + fromLang;
#endif
}

String TranslationAPI::parseJSON(String json) {
  String ret;
  DynamicJsonDocument doc(JSON_CAPACITY);
  
  Serial.print("JSON returned from WebService: ");
  Serial.print(json);
  
  // Extract values
  DeserializationError error = deserializeJson(doc, json);

  if (error) {
    ret = error.f_str();
    ret += F("\ndeserializeJson() failed: ");
    Serial.println(ret);


  } else {
    // Get transated text
    ret = doc["data"]["translations"][0]["translatedText"].as<String>();
    Serial.print("Translation: ");
    Serial.println(ret);
  }

  return ret;
}
 


