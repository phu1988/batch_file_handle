#ifndef CONFIG_SET_
#define CONFIG_SET_

#include "GlobalObjectHelper.h"
#include "PubCommon.h"
#include <string>
#include <pthread.h>
#include <stdio.h>
#include <string>
#include <map>
#include "LogInstance.h"

using namespace std;

typedef map<string, map<string, string> > ConfigData;
typedef map<string, map<string, string> >::iterator ConfigPtr;
typedef map<string, string>::iterator SectionPtr;

class ConfigSet: public GlobalObjectHelper<ConfigSet>
{
public:
	~ConfigSet();

	int Initialize(const char* configFile);
	const char* Get(const char* sec, const char* key);
	ConfigData* GetAll();
	void Print();

private:
	friend class GlobalObjectHelper<ConfigSet>;
	ConfigSet();

private:
	char config_file_path_[256 + 1];
	volatile bool is_initialize_;

	map<string, map<string, string> > map_;
};

#endif
