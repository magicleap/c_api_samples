// %BANNER_BEGIN%
// ---------------------------------------------------------------------
// %COPYRIGHT_BEGIN%
//
// Copyright (c) 2018 Magic Leap, Inc. All Rights Reserved.
// Use of this file is governed by the Creator Agreement, located
// here: https://id.magicleap.com/creator-terms
//
// %COPYRIGHT_END%
// ---------------------------------------------------------------------
// %BANNER_END%
#include <app_framework/application.h>

#include <ml_locale.h>

class LocaleApp : public ml::app_framework::Application {
public:
  LocaleApp(int argc = 0, char **argv = nullptr) : ml::app_framework::Application(argc, argv) {}

  void OnStart() override {
    const char *language_code;
    MLResult result = MLLocaleGetSystemLanguage(&language_code);
    if (MLResult_Ok == result) {
      ML_LOG(Info, "Language Code : %s", language_code);
    } else {
      ML_LOG(Error, "Failed to read the language code : %s", MLGetResultString(result));
    }

    const char *country_code;
    result = MLLocaleGetSystemCountry(&country_code);
    if (MLResult_Ok == result) {
      ML_LOG(Info, "Country Code : %s", country_code);
    } else {
      ML_LOG(Error, "Failed to read the country code : %s", MLGetResultString(result));
    }

    StopApp();
  }
};

int main(int argc, char **argv) {
  LocaleApp app(argc, argv);
  app.SetUseGfx(false);
  app.RunApp();
  return 0;
}
