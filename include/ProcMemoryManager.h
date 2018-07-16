#ifndef PROC_MEMORY_MANAGER_H_
#define PROC_MEMORY_MANAGER_H_
#include <vector>
#include "MemoryInfo.h"
#include "MemoryManager.h"

class ProcMemoryManager: public MemoryManager
{
public:
	ProcMemoryManager(IoInterface* io, int ProcSeq);
	virtual ~ProcMemoryManager();

	void AttachNewTask(const char* taskInfo);
	int GetTaskSpaceEntrySeq();
	virtual int64 Allocate(int64 mSize);

	ProcGlob* GetProcGlob();
	ShmHead* GetHead();
	ProcSpaceHead* GetProcSpaceHeadEntry();
	ProcSpaceHead* GetProcSpaceHead(int seq);
	ProcSpaceHead* GetProcSpaceHeadFree();

	void Print();

private:
	int proc_seq_;

	ProcSpaceHead* now_proc_space_;
	int64 use_len_;
	char task_run_flag_[128];
	int task_space_entry_seq_;

};

#endif // !PROC_MEMORY_MANAGER_H_
