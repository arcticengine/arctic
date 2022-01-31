#pragma once

#include "piTypes.h"

namespace arctic {

class piFile {
public:
  piFile();
  ~piFile();

  static bool DirectoryCreate(const wchar_t *name, bool failOnExists);
  static bool DirectoryExists(const wchar_t *dirName_in);
  static bool Exists(const wchar_t *name);
  static bool HaveWriteAccess( const wchar_t *name );
  static bool Copy(const wchar_t *dst, const wchar_t *src, bool failIfexists);

  typedef enum {
    CURRENT = 0,
    END = 1,
    SET = 2
  }SeekMode;

  bool Open(const wchar_t *name, const wchar_t *mode);
  bool Seek(uint64 pos, SeekMode mode);
  uint64 Tell(void);
  void Close( void );
  uint64 GetLength( void );

  //----------------------

  char* ReadString(char *buffer, int num);
  void Prints(const wchar_t *str);
  void Printf(const wchar_t *format, ...);

  //-----------------------



private:
  void *mInternal;
};

} // namespace piLibs
