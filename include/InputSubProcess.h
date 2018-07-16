#ifndef INPUT_SUBPROCESS_H_
#define INPUT_SUBPROCESS_H_
#include "Hash.h"
#include "TaskInfo.h"
#include "ProcMemoryManager.h"

class InputSubProcess
{
public:
	InputSubProcess(int procIndex, int fdR, int fdW, IoInterface* io);
	~InputSubProcess();

	int Process();

private:
	int DealFiles(const MainToInput& m2i);
	int GetFiles(const char* dir, const char* preFlag, vector<string>& files);
	void TransToRecord(const char* buffer, const FileInputRule* inRule, 
		char* keyBuffer, int keyLen, char* valBuffer,int valLen);

private:
	int proc_index_;
	int fd_read_;
	int fd_write_;
	ProcMemoryManager* proc_memory_man_;

};

#endif
