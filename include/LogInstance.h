#ifndef LOG_INSTANCE_H_
#define LOG_INSTANCE_H_

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string>
#include <string.h>
#include "GlobalObjectHelper.h"

using namespace std;

#define LOGERR(...) LogInstance::GetInstance()->Write("ERROR", __VA_ARGS__);
#define LOGWARN(...) LogInstance::GetInstance()->Write("WARN", __VA_ARGS__);
#define LOGINFO(...) LogInstance::GetInstance()->Write("INFO", __VA_ARGS__);

#ifdef DDEBUG
#define LOGDEBUG(...) LogInstance::GetInstance()->Write("DEBUG", __VA_ARGS__);
#else
#define LOGDEBUG(...) /*do nothing*/
#endif

class LogInstance: public GlobalObjectHelper<LogInstance>
{
public:
	~LogInstance();
	int Initialize(const char* logPath, 
		           const char* moduleFlag, 
				   long long flushSize = 0, 
				   long long timeLimit = 0);
	void Write(const char* logLevel, const char* fmt, ...);
	void Flush();

private:
	friend class GlobalObjectHelper<LogInstance>;
	LogInstance();

private:
	char log_path_[256 + 1];
	char module_flag_[64 + 1];
	char now_log_file_[256 + 1];
	FILE* fp_log_;
	long long flush_size_;
	long long time_limit_;

	long long now_log_size_;
	time_t now_time_ticket_;

	volatile bool is_initialize_;
};

#endif
