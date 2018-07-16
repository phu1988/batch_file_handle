#include "ProcMemoryManager.h"

ProcMemoryManager::ProcMemoryManager(IoInterface* io, int ProcSeq)
	: MemoryManager(io)
	, proc_seq_(ProcSeq)
	, now_proc_space_(NULL)
	, use_len_(0)
	, task_space_entry_seq_(-1)
{
	memset(task_run_flag_, 0, sizeof(task_run_flag_));
}

ProcMemoryManager::~ProcMemoryManager()
{
}

void ProcMemoryManager::AttachNewTask(const char* taskInfo)
{
	strncpy(task_run_flag_, taskInfo, sizeof(task_run_flag_) - 1);
	/*reset the space and len*/
	now_proc_space_ = NULL;
	use_len_ = 0;
	task_space_entry_seq_ = -1;
}

int ProcMemoryManager::GetTaskSpaceEntrySeq()
{
	return task_space_entry_seq_;
}

int64 ProcMemoryManager::Allocate(int64 mSize)
{
	if (now_proc_space_) {
		if (use_len_ + mSize <= now_proc_space_->data_len_) {
			use_len_ += mSize;
			return now_proc_space_->start_addr_ + use_len_ - mSize;
		}
	}

	while (true) {
		/*1.get the free process space*/
		now_proc_space_ = GetProcSpaceHeadFree();
		/*there is no free space*/
		if (NULL == now_proc_space_) return -1;
		/*2.judge have enough space*/
		if (mSize > now_proc_space_->data_len_) continue;
		/*3.set the task info*/
		strncpy(now_proc_space_->task_run_flag_, 
			task_run_flag_, sizeof(now_proc_space_->task_run_flag_) - 1);
		/*4.set use status*/
		now_proc_space_->use_status_ = 1;
		/*5.set the free point, free count*/
		GetProcGlob()->memory_free_count_--;
		/*6.set the free space entry*/
		int i = 0;
		for (; i < GetProcGlob()->memory_split_count_; ++i) {
			if (0 == GetProcSpaceHead(i)->use_status_) {
				GetProcGlob()->proc_space_head_free_entry_ = 
					GetProcGlob()->proc_addr_ + sizeof(ProcSpaceHead) * i;
				LOGINFO("ProcIndex[%d]TaskId[%s], Now Space[%d], Next Free[%d]", 
					proc_seq_, task_run_flag_, now_proc_space_->area_seq_, i);
				break;
			}
		}

		if (i == GetProcGlob()->memory_split_count_) {
			GetProcGlob()->proc_space_head_free_entry_ = -1;
		}

		/*7.set use len*/
		use_len_ = mSize;
		/*8.set task init proc space seq*/
		if (task_space_entry_seq_ < 0) {
			task_space_entry_seq_ = now_proc_space_->area_seq_;
		}

		return now_proc_space_->start_addr_;
	}

	return -1;
}

ProcGlob* ProcMemoryManager::GetProcGlob()
{
	return io_oper_->GetValue<ProcGlob>(
		sizeof(ShmHead) + proc_seq_ * sizeof(ProcGlob));
}

ShmHead* ProcMemoryManager::GetHead()
{
	return io_oper_->GetValue<ShmHead>(0);
}

ProcSpaceHead* ProcMemoryManager::GetProcSpaceHeadEntry()
{
	return io_oper_->GetValue<ProcSpaceHead>(GetProcGlob()->proc_addr_);
}

ProcSpaceHead* ProcMemoryManager::GetProcSpaceHead(int seq)
{
	assert(seq < GetProcGlob()->memory_split_count_);

	return io_oper_->GetValue<ProcSpaceHead>(GetProcGlob()->proc_addr_ 
		+ sizeof(ProcSpaceHead) * seq);
}

ProcSpaceHead* ProcMemoryManager::GetProcSpaceHeadFree()
{
	if (GetProcGlob()->proc_space_head_free_entry_ < 0) return NULL;

	return io_oper_->GetValue<ProcSpaceHead>(
		GetProcGlob()->proc_space_head_free_entry_);
}

void ProcMemoryManager::Print()
{
	/*ProcGlob*/
	char outInfo[4096] = {0};
	snprintf(outInfo, sizeof(outInfo), 
		"=========================Process[%d:%d] free[%d]========================\n", 
		proc_seq_, 
		GetProcGlob()->process_pid_, 
		GetProcGlob()->memory_free_count_
	);

	for (int i = 0; i < GetProcGlob()->memory_split_count_; ++i) {
		snprintf(outInfo + strlen(outInfo), sizeof(outInfo) - strlen(outInfo), 
			"SpaceHead[%d] TaskId[%s] UseStatus[%d]\n", 
			i, 
			GetProcSpaceHead(i)->task_run_flag_, 
			GetProcSpaceHead(i)->use_status_
		);
	}

	LOGDEBUG("\n%s", outInfo);
}
