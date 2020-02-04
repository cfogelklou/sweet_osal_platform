/**
 * COPYRIGHT	(c)	Applicaudia
 * @file        json_commands.h
 * @brief       Converts JSON commands coming from the UI into action.
 */
#ifndef JSON_COMMANDS_H__
#define JSON_COMMANDS_H__

#ifdef __cplusplus
#include <string>
#include "nlohmann/json.hpp"

struct TaskSchedulableTag;

class JsonCommand;

// A JsonCommand is registered and executed asynchronously, and produces a response.
class JsonCommand {
public:

  typedef struct HdrTag {
    std::string  cmd;
    int          cmdId;
    HdrTag()
      :cmd("")
      , cmdId(0)
    {
    }
  } Hdr;

  typedef void(*OnCompletedCb)(void* const pUserData, const bool status, const char* const pJsonEvent);

  Hdr hdr;

  JsonCommand(
    const char* pJson,
    OnCompletedCb onCompleted = NULL,
    void* pUserData = NULL,
    const bool executeImmediately = false
  );
  virtual ~JsonCommand();

protected:
  // Used to get a pointer back to ourselves from the callback.
  typedef struct JsonCommandCStructTag{
    struct TaskSchedulableTag* pSched;
    JsonCommand* const pThis;
    JsonCommandCStructTag(JsonCommand* _pThis)
      : pSched(nullptr)
      , pThis(_pThis)
    {

    }
  } JsonCommandCStruct;

  // Inherit this for your custom execution routine.
  virtual void exec();

  // Callback
  static void OnSchedCb(void* pUserData, uint32_t timeOrTicks);

protected:
  class JsonHolder;
  JsonHolder         *mpJsonHolder;
  OnCompletedCb       mpOnCompletedCb;
  void               *mpUserData;
  JsonCommandCStruct  mC;

private:
  JsonCommand();
};


typedef struct {
  std::string cmd;
  int cmdId;
  // Is a nlohmann::json pointer, but defined like this so functions can be defined externally without requiring to include json.hpp from a header file.
  const nlohmann::json& jsonIn;
  // Is a nlohmann::json pointer, but defined like this so functions can be defined externally without requiring to include json.hpp from a header file.
  nlohmann::json& jsonOut;
} CmdHandlerNodeData;

typedef struct {
  const std::string cmd;
  bool(*OnCommand)(CmdHandlerNodeData* const pCmdData);
} CmdHandlerNode;

extern void JsonCommandInit(const CmdHandlerNode* pAry, const int len);


#endif

#endif // #ifndef JSON_COMMANDS_H__
