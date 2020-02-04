//
// Created by chfo on 2/24/2017.
//

#ifndef __JNI_NATIVE_H__
#define __JNI_NATIVE_H__
#include "osal/platform_type.h"
#include "utils/helper_utils.h"

#include <jni.h>

std::string JNIN_FetchString(JNIEnv *pEnv, jstring jId);

jstring JNIN_GetDeviceId(JNIEnv *pEnv, jobject contentObj);

// Holds function definitions shared between BLENative, CryptoNative, etc.

// Reduces some boilerplate code getting and setting an instance
// of a CPP class used by many of these native APIs.
// Tstruct is a struct with at least one member - a pointer to a Cpp structure.
// Tclass is the type of the CPP structure.
template <typename Tstruct, class Tclass>
class JniEntryGetCppInst {
public:

  // Allocates a new instance (byte array) to hold a CPP pointer.
  JniEntryGetCppInst(JNIEnv * pEnv, jclass jcls, Tclass *pCpp) :
     m_pEnv(pEnv),
     m_jInstArr(NULL)
  {
    jboolean jFalse = false;
    m_jInstArr = m_pEnv->NewByteArray(sizeof(Tstruct) * sizeof(char));
    m_pInst = m_pEnv->GetByteArrayElements(m_jInstArr, &jFalse);
    memset(m_pInst, 0, sizeof(Tstruct));
    m_pStruct = (Tstruct *)m_pInst;
    m_pStruct->pCpp = pCpp;
  }

  // Gets the CPP pointer holder.
  JniEntryGetCppInst(JNIEnv * pEnv, jclass jcls, jbyteArray jInstArr):
     m_pEnv(pEnv),
     m_jInstArr(jInstArr)
  {
    jboolean jFalse = false;
    m_pInst = pEnv->GetByteArrayElements(m_jInstArr, &jFalse);
    m_pStruct = (Tstruct *)m_pInst;
    m_pCppInst = m_pStruct->pCpp;
  }

  // Gets the byte array holding the CPP pointer.
  jbyteArray getInstArr() {
    return m_jInstArr;
  }

  // Gets the CPP instance.
  Tclass &getCppInst() {
    return *m_pCppInst;
  }

  Tclass *getCppInstPtr() {
    return m_pCppInst;
  }

  Tstruct *getStructPtr() {
    return m_pStruct;
  }

  ~JniEntryGetCppInst() {
    m_pEnv->ReleaseByteArrayElements(m_jInstArr, m_pInst, 0);
  }

private:
  JNIEnv * m_pEnv;
  jbyteArray m_jInstArr;
  jbyte *m_pInst;
  Tclass *m_pCppInst;
  Tstruct *m_pStruct;
};

// A utility to reduce boilerplate in fetching strings.
// Also prevents us forgetting to release them.
class JStringGetter {
public:
  JStringGetter(JNIEnv * const pEnv, jstring &jsz)
    : mpEnv(pEnv)
      , mIsCopy(false)
    , mjsz(jsz)
    , mpString(pEnv->GetStringUTFChars(jsz, &mIsCopy))
    , mStdString(mpString)
    , mLen(pEnv->GetStringUTFLength(jsz))
  {
  }

  const char *get(){
    return mpString;
  }

  const uint8_t *getU8(){
    return (uint8_t *)mpString;
  }

  const std::string &getString(){
    return mStdString;
  }

  const size_t len(){
    return mLen;
  }

  ~JStringGetter(){
    mpEnv->ReleaseStringUTFChars(mjsz, mpString);
  }

private:
  JNIEnv * const mpEnv;
  jboolean mIsCopy;
  jstring &mjsz;
  const char * const mpString;
  const std::string mStdString;
  const jsize mLen;
};

#endif //__JNI_NATIVE_H__

