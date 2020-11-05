/**
 * COPYRIGHT    (c)	Chris Fogelklou 2020
 * @author:     Chris Fogelklou
 * @brief       Generic JSON command handler, refactored from BleCommand.
 */
#ifndef JSON_COMMANDS_H__
#define JSON_COMMANDS_H__

#ifdef __cplusplus

#include "nlohmann/json.hpp"
#include <string>

struct TaskSchedulableTag;

namespace jsoncmd {
  class JsonCommandExec;
  class JsonHolder;

  typedef struct HdrTag {
    std::string  cmd = "";
    int          cmdId = 0;
  } Hdr;

  // A JsonCommand is registered and executed asynchronously, and produces a response.
  class JsonCommand {
  public:

    typedef void(*OnCompletedCb)(
      void* const pUserData,
      const char* const pJsonEvent);

    Hdr hdr;

    JsonCommand(
      const std::string pJson,
      JsonCommandExec &mExecutor,
      OnCompletedCb onCompleted = nullptr,
      void* pUserData = nullptr,
      const bool executeImmediately = false
    );
    virtual ~JsonCommand();

    // Inherit this for your custom execution routine.
    virtual void exec();


  protected:

    static void OnSchedCb(void* pUserData, uint32_t timeOrTicks);

    typedef struct {
      struct TaskSchedulableTag* pSched;
      JsonCommand* pThis;
    } JsonCommandCStruct;

  protected:
    friend class JsonCommandExec;
    JsonHolder* mpJsonHolder;
    OnCompletedCb mpOnCompletedCb;
    void* mpUserData;
    JsonCommandCStruct mC;
    JsonCommandExec* const mpExec;

  private:
    JsonCommand();
  };




  typedef struct CmdHandlerNodeDataTag {
    std::string cmd;
    int cmdId;
    // Is a nlohmann::json pointer, but defined like this so functions can be defined externally without requiring to include json.hpp from a header file.
    const nlohmann::json& jsonIn;
    // Is a nlohmann::json pointer, but defined like this so functions can be defined externally without requiring to include json.hpp from a header file.
    nlohmann::json& jsonOut;
  } CmdHandlerNodeData;

  typedef bool(*OnJsonCommandFn)(CmdHandlerNodeData* const pCmdData);

  typedef struct CmdHandlerNodeTag {
    const std::string cmd;
    OnJsonCommandFn OnCommand;
  } CmdHandlerNode;

  class JsonCommandExec {
  public:
    JsonCommandExec();
    ~JsonCommandExec();

    void addCommand(const std::string cmd, OnJsonCommandFn OnCommand);
    void addCommand(const CmdHandlerNode& cmd);
    void addCommands(const CmdHandlerNode ary[], const int numCommands);
    void removeCommand(const std::string cmd);
    void removeCommand(const CmdHandlerNode &cmd);
    void removeCommands(const CmdHandlerNode ary[], const int numCommands);

  protected:
    friend class JsonCommand;
    OnJsonCommandFn getNodeByCmd(std::string cmd);



  private:
    std::map<std::string, OnJsonCommandFn> mCommands;


  };

};


#endif

#endif // #ifndef JSON_COMMANDS_H__
