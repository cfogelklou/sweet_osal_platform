#ifndef MOBILE_JSON_HANDLER_HPP
#define MOBILE_JSON_HANDLER_HPP

#include "utils/simple_string.hpp"
#include "json_command/json_command.hpp"

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
