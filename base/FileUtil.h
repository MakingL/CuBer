// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
//
// This is a public header file, it must only include public header files.

#ifndef CUBER_BASE_FILEUTIL_H
#define CUBER_BASE_FILEUTIL_H

#include "base/noncopyable.h"
#include "base/StringPiece.h"
#include <sys/types.h>  // for off_t
#include <sys/stat.h>
#include <unistd.h>

namespace cuber
{
namespace FileUtil
{

class FileStat
{
private:
  struct stat stat_;
  bool exists_;
public:
    explicit FileStat() : exists_(false) {
    }
  explicit FileStat(const char* filename);
  ~FileStat() = default;
  bool exists();
  void setFileName(const char *filename);
  bool isfile();
  bool isfile(const char* filename);
  bool isdir();
  bool isdir(const char *dirname);
  int64_t size();
};


// read small file < 64KB
class ReadSmallFile : noncopyable
{
 public:
  ReadSmallFile(StringArg filename);
  ~ReadSmallFile();

  // return errno
  template<typename String>
  int readToString(int maxSize,
                   String* content,
                   int64_t* fileSize,
                   int64_t* modifyTime,
                   int64_t* createTime);

  /// Read at maxium kBufferSize into buf_
  // return errno
  int readToBuffer(int* size);

  const char* buffer() const { return buf_; }

  static const int kBufferSize = 64*1024;

 private:
  int fd_;
  int err_;
  char buf_[kBufferSize];
};

// read the file content, returns errno if error happens.
template<typename String>
int readFile(StringArg filename,
             int maxSize,
             String* content,
             int64_t* fileSize = NULL,
             int64_t* modifyTime = NULL,
             int64_t* createTime = NULL)
{
  ReadSmallFile file(filename);
  return file.readToString(maxSize, content, fileSize, modifyTime, createTime);
}

// not thread safe
class AppendFile : noncopyable
{
 public:
  explicit AppendFile(StringArg filename);

  ~AppendFile();

  void append(const char* logline, size_t len);

  void flush();

  off_t writtenBytes() const { return writtenBytes_; }

 private:

  size_t write(const char* logline, size_t len);

  FILE* fp_;
  char buffer_[64*1024];
  off_t writtenBytes_;
};

}  // namespace FileUtil
}  // namespace cuber

#endif  // CUBER_BASE_FILEUTIL_H

