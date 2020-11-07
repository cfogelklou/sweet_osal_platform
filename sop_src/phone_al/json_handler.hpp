#ifndef ANDROID_JSON_HANDLER_HPP
#define ANDROID_JSON_HANDLER_HPP

//#include "al_ble_mobile/pak_abstraction_layer.h"
//#include "pakp_sm/private/pakp_ble_responses.h"
//#include "al_ble_mobile/pak_abstraction_layer.h"
#include "utils/simple_string.hpp"
#include "json_command/json_command.hpp"
//#include "pakp_sm/pakp_ble_commands.h"

#ifdef __cplusplus

class JsonHandler {
public:
  static JsonHandler &inst();

  void JsonEvent(void *const pConfigUserData,
                 const char * const pJsonStr,
                 const int strLen);

  static void OnJsonCompletedCb(
      void * const pUserData, const char * const pJsonEvent);

  void jsonCommand(const char * const jsonCommand);


public:

  JsonHandler();
  jsoncmd::JsonCommandExec mExec;
};

#endif // __cplusplus

#endif
