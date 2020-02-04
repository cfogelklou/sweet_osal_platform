#include "json_commands.hpp"
#include "utils/platform_log.h"
#include "utils/helper_macros.h"
#include "nlohmann/json.hpp"
#include "task_sched/task_sched.h"
#include "osal/platform_type.h"
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

LOG_MODNAME("pakp_ble_commands.cpp");

using namespace nlohmann;

// ////////////////////////////////////////////////////////////////////////////
// Internal class for holding a parsed json string.
class JsonCommand::JsonHolder {
public:
  JsonHolder(const char* const pStr)
    : mJson()
  {
    __try {
      json j = json::parse(pStr);
      if (j.size() > 0) {
        mJson = j;
        LOG_TRACE(("JsonHolder::Holding JSON of size %d", mJson.size()));
      }
      else {
        LOG_TRACE(("JsonHolder::JSON size was zero!"));
        LOG_ASSERT_WARN(false);
      }
    }
    __catch(std::exception &) {
      LOG_ASSERT_WARN(false);
      mJson.clear();
    }
    __catch(...) {
      LOG_ASSERT_WARN(false);
      mJson.clear();
    }
  }

  ~JsonHolder() {
    mJson.clear();
  }

  json& get() {
    return mJson;
  }

protected:
  json mJson;

};

// ////////////////////////////////////////////////////////////////////////////
void JsonCommand::OnSchedCb(void* pUserData, uint32_t timeOrTicks) {
  JsonCommandCStruct* pC = (JsonCommandCStruct*)pUserData;
  (void)timeOrTicks;
  if ((pC) && (pC->pThis)) {
    pC->pThis->exec();
    delete pC->pThis;
  }
}

static void jsonCmdDefaultOnCompletedCbC(
  void* const,
  const bool,
  const char* const) {
}

// ////////////////////////////////////////////////////////////////////////////
JsonCommand::JsonCommand(
  const char* pJson,
  OnCompletedCb onCompleted,
  void* pUserData,
  const bool executeImmediately
)
  : hdr()
  , mpOnCompletedCb((onCompleted) ? onCompleted : jsonCmdDefaultOnCompletedCbC)
  , mpUserData(pUserData)
  , mC(this)
{
  mpJsonHolder = new JsonHolder(pJson);
  json& j = mpJsonHolder->get();
  hdr.cmd = j["cmd"].get<std::string>();
  hdr.cmdId = j["cmdId"].get<int>();

  mC.pSched = new TaskSchedulable(OnSchedCb, &mC, TS_PRIO_APP_EVENTS, 0, 0);
  if (!executeImmediately) {
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
static bool OnUnhandledCmd(CmdHandlerNodeData* const pCmdData) {
  const json& jIn = pCmdData->jsonIn;
  json& jOut = pCmdData->jsonOut;
  (void)jIn;
  (void)jOut;
  //std::cerr << "Unhandled command!" << std::endl;
  return true;
}

// ////////////////////////////////////////////////////////////////////////////
static const CmdHandlerNode CmdHandlerNodeAryDummy[] = {
  { "invalid", OnUnhandledCmd },
};

// ////////////////////////////////////////////////////////////////////////////
static const CmdHandlerNode* pCmdHandlerNodeAry = CmdHandlerNodeAryDummy;
static int cmdHandlerNodeAryLen = 1;

// ////////////////////////////////////////////////////////////////////////////
void JsonCommandInit(const CmdHandlerNode* pAry, const int len) {
  pCmdHandlerNodeAry = pAry;
  cmdHandlerNodeAryLen = len;
}

// ////////////////////////////////////////////////////////////////////////////
static const CmdHandlerNode* getNodeByCmd(const std::string& cmd) {
  const CmdHandlerNode* rval = NULL;
  const int sz = cmdHandlerNodeAryLen;
  int ncmd = 0;
  while ((NULL == rval) && (ncmd < sz)) {
    const CmdHandlerNode& node = pCmdHandlerNodeAry[ncmd];
    if (node.cmd == cmd) {
      rval = &node;
    }
    ncmd++;
  }
  return rval;
}

// ////////////////////////////////////////////////////////////////////////////
void JsonCommand::exec() {
  json& j = mpJsonHolder->get();
  json cmdRsp = json::object();
  const CmdHandlerNode* const pNode = getNodeByCmd(hdr.cmd);
  bool replyNow = false;
  __try {
    if (pNode) {
      CmdHandlerNodeData data = {
        hdr.cmd,
        hdr.cmdId,
        j["cmdData"],
        cmdRsp
      };
      replyNow = pNode->OnCommand(&data);
    }
  }
  __catch(std::exception & e) {
#ifndef ANDROID
    (void)e;
#endif
    //std::cerr << "ERROR in JsonCommand::exec() " << std::endl;
    LOG_ASSERT(false);
  }
  __catch(...) {
    //std::cerr << "ERROR in JsonCommand::exec() Unknown exception caught." << std::endl;
    LOG_ASSERT(false);
  }

  if ((replyNow) && (mpOnCompletedCb)) {
    json reply = json::object();
    reply["status"] = true;
    reply["cmdId"] = hdr.cmdId;
    reply["cmd"] = hdr.cmd;
    reply["cmdRsp"] = cmdRsp;
    std::string sz = reply.dump();
    mpOnCompletedCb(mpUserData, true, sz.c_str());
  }
  else {
    // Todo: Implement reply for command that cannot reply immediately.
    LOG_ASSERT_WARN(false);
  }
}
