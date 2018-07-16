#ifndef TASK_INFO_H_
#define TASK_INFO_H_
#include <vector>
#include <map>
#include "TypeInfo.h"
#include "GlobalObjectHelper.h"
#include "ConfigSet.h"

using namespace std;

struct TaskInfo
{
	char task_id_[64 + 1];
	char args_[128 + 1];
	char task_dir_[256 + 1];
	char input_rule_id_[64 + 1];
	int input_complete_status;
	/*output rule split with ;*/
	char output_rule_list_[128 + 1];
	int output_complete_num;
	/*default for 10000*/
	int64 task_hash_size_;

	void Print()
	{
		fprintf(stderr, "%s|%s|%s|%s|%d|%s|%d|%ld\n", 
			task_id_, args_, task_dir_, input_rule_id_, 
			input_complete_status, output_rule_list_, 
			output_complete_num, task_hash_size_);
	}
};

enum InOutStatus
{
	YES = 0,
	NO
};

typedef TaskInfo MainToInput;

struct InputToMain
{
	char task_id_[64 + 1];
	int input_index_;
	int data_space_index_;
	InOutStatus status;

	void Print()
	{
		fprintf(stderr, "InputToMain:%s|%d|%d|%d", 
			task_id_, input_index_, data_space_index_, status);
	}
};

struct MainToOutput
{
	char task_id_[64 + 1];
	int input_index_;
	int data_space_index_;
	char input_rule_id_[64 + 1];
	char output_rule_id_[64 + 1];

	/*default for 10000*/
	int64 task_hash_size_;
};

struct OutputToMain
{
	char task_id_[64 + 1];
	int output_rule_index_;
	InOutStatus status;
};

struct FileInputRule
{
	struct _start_and_len {
		int start_;
		int length_;
	};

	char text_split_[16 + 1];
	char deal_file_pre_flag_[64 + 1];
	char deal_file_flag_[64 + 1];
	char val_type_info_[64 + 1];
	vector<_start_and_len> key_rule_;
	vector<_start_and_len> value_rule_;

	/*
	size_t keyLen = 0;
	for (int i = 0; i < inRule->key_rule_.size(); ++i) {
	keyLen += inRule->key_rule_[i].length_; 
	}
	size_t valLen = inRule->value_rule_.size() * sizeof(int64);
	*/
	size_t key_length_;
	size_t val_length_;

	void PushBackKey(const string& elm)
	{
		_start_and_len sl;
		sl.start_ = atoi(elm.substr(0, elm.find_first_of(",")).c_str());
		sl.length_ = atoi(elm.substr(elm.find_first_of(",") + 1).c_str());
		key_rule_.push_back(sl);
	}

	void PushBackVal(const string& elm)
	{
		_start_and_len sl;
		sl.start_ = atoi(elm.substr(0, elm.find_first_of(",")).c_str());
		sl.length_ = atoi(elm.substr(elm.find_first_of(",") + 1).c_str());
		value_rule_.push_back(sl);
	}
};

struct FileOutputRule
{
	char out_text_split_[16 + 1];
	vector<int> key_in_;
	vector<int> val_in_;

	void PushBackKey(const string& keyStr)
	{
		key_in_.clear();
		vector<string> vec;
		PubCommon::SplitToVec(keyStr.c_str(), vec, ";");
		vector<string>::iterator it = vec.begin();
		for (; it != vec.end(); ++it) {
			key_in_.push_back(atoi(it->c_str()));
		}
	}

	void PushBackVal(const string& valStr)
	{
		val_in_.clear();
		vector<string> vec;
		PubCommon::SplitToVec(valStr.c_str(), vec, ";");
		vector<string>::iterator it = vec.begin();
		for (; it != vec.end(); ++it) {
			val_in_.push_back(atoi(it->c_str()));
		}
	}
};

class FileRule: public GlobalObjectHelper<FileRule>
{
public:
	~FileRule();

	int Load();
	FileInputRule* GetInputRule(const char* ruleId);
	FileOutputRule* GetOutputRule(const char* ruleId);

private:
	friend class GlobalObjectHelper<FileRule>;
	FileRule();

private:
	map<string, FileInputRule> input_rule_;
	map<string, FileOutputRule> output_rule_;
	volatile bool is_load_;
	
};

#endif
