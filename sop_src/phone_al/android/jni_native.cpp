//
// Created by chfo on 2/24/2017.
//

#if (TARGET_OS_ANDROID)

#include "jni_native.h"

std::string JNIN_FetchString(JNIEnv *pEnv, jstring jId) {
  std::string rval = "";
  if (jId) {
    const char *szId = pEnv->GetStringUTFChars(jId, 0);
    if (szId) {
      rval = szId;
      pEnv->ReleaseStringUTFChars(jId, szId);
    }
  }
  return rval;
}

jstring JNIN_GetDeviceId(JNIEnv *pEnv, jobject contentObj) {
  jstring jandroid_id = NULL;
  //try {
    jclass secClass = pEnv->FindClass("android/provider/Settings$Secure");
    jmethodID secMid = pEnv->GetStaticMethodID(secClass, "getString", "(Landroid/content/ContentResolver;Ljava/lang/String;)Ljava/lang/String;");
    jstring jStringParam = pEnv->NewStringUTF("android_id");
    jandroid_id = (jstring)pEnv->CallStaticObjectMethod(secClass, secMid, contentObj, jStringParam);
    // Remember to release when done!
    pEnv->DeleteLocalRef(jStringParam);
  //}
  //catch (int e) {
  //  LOG_TRACE(("Caught exception %d!\n", e));
  //}
  return jandroid_id;
}
#endif
