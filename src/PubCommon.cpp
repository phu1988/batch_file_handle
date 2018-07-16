#include <time.h>
#include <regex.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>
#include "PubCommon.h"
#include "LogInstance.h"

long PubCommon::GetNextDayFromNow()
{
	time_t today, tomorrow;
	time(&today);
	tomorrow = today + 86400;

	tm* tm_tomarrow = localtime(&tomorrow);
	tm_tomarrow->tm_hour = 0;
	tm_tomarrow->tm_min = 0;
	tm_tomarrow->tm_sec = 0;

	tomorrow = mktime(tm_tomarrow);

	return tomorrow - today;
}

bool PubCommon::IsTheFirstDayOfMonth()
{
	time_t today;
	time(&today);

	tm* tm_today = localtime(&today);

	return (1 == tm_today->tm_mday);
}

bool PubCommon::IsTheFirstDayOfYear()
{
	time_t today;
	time(&today);

	tm* tm_today = localtime(&today);

	return (1 == tm_today->tm_mday && 0 == tm_today->tm_mon);
}

long PubCommon::GetTimeFromNow(const std::string& strTime)
{
	tm tm_b_time;
	tm_b_time.tm_year = atoi(strTime.substr(0, 4).c_str()) - 1900;
	tm_b_time.tm_mon = atoi(strTime.substr(4, 2).c_str()) - 1;
	tm_b_time.tm_mday = atoi(strTime.substr(6, 2).c_str());
	tm_b_time.tm_hour = atoi(strTime.substr(8, 2).c_str());
	tm_b_time.tm_min = atoi(strTime.substr(10, 2).c_str());
	tm_b_time.tm_sec = atoi(strTime.substr(12, 2).c_str());

	time_t now;
	time(&now);

	return mktime(&tm_b_time) - now;
}

void PubCommon::GetRealDate(const char* srcStr, char* realDate, size_t maxLen)
{
#define MATCH_NUM 4

	const char* pattern = 
		"<\\(YYYY\\|YYYYMM\\|YYYYMMDD\\)\\([\\+\\|-]\\{0,1\\}\\)\\([0-9]*\\)>";

	regex_t reg;
	regmatch_t mth[MATCH_NUM];
	char eBuff[255 + 1] = {0};
	int ret = regcomp(&reg, pattern, 0);
	if (0 != ret) {
		regerror(ret, &reg, eBuff, sizeof(eBuff));
		LOGERR("err pattern: %s\n", eBuff);
		return;
	}

	ret = regexec(&reg, srcStr, MATCH_NUM, mth, 0);
	if (ret == REG_NOMATCH) {
		LOGINFO("Pattern: %s   no match", srcStr);
	} else if (0 != ret) {
		regerror(ret, &reg, eBuff, sizeof(eBuff));
		LOGERR("regexec err: %s\n", eBuff);	
	} else {
		char all[16 + 1] = {0}, fmt[8 + 1] = {0}, cal[1 + 1] = {0};
		char num[8 + 1] = {0}, outDate[8 + 1] = {0};
		strncpy(all, srcStr + mth[0].rm_so, mth[0].rm_eo - mth[0].rm_so);
		strncpy(fmt, srcStr + mth[1].rm_so, mth[1].rm_eo - mth[1].rm_so);
		strncpy(cal, srcStr + mth[2].rm_so, mth[2].rm_eo - mth[2].rm_so);
		strncpy(num, srcStr + mth[3].rm_so, mth[3].rm_eo - mth[3].rm_so);
		GetNowFormat(fmt, cal, num, outDate);

		strncpy(realDate, srcStr, maxLen - 1);
		ReplaceStr(realDate, all, outDate, maxLen);
	}

	regfree(&reg);
}

void PubCommon::GetNowFormat(const char* fmt, 
							 const char* cal, 
							 const char* num, 
							 char* out)
{
	time_t today;
	time(&today);

	if (0 == strcmp("YYYYMMDD", fmt)) {
		if (0 == strcmp("+", cal)) {
			today += 24 * 60 * 60 * atoi(num);
		} else if (0 == strcmp("-", cal)) {
			today -= 24 * 60 * 60 * atoi(num);
		}

		tm* tm_today = localtime(&today);
		sprintf(out, "%04d%02d%02d", 
			tm_today->tm_year + 1900, tm_today->tm_mon + 1, tm_today->tm_mday);
	} else if (0 == strcmp("YYYYMM", fmt)) {
		if (0 == strcmp("+", cal)) {
			today += 30 * 24 * 60 * 60 * atoi(num);
		} else if (0 == strcmp("-", cal)) {
			today -= 30 * 24 * 60 * 60 * atoi(num);
		}

		tm* tm_today = localtime(&today);
		sprintf(out, "%04d%02d", tm_today->tm_year + 1900, tm_today->tm_mon + 1);
	} else if (0 == strcmp("YYYY", fmt)) {
		if (0 == strcmp("+", cal)) {
			today += 365 * 24 * 60 * 60 * atoi(num);
		} else if (0 == strcmp("-", cal)) {
			today -= 365 * 24 * 60 * 60 * atoi(num);
		}

		tm* tm_today = localtime(&today);
		sprintf(out, "%04d", tm_today->tm_year + 1900);
	} else {
		tm* tm_today = localtime(&today);
		sprintf(out, "%04d%02d%02d", 
			tm_today->tm_year + 1900, tm_today->tm_mon + 1, tm_today->tm_mday);
	}
}

