#ifndef PUB_COMMON_H_
#define PUB_COMMON_H_

#include <string>
#include <stdio.h>
#include <string.h>
#include <vector>

/*static class*/
class PubCommon
{
public:
	static long GetNextDayFromNow();
	static bool IsTheFirstDayOfMonth();
	static bool IsTheFirstDayOfYear();
	static long GetTimeFromNow(const std::string& strTime);
	static void GetRealDate(const char* pattern, char* realDate, size_t len);
	static const char* ReplaceStr(char* srcStr, 
								  const char* rec, 
								  const char* tar, 
								  size_t maxLen);
	static void SplitToVec(const char *source, 
						   std::vector<std::string> &vec, 
						   const char *spl = " ");
	static char* TrimChr(char *source, size_t max_len);
	static bool IsBlank(const char* src);
	static char* ToUpper(char* src);
	static char* ToLower(char* src);
	static void ToDaemon();

	static int Read(int fd, void* data, int size);
	static int Write(int fd, const void* buff, int size);
	static int FdCanRead(int fd, int seconds);

private:
	static void GetNowFormat(const char* fmt, 
							 const char* cal, 
							 const char* num, 
							 char* out);
	
};

#endif
