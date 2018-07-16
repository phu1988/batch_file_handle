#include "LogInstance.h"
#include <stdarg.h>

LogInstance::LogInstance()
	: fp_log_(NULL)
	, flush_size_(-1)
	, time_limit_(-1)
	, now_log_size_(-1)
	, now_time_ticket_(-1)
	, is_initialize_(false)
{
	memset(log_path_, 0, sizeof(log_path_));
	memset(module_flag_, 0, sizeof(module_flag_));
	memset(now_log_file_, 0, sizeof(now_log_file_));
}

LogInstance::~LogInstance()
{
	if (fp_log_ != NULL) {
		fclose(fp_log_);
		fp_log_ = NULL;
	}
}

int LogInstance::Initialize(const char* logPath, 
							const char* moduleFlag, 
							long long flushSize, 
							long long timeLimit)
{
	if (!__sync_bool_compare_and_swap(&is_initialize_, 0, 1)) return 1;

	if (NULL != logPath) {
		strncpy(log_path_, logPath, sizeof(log_path_) - 1);
		if (log_path_[strlen(log_path_) - 1] != '/') {
			strncat(log_path_, "/", sizeof(log_path_) - 1);
		}
	}

	strncpy(module_flag_, moduleFlag, sizeof(module_flag_) - 1);
	flush_size_ = flushSize;
	time_limit_ = timeLimit;
	now_log_size_ = 0;
	time(&now_time_ticket_);

	return 0;
}

void LogInstance::Flush()
{
	if (fp_log_) {
		fflush(fp_log_);
		now_log_size_ = 0;
		time(&now_time_ticket_);
	}
}

void LogInstance::Write(const char* logLevel, const char* fmt, ...)
{
	if (!is_initialize_) {
		fprintf(stderr, "LogInstance not init!\n");
		return;
	}

	char buffer[4096] = {0};
	time_t now;
	time(&now);
	tm* tm_now = localtime(&now);
	snprintf(buffer, sizeof(buffer), "[%s][%d]%04d%02d%02d%02d%02d%02d: ", 
		logLevel, 
		getpid(),
		tm_now->tm_year + 1900, 
		tm_now->tm_mon + 1, 
		tm_now->tm_mday, 
		tm_now->tm_hour, 
		tm_now->tm_min, 
		tm_now->tm_sec );
	int wLen = strlen(buffer);

	va_list ap;
	va_start(ap, fmt);
	vsnprintf(buffer + wLen, sizeof(buffer) - wLen, fmt, ap);
	va_end(ap);

	strncat(buffer, "\n", sizeof(buffer) - 1);
	wLen = strlen(buffer);

	/*console*/
	if (strlen(log_path_) == 0) {
		fprintf(stderr, buffer);
		return;
	}

	char aLogPath[256 + 1] = {0};
	snprintf(aLogPath, sizeof(aLogPath), "%s%s.%04d%02d.log", 
		log_path_, module_flag_, tm_now->tm_year + 1900, tm_now->tm_mon + 1);
	if (0 != strcmp(aLogPath, now_log_file_)) {
		/*close the old file and create new file*/
		if (NULL != fp_log_) {
			fclose(fp_log_);
			fp_log_ = NULL;
		}

		fp_log_ = fopen(aLogPath, "a+");
		if (NULL == fp_log_) {
			fprintf(stderr, "Can not open the log file\n");
			return;
		}

		strncpy(now_log_file_, aLogPath, sizeof(now_log_file_) - 1);

		fwrite(buffer, wLen, 1, fp_log_);
		now_log_size_ += wLen;
	} else {
		if (NULL == fp_log_) {
			fprintf(stderr, "There is no file handle exists, create again\n");
			fp_log_ = fopen(now_log_file_, "a+");
		}

		fwrite(buffer, wLen, 1, fp_log_);
		now_log_size_ += wLen;
	}

	if (flush_size_ <= 0) {
		Flush();
	} else if (now_log_size_ >= flush_size_ || 
		now - now_time_ticket_ >= time_limit_) {
		Flush();
	}
}
