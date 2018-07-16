#ifndef CONTROL_H_
#define CONTROL_H_
#include <unistd.h>
#include <sys/types.h>
#include <map>
#include <vector>
#include "IoShmMemory.h"
#include "ProcMemoryManager.h"
#include "MemoryInfo.h"
#include "TaskInfo.h"

using namespace std;

class Control: public GlobalObjectHelper<Control>
{
public:
	~Control();

	int Initialize();
	void End();
	void PrintMemoryInfo(bool isAll = true);

	int SendToInput(const MainToInput& m2i);
	int ScanFromInputProc(vector<InputToMain>& i2ms);
	int SendToOutput(const MainToOutput& m2o);
	int ScanFromOutputProc(vector<OutputToMain>& o2ms);

private:
	friend class GlobalObjectHelper<Control>;
	Control();

private:
	int CreateInProcAndInit();
	int CreateOutProcAndInit();
	void SetProcGlobMemory(pid_t pid, int index);
	void SetProcSpaceHeadMemory(pid_t pid, int index);

private:
	IoInterface* io_oper_;
	
	struct _pipe_read_write
	{
		int seq;
		bool status;
		int read_;
		int write_;
	};

	enum _pipe_rw_flag
	{
		FDR = 0,
		FDW
	};

	enum _proc_status
	{
		IDLE = 0,
		BUSY,
		BROKEN
	};

	map<pid_t, _pipe_read_write> pid_in_pipe_;
	map<pid_t, _pipe_read_write> pid_out_pipe_;
	map<pid_t, ProcMemoryManager*> proc_memory_manager_;

};

#endif
