#ifndef FILE_UTILS_HPP__
#define FILE_UTILS_HPP__

#ifdef __cplusplus
#include "utils/helper_macros.h"

#include <fstream>
class FileUtils {
public:
  // //////////////////////////////////////////////////////////////////////////
  static size_t getFilesize(const char* const filename) {
    std::ifstream in(filename, std::ifstream::ate | std::ifstream::binary);
    return (size_t)in.tellg();
  }

  // //////////////////////////////////////////////////////////////////////////
  static std::string getPathToFile(const char* fname) {
    std::string rval;
    std::string tmp = fname;
    std::string up  = "../";

    int fsize = (int)getFilesize(tmp.c_str());

    int tries = 10;
    while ((tries > 0) && (fsize < 0)) {
      tmp   = up + tmp;
      fsize = (int)getFilesize(tmp.c_str());
      tries--;
    }

    if (fsize > 0) {
      rval = tmp;
    }

    return rval;
  }

  typedef void (*ChunkFileCb)(void* p, int16_t samps[], const size_t numSamps);
  static void chunkRawFile(
    const std::string fileName, const size_t chunkSamps16Le,
    ChunkFileCb cb, void* p) {
    const std::string path = getPathToFile(fileName.c_str());
    auto size = getFilesize(path.c_str());
    std::fstream input(path, std::ios_base::in | std::ios_base::binary);
    if (input.is_open()) {
      int16_t* buf          = new int16_t[ chunkSamps16Le ];
      size_t remainingSamps = size / 2;
      while (remainingSamps > 0) {
        const size_t s = MIN(remainingSamps, chunkSamps16Le);
        const size_t bytes = s * sizeof(int16_t);

        // Read in from the file.
        memset(buf, 0, sizeof(int16_t) * chunkSamps16Le);
        input.read((char*)buf, bytes);

        // Call the callback
        if (cb) {
          cb(p, buf, chunkSamps16Le);
        }
        remainingSamps -= s;
      }

      delete[] buf;
    }
  }
};

#endif // #ifdef __cplusplus

#endif
