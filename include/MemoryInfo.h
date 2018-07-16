#ifndef MEMORY_INFO_H_
#define MEMORY_INFO_H_
#include <stdio.h>
#include "TypeInfo.h"

/*
head = base
int64 shm_size_;
int64 one_pid_per_size_ = (shm_size_ - sizeof(head) - head->process_count_ * sizeof(process_glob))/head->process_count_;
int64 split_memory_count_one_proc_;
unsigned int process_count_;
*/
struct ShmHead
{
	int64 shm_size_;
	int64 one_pid_per_size_;
	int64 split_memory_count_one_proc_;
	unsigned int process_count_;
};

/*
process_glob(i) = base + sizeof(head) + sizeof(process_glob) * i
pid_t process_pid_;
int64 process_address;
int64 data_len_;
*/
struct ProcGlob
{
	pid_t process_pid_;
	int64 proc_addr_;
	int64 proc_data_len_;
	int memory_split_count_;
	int memory_free_count_;
	int64 proc_space_head_free_entry_;
};

/*
private space(i) = base + sizeof(head) + head->process_count_ * sizeof(process_glob) + one_pid_per_size_ * i
int area_seq_;
char task_run_flag_[64]
int64 memory_size_;
int use_status_;
*/
struct ProcSpaceHead
{
	/*start from 0, limit to memory_split_count_ - 1*/
	int area_seq_;
	char task_run_flag_[128];
	/*0:unuse;1 use; 
	Particularly, the task first seq will be initialize for output rule number*/
	int use_status_;
	int64 start_addr_;
	int64 data_len_;
};

#endif
