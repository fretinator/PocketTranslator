#ifndef TRANSLATION_API_H
#define TRANSLATION_API_H

#include <Arduino.h>

#undef RELEASE_MODE

#ifdef RELEASE_MODE
 #include "private_settings.h"
#else
  #define H_RAPID_API_KEY_VALUE  "ItsInYourKitchenDrawer";
#endif

class TranslationAPI {
  public:



  //static const char* getx_client_trace_id_value();
  static String getTranslation(String srcString, String fromLang, 
    String toLang);

  // Includes information for the translaction API
  static constexpr char* API_SUBMIT_TYPE = "POST";




  // HEADERS
  static constexpr char*  H_API_BEGIN_URL =
   "https://microsoft-translator-text.p.rapidapi.com/translate?api-version=3.0&profanityAction=NoAction&textType=plain";
  static constexpr char* X_CLIENT_TRACE_ID_LABEL =  "X-ClientTraceId";
  static constexpr char* H_RAPID_API_HOST_LABEL =   "x-rapidapi-host";
  static constexpr char* H_RAPID_API_HOST_VALUE =   "microsoft-translator-text.p.rapidapi.com";
  static constexpr char* H_RAPID_API_KEY_LABEL =    "X-RapidAPI-Key";

  static constexpr char* H_CONTENT_TYPE_LABEL =     "content-type";
  static constexpr char* H_CONTENT_TYPE_VALUE =     "application/json";
  static constexpr char* SOURCE_LANGUAGE =          "en";
  static constexpr char* DEST_LANGUAGE =            "fil";
  static constexpr char* API_VERSION_PARAM =        "api-version=3.0";
  // Only need these 2 if you include the from language
  static constexpr char* BODY_JSON_SECTION =        "translations";
  static constexpr char* BODY_JSON__FIELD =         "text";




  static String getSubmitJSON(String srcString); 
  static String parseJSON(String json);
  static void getGUID();
};
#endif