const char* PubCommon::ReplaceStr(char* source, 
							      const char* rec, 
							      const char* tar, 
							      size_t maxLen)
{
	if (NULL == source || 0 == strcmp("", source)) {
		return NULL;
	}

	int count = 0;
	char *buff = new char[maxLen];
	memset(buff, 0, maxLen);
	char *p = source;
	char *t = buff;

	while (*p != '\0') {
		if (0 == strncmp(rec, p, strlen(rec))) {
			strcpy(t, tar);
			p += strlen(rec);
			t += strlen(tar);
			++count;
		} else {
			*(t++) = *(p++);
		}
	}

	memset(source, 0, maxLen);
	strncpy(source, buff, maxLen - 1);

	delete []buff;
	return source;
}

void PubCommon::SplitToVec(const char *source, 
								  std::vector<std::string> &vec, 
								  const char *spl)
{
	if (0 == strcmp("", source) || NULL == source) {
		return;
	}

	char tmp[2048 + 1] = {0};
	const char *p = source;
	int i = 0;
	if (0 == strncmp(p, spl, strlen(spl))) p += strlen(spl);
	while (*p != '\0') {
		if (0 == strncmp(p, spl, strlen(spl))) {
			if ('\0' != tmp[0]) {
				vec.push_back(tmp);
			}

			memset(tmp, 0, sizeof(tmp));
			i = 0;
			p += strlen(spl);
		} else if ('\r' == *p || '\t' == *p || '\n' == *p) {
			++p;
		} else {
			tmp[i++] = *(p++);
		}
	}

	if (i > 0) {
		vec.push_back(tmp);
	}
}

char* PubCommon::TrimChr(char *source, size_t max_len)
{
	ReplaceStr(source, " ", "", max_len);
	ReplaceStr(source, "\n", "", max_len);
	ReplaceStr(source, "\t", "", max_len);
	ReplaceStr(source, "\r", "", max_len);

	return source;
}

bool PubCommon::IsBlank(const char* src)
{
	if (NULL == src) return true;

	while (*src != '\0') {
		if (' ' != *src && '\t' != *src && '\r' != *src && '\n' != *src)
			return false;
		++src;
	}

	return true;
}

char* PubCommon::ToUpper(char* src)
{
	if (NULL == src) return NULL;

	char* pos = src;
	while (*pos != '\0') {
		if (*pos <= 'z' && *pos >= 'a') {
			*pos += ('A' - 'a');
		}
		++pos;
	}

	return src;
}

char* PubCommon::ToLower(char* src)
{
	if (NULL == src) return NULL;

	char* pos = src;
	while (*pos != '\0') {
		if (*pos <= 'Z' && *pos >= 'A') {
			*pos += ('a' - 'A');
		}
		++pos;
	}

	return src;
}

void PubCommon::ToDaemon()
{
	int i;
	pid_t pid;
	if (pid = fork()) {
		exit(0);
	} else if(pid < 0) {
		exit(-1);
	}

	if(setsid()<0) {
		exit(-1);
	}

	if (signal(SIGHUP,SIG_IGN) < 0) {
		exit(-1);
	}

	if (pid = fork()) {
		exit(0);
	} else if(pid < 0) {
		exit(-1);
	}

	int fdi = open("/dev/null", O_RDWR);
	dup2(fdi, 0);
	dup2(fdi, 1);
	/*dup2(fdi, 2);*/
}

int PubCommon::Read(int fd, void* data, int size)
{
	char *buf = (char*)data;
	int pos = 0, left = size;
	while (left) {
		int rt = read(fd, buf + pos, left);
		if (-1 == rt && EINTR == errno) continue;
		if (-1 == rt || 0 == rt) return rt;
		pos += rt;
		left -= rt;
	}

	return pos;
}

int PubCommon::Write(int fd, const void* buff, int size)
{
	int left = size;
	int pos = 0;
	const char *p = (const char*)buff;

	while ( left ) {
		int rs = 0;
		while (true) {
			rs = write(fd, p + pos, left);
			if (-1 == rs && EINTR == errno) continue;
			break;
		}
		if ( -1 == rs || 0 == rs ) return rs;
		left -= rs;
		pos += rs;
	}

	return pos;
}

int PubCommon::FdCanRead(int fd, int seconds)
{
	fd_set fdSet;
	FD_ZERO(&fdSet);
	FD_SET(fd, &fdSet);
	struct timeval tVal = {seconds, 0};

	while (true) {
		int ret = select(fd + 1, &fdSet, NULL, NULL, seconds < 0? NULL: &tVal);
		if (ret == -1 && errno == EINTR) continue;
		return ret;
	}
}

