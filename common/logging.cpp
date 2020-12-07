#include "logging.h"

// bool EmitThreadIdFromEnv() {
//   const char* tf_env_var_val = getenv("TF_CPP_LOG_THREAD_ID");
//   return tf_env_var_val == nullptr
//              ? false
//              : ParseInteger(tf_env_var_val, strlen(tf_env_var_val)) != 0;
// }
// 
// int64 MinLogLevelFromEnv() {
// // We don't want to print logs during fuzzing as that would slow fuzzing down
// // by almost 2x. So, if we are in fuzzing mode (not just running a test), we
// // return a value so that nothing is actually printed. Since LOG uses >=
// // (see ~LogMessage in this file) to see if log messages need to be printed,
// // the value we're interested on to disable printing is the maximum severity.
// // See also http://llvm.org/docs/LibFuzzer.html#fuzzer-friendly-build-mode
// #ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
//   return tensorflow::NUM_SEVERITIES;
// #else
//   const char* tf_env_var_val = getenv("TF_CPP_MIN_LOG_LEVEL");
//   return LogLevelStrToInt(tf_env_var_val);
// #endif
// }
// 
// int64 MinVLogLevelFromEnv() {
// // We don't want to print logs during fuzzing as that would slow fuzzing down
// // by almost 2x. So, if we are in fuzzing mode (not just running a test), we
// // return a value so that nothing is actually printed. Since VLOG uses <=
// // (see VLOG_IS_ON in logging.h) to see if log messages need to be printed,
// // the value we're interested on to disable printing is 0.
// // See also http://llvm.org/docs/LibFuzzer.html#fuzzer-friendly-build-mode
// #ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
//   return 0;
// #else
//   const char* tf_env_var_val = getenv("TF_CPP_MIN_VLOG_LEVEL");
//   return LogLevelStrToInt(tf_env_var_val);
// #endif
// }
// 
// LogMessage::LogMessage(const char* fname, int line, int severity)
//     : fname_(fname), line_(line), severity_(severity) {}
// 
// LogMessage& LogMessage::AtLocation(const char* fname, int line) {
//   fname_ = fname;
//   line_ = line;
//   return *this;
// }
// 
// LogMessage::~LogMessage() {
//   // Read the min log level once during the first call to logging.
//   static int64 min_log_level = MinLogLevelFromEnv();
//   if (severity_ >= min_log_level) {
//     GenerateLogMessage();
//   }
// }
// 
// void LogMessage::GenerateLogMessage() {
//   static bool log_thread_id = EmitThreadIdFromEnv();
//   uint64 now_micros = EnvTime::NowMicros();
//   time_t now_seconds = static_cast<time_t>(now_micros / 1000000);
//   int32 micros_remainder = static_cast<int32>(now_micros % 1000000);
//   const size_t time_buffer_size = 30;
//   char time_buffer[time_buffer_size];
//   strftime(time_buffer, time_buffer_size, "%Y-%m-%d %H:%M:%S",
//            localtime(&now_seconds));
//   const size_t tid_buffer_size = 10;
//   char tid_buffer[tid_buffer_size] = "";
//   if (log_thread_id) {
//     snprintf(tid_buffer, sizeof(tid_buffer), " %7u",
//              absl::base_internal::GetTID());
//   }
//   // TODO(jeff,sanjay): Replace this with something that logs through the env.
//   fprintf(stderr, "%s.%06d: %c%s %s:%d] %s\n", time_buffer, micros_remainder,
//           "IWEF"[severity_], tid_buffer, fname_, line_, str().c_str());
// }
// #endif
// 
// int64 LogMessage::MinVLogLevel() {
//   static int64 min_vlog_level = MinVLogLevelFromEnv();
//   return min_vlog_level;
// }
// 
// bool LogMessage::VmoduleActivated(const char* fname, int level) {
//   if (level <= MinVLogLevel()) {
//     return true;
//   }
//   static VmoduleMap* vmodules = VmodulesMapFromEnv();
//   if (TF_PREDICT_TRUE(vmodules == nullptr)) {
//     return false;
//   }
//   const char* last_slash = strrchr(fname, '/');
//   const char* module_start = last_slash == nullptr ? fname : last_slash + 1;
//   const char* dot_after = strchr(module_start, '.');
//   const char* module_limit =
//       dot_after == nullptr ? strchr(fname, '\0') : dot_after;
//   StringData module(module_start, module_limit - module_start);
//   auto it = vmodules->find(module);
//   return it != vmodules->end() && it->second >= level;
// }
