#ifndef OUTPUT_SUBPROCESS_H_
#define OUTPUT_SUBPROCESS_H_

#include "Hash.h"
#include "TaskInfo.h"
#include "ProcMemoryManager.h"

class CallBackDefault: public CallBackBase
{
	struct InputRulePos
	{
		int start_;
		int len_;
	};

public:
	CallBackDefault(FileInputRule* inRule, FileOutputRule* outRule);
	virtual ~CallBackDefault();

	void Print();
	void Process(const void* key, const void* value);

private:
	map<string, void*> data_;
	vector<InputRulePos> in_key_pos_;
	FileOutputRule* out_rule_;

};

class OutputSubProcess
{
public:
	OutputSubProcess(int index, int fdR, int fdW, IoInterface* io);
	~OutputSubProcess();

	int Process();

private:
	int proc_index_;
	int fd_read_;
	int fd_write_;
	IoInterface* io_oper_;
};

#endif
