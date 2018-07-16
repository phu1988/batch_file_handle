#include "TaskInfo.h"
#include <pthread.h>

FileRule::FileRule()
	: is_load_(false)
{

}

FileRule::~FileRule()
{

}

int FileRule::Load()
{
	if (!__sync_bool_compare_and_swap(&is_load_, 0, 1)) return 1;

	ConfigData* data = ConfigSet::GetInstance()->GetAll();
	ConfigPtr ptr = data->begin();
	for (; ptr != data->end(); ++ptr) {
		if (0 == strncmp("INPUT_RULE", ptr->first.c_str(), strlen("INPUT_RULE"))) {
			FileInputRule rule;
			memset(&rule, 0, sizeof(rule));

			SectionPtr sPtr = ptr->second.begin();
			for (; sPtr != ptr->second.end(); ++sPtr) {
				if (sPtr->first == "TEXT_SPLIT") {
					strcpy(rule.text_split_, sPtr->second.c_str());
				}

				if (sPtr->first == "DEAL_FILE_PRE_FLAG") {
					strcpy(rule.deal_file_pre_flag_, sPtr->second.c_str());
				}

				if (sPtr->first == "DEAL_FILE_FLAG") {
					strcpy(rule.deal_file_flag_, sPtr->second.c_str());
				}

				if (sPtr->first == "VAL_TYPE_INFO") {
					strcpy(rule.val_type_info_, sPtr->second.c_str());
				}

				if (sPtr->first == "KEY_RULE" || sPtr->first == "VAL_RULE") {
					char buffer[256 + 1] = {0};
					strncpy(buffer, sPtr->second.c_str(), sizeof(buffer) - 1);
					vector<string> aKey;
					PubCommon::SplitToVec(buffer, aKey, ";");
					vector<string>::iterator it = aKey.begin();
					for (; it != aKey.end(); ++it) {
						if (sPtr->first == "KEY_RULE") {
							rule.PushBackKey(*it);
						} else {
							rule.PushBackVal(*it);
						}
					}
				}
			}
			
			for (int i = 0; i < rule.key_rule_.size(); ++i) {
				rule.key_length_ += rule.key_rule_[i].length_;
			}
			rule.val_length_ = rule.value_rule_.size() * sizeof(int64);

			input_rule_.insert(make_pair(ptr->first, rule));
		}

		if (0 == strncmp("OUTPUT_RULE", ptr->first.c_str(), strlen("OUTPUT_RULE"))) {
			FileOutputRule rule;
			memset(&rule, 0, sizeof(rule));

			SectionPtr sPtr = ptr->second.begin();
			for (; sPtr != ptr->second.end(); ++sPtr) {
				if (sPtr->first == "OUT_TEXT_SPLIT") {
					strcpy(rule.out_text_split_, sPtr->second.c_str());
				}

				if (sPtr->first == "KEY_IN") {
					rule.PushBackKey(sPtr->second);
				}

				if (sPtr->first == "VAL_IN") {
					rule.PushBackVal(sPtr->second);
				}
			}

			output_rule_.insert(make_pair(ptr->first, rule));
		}
	}
	return 0;
}

FileInputRule* FileRule::GetInputRule(const char* ruleId)
{
	map<string, FileInputRule>::iterator it = input_rule_.find(ruleId);
	if (it == input_rule_.end()) return NULL;

	return &(it->second);
}

FileOutputRule* FileRule::GetOutputRule(const char* ruleId)
{
	map<string, FileOutputRule>::iterator it = output_rule_.find(ruleId);

	if (it == output_rule_.end()) return NULL;

	return &(it->second);
}
