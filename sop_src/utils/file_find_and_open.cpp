#include "file_find_and_open.hpp"

#include "platform_log.h"
#include "utils/file_utils.hpp"


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
    LOG_ASSERT(nullptr != f);
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
