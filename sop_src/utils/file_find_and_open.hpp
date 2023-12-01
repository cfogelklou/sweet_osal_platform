#ifndef FILE_FIND_AND_OPEN_HPP
#define FILE_FIND_AND_OPEN_HPP 1
#include <map>
#include <string>

class MyFile {
public:
  MyFile(const char* name, const char* mode);

  FILE* getPtr();
  void doClose();
  bool isOpen();

private:
  FILE* f;
};

class MyOpenFiles {
public:
  std::map<uint64_t, MyFile*> mFiles;
  static MyOpenFiles& inst();

  MyFile* fopen(const char* name, const char* mode);

  void fclose(MyFile* p);

  void clearAllForgotten();
};


class FileDeleteRememberer {
public:
  FileDeleteRememberer();
  ~FileDeleteRememberer();
};


#define FOPEN(n, m) MyOpenFiles::inst().fopen(n, m)
#define FCLOSE(p) MyOpenFiles::inst().fclose(p)

#endif