#include "spdlog/spdlog.h"
#include <iostream>
#include <sstream>

namespace runtime {

#define CHECK(condition, ...) \
  if (!(condition)) {         \
    LOG(ERROR) << "Check failed: " << #condition "." #__VA_ARGS__;\
  } 

#define CHECK_EQ(ret, condition) \
    CHECK(ret == condition)

const int DEBUG   = 0;
const int INFO    = 1;
const int WARNING = 2;
const int ERROR   = 3;
const int FATAL   = 4;

#define _LOG_DEBUG \
  ::runtime::LogMessage(__FILE__, __LINE__, ::runtime::DEBUG)
#define _LOG_INFO \
  ::runtime::LogMessage(__FILE__, __LINE__, ::runtime::INFO)
#define _LOG_WARNING \
  ::runtime::LogMessage(__FILE__, __LINE__, ::runtime::WARNING)
#define _LOG_ERROR \
  ::runtime::LogMessage(__FILE__, __LINE__, ::runtime::ERROR)
#define _LOG_FATAL \
  ::runtime::LogMessage(__FILE__, __LINE__. ::runtime::FATAL)

#define LOG(severity) _LOG_##severity

class LogMessage : public std::basic_ostringstream<char> {
 public:
  LogMessage(const char* fname, int line, int severity);
  ~LogMessage() override;

  // Change the location of the log message.
  LogMessage& AtLocation(const char* fname, int line);

  // Returns the minimum log level for VLOG statements.
  // E.g., if MinVLogLevel() is 2, then VLOG(2) statements will produce output,
  // but VLOG(3) will not. Defaults to 0.
  static int64_t MinVLogLevel();

  // Returns whether VLOG level lvl is activated for the file fname.
  //
  // E.g. if the environment variable TF_CPP_VMODULE contains foo=3 and fname is
  // foo.cc and lvl is <= 3, this will return true. It will also return true if
  // the level is lower or equal to TF_CPP_MIN_VLOG_LEVEL (default zero).
  //
  // It is expected that the result of this query will be cached in the VLOG-ing
  // call site to avoid repeated lookups. This routine performs a hash-map
  // access against the VLOG-ing specification provided by the env var.
  static bool VmoduleActivated(const char* fname, int level);

 protected:
  void GenerateLogMessage();

 private:
  const char* fname_;
  int line_;
  int severity_;
};


}  // namespace runtime



