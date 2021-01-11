/**
 * COPYRIGHT    (c)	Chris Fogelklou 2020
 * @author:     Chris Fogelklou
 * @brief       Generic JSON command handler, refactored from BleCommand.
 */
#include "json_command/json_command.hpp"

#include "utils/helper_macros.h"
#include "task_sched/task_sched.h"
#include "osal/platform_type.h"
#include <iostream>
#include <string>

#if (TARGET_OS_ANDROID > 0)
#ifndef __try
#define __try if(true)
#endif
#ifndef __catch
#define __catch(e) if(false)
#endif
#endif

#ifndef __try
#define __try try
#endif
#ifndef __catch
#define __catch(e) catch(e)
#endif

LOG_MODNAME("jsoncommands.cpp");

using namespace jsoncmd;
using namespace nlohmann;

// ////////////////////////////////////////////////////////////////////////////
namespace jsoncmd {

  class JsonHolder {
  public:
    JsonHolder(const char * const pStr)
    {
        __try {
        json j = json::parse(pStr);
        if (j.size() > 0){
          mJson = j;
          LOG_VERBOSE(("Got JSON of size %d\r\n", mJson.size()));
        }
        else {
          LOG_VERBOSE(("JSON size was zero!"));
        }
      }
      __catch (std::exception& e) {
#ifndef ANDROID
        (void)e;
#endif
        std::cerr << "ERROR in construction of JsonHolder " << std::endl;
        LOG_ASSERT_WARN(false);
      }
        __catch (...) {
        std::cerr << "ERROR in construction of JsonHolder: Unknown exception caught." << std::endl;
        LOG_ASSERT_WARN(false);
      }
    }

    ~JsonHolder() {
      mJson.clear();
    }

    json &get() {
      return mJson;
    }

  protected:
    json mJson;

  };
}

// ////////////////////////////////////////////////////////////////////////////
void JsonCommand::OnSchedCb(void *pUserData, uint32_t timeOrTicks) {
  JsonCommand::JsonCommandCStruct *pC = (JsonCommand::JsonCommandCStruct *)pUserData;
  (void)timeOrTicks;
  if (pC->pThis) {
    pC->pThis->exec();
  }
  delete pC->pThis;
}

// ////////////////////////////////////////////////////////////////////////////
static void bleCmdDefaultOnCompletedCbC(
  void * const,
  const char * const) {
}

// ////////////////////////////////////////////////////////////////////////////
JsonCommand::JsonCommand(
  const std::string pJson,
  JsonCommandExec& mExecutor,
  OnCompletedCb onCompleted,
  void *pUserData,
  const bool executeImmediately
                       )
  : hdr()
  , mpJsonHolder(nullptr)
  , mpOnCompletedCb((onCompleted) ? onCompleted : bleCmdDefaultOnCompletedCbC)
  , mpUserData(pUserData)
  , mC()
  , mpExec(&mExecutor)
{
  hdr.cmd = "invalid";
  hdr.cmdId = 0;
  mpJsonHolder = new JsonHolder(pJson.c_str());
  LOG_ASSERT(mpJsonHolder);
  json &j = mpJsonHolder->get();
  hdr.cmd = j["cmd"].get<std::string>();
  hdr.cmdId = j["cmdId"].get<int>();
  mC.pThis = this;
  mC.pSched = new TaskSchedulable(OnSchedCb, &mC);
  if (!executeImmediately){
    TaskSchedAddTimerFn(TS_PRIO_APP_EVENTS, mC.pSched, 0, 0);
  }
  else {
    OnSchedCb(&mC, OSALGetMS());
  }
}

// ////////////////////////////////////////////////////////////////////////////
JsonCommand::~JsonCommand()
{
  delete mC.pSched;
  delete mpJsonHolder;
}

