#ifndef FILE_UTILS_HPP__
#define FILE_UTILS_HPP__

#ifdef __cplusplus
#include <fstream>
#include "utils/helper_macros.h"
class FileUtils {
public:

  // //////////////////////////////////////////////////////////////////////////
  static size_t getFilesize(const char* const filename){
    std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
    return (size_t)in.tellg();
  }

  // //////////////////////////////////////////////////////////////////////////
  static std::string getPathToFile(const char* fname) {
    std::string rval;
    std::string tmp = fname;
    std::string up = "../";

    int fsize = (int)getFilesize(tmp.c_str());

    int tries = 10;
    while ((tries > 0) && (fsize < 0)) {
      tmp = up + tmp;
      fsize = (int)getFilesize(tmp.c_str());
      tries--;
    }

    if (fsize > 0) {
      rval = tmp;
    }

    return rval;
  }

};

#endif // #ifdef __cplusplus

#endif
