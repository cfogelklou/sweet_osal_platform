#include "file_find_and_open.hpp"

#include "platform_log.h"
#include "utils/file_utils.hpp"

#include <stdio.h>
#include <stdlib.h>


MyFile::MyFile(const char* name, const char* mode)
  : f(nullptr) {
  std::string m = mode;
  std::string s = name;
  const auto fm = m.find('w');
  if (fm != std::string::npos) {
    LOG_TRACE(("Opening file %s for write with mode %s.\r\n", name, mode));
    f = fopen(name, mode);
  } else {
    std::string s1 = std::string("sau_src/test_data/") + name;
    s = FileUtils::getPathToFile(s1.c_str());
    if (s.length() > 0) {
      f = fopen(s.c_str(), mode);
    }
  }
  if (nullptr == f) {
    LOG_TRACE(("Failed to open file:%s at path %s\r\n", name, s.c_str()));
#ifndef LOG_DISABLE_ASSERT
    // Test binaries must fail fast with a readable message instead of
    // segfaulting on a nullptr MyFile later on. Release builds
    // (LOG_DISABLE_ASSERT) keep the historical nullptr-return contract so
    // MyOpenFiles::fopen() callers can handle the failure themselves.
    printf("Missing required file: %s (searched from cwd %s)\n", name,
           s.c_str());
    exit(-999);
#endif
  }
}

FILE* MyFile::getPtr() {
  return f;
}

void MyFile::doClose() {
  if (f) {
    fclose(f);
  }
  f = nullptr;
}

bool MyFile::isOpen() {
  return (nullptr != f);
}


MyOpenFiles& MyOpenFiles::inst() {
  static MyOpenFiles in;
  return in;
}

MyFile* MyOpenFiles::fopen(const char* name, const char* mode) {
  MyFile* p = new MyFile(name, mode);
  if (p->isOpen()) {
    uint64_t a  = (uint64_t)p;
    mFiles[ a ] = p;
    return p;
  } else {
    delete p;
    return nullptr;
  }
}

void MyOpenFiles::fclose(MyFile* p) {
  uint64_t a = (uint64_t)p;
  if (mFiles.find(a) != mFiles.end()) {
    p->doClose();
    mFiles.erase(a);
    delete p;
  }
}

void MyOpenFiles::clearAllForgotten() {
  if (!mFiles.empty()) {
    for (auto f : mFiles) {
      auto p = f.second;
      delete p;
      printf("Deleting a forgotten file\r\n");
    }
    mFiles.clear();
  }
}


FileDeleteRememberer::FileDeleteRememberer() {
}

// Delete all forgotten files
FileDeleteRememberer::~FileDeleteRememberer() {
  MyOpenFiles::inst().clearAllForgotten();
}