// ////////////////////////////////////////////////////////////////////////////
static bool OnUnhandledCmd(CmdHandlerNodeData * const pCmdData) {
  const json &jIn = pCmdData->jsonIn;
  json &jOut = pCmdData->jsonOut;
  (void)jIn;
  (void)jOut;
  std::cerr << "Unhandled command!" << std::endl;
  return true;
}

// ////////////////////////////////////////////////////////////////////////////
static const CmdHandlerNode CmdHandlerNodeAry[] = {
  { "invalid", OnUnhandledCmd },
};

// ////////////////////////////////////////////////////////////////////////////
void JsonCommand::exec() {
  json& j = mpJsonHolder->get();
  json cmdRsp = json::object();
  OnJsonCommandFn handlerFn = mpExec->getNodeByCmd(hdr.cmd);
  bool replyNow = false;
  __try {
    if (handlerFn) {
      CmdHandlerNodeData data = {
        hdr.cmd,
        hdr.cmdId,
        j["cmdData"],
        cmdRsp
      };
      replyNow = handlerFn(&data);
    }
  }
  __catch(std::exception & e) {
#ifndef ANDROID
    (void)e;
#endif
    std::cerr << "ERROR in JsonCommand::exec() " << std::endl;
    LOG_ASSERT(false);
  }
  __catch(...) {
    std::cerr << "ERROR in JsonCommand::exec() Unknown exception caught." << std::endl;
    LOG_ASSERT(false);
  }

  if ((replyNow) && (mpOnCompletedCb)) {
    json reply = json::object();
    reply["status"] = true;
    reply["cmdId"] = hdr.cmdId;
    reply["cmd"] = hdr.cmd;
    reply["cmdRsp"] = cmdRsp;
    std::string sz = reply.dump();
    mpOnCompletedCb(mpUserData, sz.c_str());
  }
  else {
    if (!replyNow) {
      // Todo: Implement reply for command that cannot reply immediately.
      LOG_WARNING(("A json handler has specified that reply will happen later, but this must be implemented in JsonCommand()\r\n"));
    }
  }
}

// ////////////////////////////////////////////////////////////////////////////
JsonCommandExec::JsonCommandExec()
: mCommands()
{
}

// ////////////////////////////////////////////////////////////////////////////
JsonCommandExec::~JsonCommandExec() {
}

// ////////////////////////////////////////////////////////////////////////////
OnJsonCommandFn JsonCommandExec::getNodeByCmd(std::string cmd) {
  OnJsonCommandFn rval = nullptr;
  //LOG_TRACE(("JsonCommandExec::size=%d\r\n", mCommands.size()));
  auto f = mCommands.find(cmd);
  if (f != mCommands.end()) {
    rval = f->second;
  }
  return rval;
}

// ////////////////////////////////////////////////////////////////////////////
void JsonCommandExec::addCommand(const std::string cmd, OnJsonCommandFn OnCommand) {
  mCommands[cmd] = OnCommand;
}

// ////////////////////////////////////////////////////////////////////////////
void JsonCommandExec::addCommand(const CmdHandlerNode &cmd) {
  addCommand(cmd.cmd, cmd.OnCommand);
}

// ////////////////////////////////////////////////////////////////////////////
void JsonCommandExec::addCommands(const CmdHandlerNode ary[], const int numCommands) {
  for (int i = 0; i < numCommands; i++) {
    addCommand(ary[i]);
  }
}

// ////////////////////////////////////////////////////////////////////////////
void JsonCommandExec::removeCommand(const std::string  cmd) {
  mCommands.erase(cmd);
}

// ////////////////////////////////////////////////////////////////////////////
void JsonCommandExec::removeCommand(const CmdHandlerNode& cmd) {
  removeCommand(cmd.cmd);
}

// ////////////////////////////////////////////////////////////////////////////
void JsonCommandExec::removeCommands(const CmdHandlerNode ary[], const int numCommands) {
  for (int i = 0; i < numCommands; i++) {
    removeCommand(ary[i]);
  }
}


