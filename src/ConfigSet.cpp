#include <errno.h>
#include "ConfigSet.h"

ConfigSet::ConfigSet()
	: is_initialize_(false)
{
	memset(config_file_path_, 0, sizeof(config_file_path_));
}

ConfigSet::~ConfigSet()
{

}

int ConfigSet::Initialize(const char* configFile)
{
	if (!__sync_bool_compare_and_swap(&is_initialize_, 0, 1)) return 1;

	strncpy(config_file_path_, configFile, sizeof(config_file_path_) - 1);

	FILE* fp = fopen(config_file_path_, "rb");
	if (NULL == fp) {
		fprintf(stderr, "Open Config File[%s] Error[%d]:%s", 
			config_file_path_, errno, strerror(errno));
		return -1;
	}

	map<string, string> mapSec;
	char secKey[64] = {0};
	char buffer[512 + 1] = {0};
	while (fgets(buffer, sizeof(buffer), fp) != NULL) {
		if (PubCommon::IsBlank(buffer)) continue;
		PubCommon::TrimChr(buffer, sizeof(buffer));
		if ('#' == buffer[0]) continue;
		
		if ('[' == buffer[0] && ']' == buffer[strlen(buffer) - 1]) {
			/*parent*/
			if (mapSec.size() > 0 && secKey[0] != '\0') {
				map_.insert(pair<string, map<string, string> >(secKey, mapSec));
			}

			mapSec.clear();
			strncpy(secKey, buffer + 1, strlen(buffer) - 2);
			PubCommon::ToUpper(secKey);
			continue;
		}

		/*child*/
		char* strIndex = strstr(buffer, "=");
		*strIndex = '\0';
		if (NULL == strIndex) {
			LOGWARN("Config Error String:%s", buffer);
			continue;
		}

		mapSec.insert(pair<string, string>(PubCommon::ToUpper(buffer), 
                                           strIndex + 1));
	}

	if (mapSec.size() > 0 && secKey[0] != '\0') {
		map_.insert(pair<string, map<string, string> >(secKey, mapSec));
	}

	fclose(fp);

	return 0;
}

const char* ConfigSet::Get(const char* sec, const char* key)
{
	char strSec[64 + 1] = {0};
	char strKey[64 + 1] = {0};
	strncpy(strSec, sec, sizeof(strSec) - 1);
	strncpy(strKey, key, sizeof(strKey) - 1);

	ConfigPtr ptr = map_.find(PubCommon::ToUpper(strSec));
	if (ptr == map_.end()) return NULL;

	SectionPtr sPtr = ptr->second.find(PubCommon::ToUpper(strKey));
	if (sPtr == ptr->second.end()) return NULL;

	return sPtr->second.c_str();
}

void ConfigSet::Print()
{
	std::string pBuffer = "\n";
	for (ConfigPtr ptr = map_.begin(); ptr != map_.end(); ++ptr) {
		pBuffer += "[";
		pBuffer += ptr->first.c_str();
		pBuffer += "]\n";
		for (SectionPtr ptr1 = ptr->second.begin(); 
			ptr1 != ptr->second.end(); ++ptr1)  {
			pBuffer += ptr1->first.c_str();
			pBuffer += "=";
			pBuffer += ptr1->second.c_str();
			pBuffer += "\n";
		}
	}

	printf(pBuffer.c_str());
}

ConfigData* ConfigSet::GetAll()
{
	return &map_;
}
