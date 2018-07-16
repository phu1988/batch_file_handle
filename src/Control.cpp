#include "Control.h"
#include <sys/select.h>
#include <sys/time.h>
#include <sys/wait.h>
#include "PubCommon.h"
#include "InputSubProcess.h"
#include "OutputSubProcess.h"

Control::Control()
	: io_oper_(NULL)
{
}

Control::~Control()
{
	map<pid_t, ProcMemoryManager*>::iterator it = proc_memory_manager_.begin();
	for (; it != proc_memory_manager_.end(); ++it) {
		delete (it->second);
	}

	if (NULL != io_oper_) delete io_oper_;

	proc_memory_manager_.clear();
}

int Control::Initialize()
{
	io_oper_ = new IoShmMemory(MY_SHM_KEY, MY_SHM_LEN);
	if (io_oper_->Initialize() < 0) {
		LOGERR("Control Initialize Error");
		return -1;
	}

	ShmHead* head = io_oper_->GetValue<ShmHead>(0);
	head->shm_size_ = MY_SHM_LEN;
	head->process_count_ = MY_INPUT_PROCESS_NUM;
	head->split_memory_count_one_proc_ = MY_SPLIT_ONE_PROC;
	head->one_pid_per_size_ = (MY_SHM_LEN - sizeof(head) - 
		MY_INPUT_PROCESS_NUM * sizeof(ProcGlob)) / MY_INPUT_PROCESS_NUM;

	if (CreateInProcAndInit() < 0) {
		return -1;
	}

	if (CreateOutProcAndInit() < 0) {
		return -1;
	}
}

int Control::CreateInProcAndInit()
{
	for (int i = 0; i < MY_INPUT_PROCESS_NUM; ++i) {
		int fd_c_r[2], fd_p_r[2];
		pipe(fd_c_r);
		pipe(fd_p_r);
		pid_t pid = fork();
		if (pid < 0) {
			LOGERR("Create Proc[%d] Error", i);
			return -3;
		}

		if (0 == pid) {
			/*child*/
			close(fd_c_r[FDW]);
			close(fd_p_r[FDR]);

			InputSubProcess* input = new InputSubProcess(
				i, fd_c_r[FDR], fd_p_r[FDW], io_oper_);
			input->Process();
			io_oper_->UnInitialize();
			delete input;

			exit(0);
		}

		/*parent*/
		close(fd_c_r[FDR]);
		close(fd_p_r[FDW]);
		_pipe_read_write pipeRw;
		pipeRw.seq = i;
		pipeRw.status = IDLE;
		pipeRw.read_ = fd_p_r[FDR];
		pipeRw.write_ = fd_c_r[FDW];
		pid_in_pipe_.insert(pair<pid_t, _pipe_read_write>(pid, pipeRw));

		SetProcGlobMemory(pid, i);
		SetProcSpaceHeadMemory(pid, i);
	}

	return 0;
}

void Control::SetProcGlobMemory(pid_t pid, int index)
{
	ProcMemoryManager* procMM = new ProcMemoryManager(io_oper_, index);
	proc_memory_manager_.insert(make_pair(pid, procMM));

	/*ProcGlob*/
	ProcGlob* procGlob = procMM->GetProcGlob();
	procGlob->process_pid_ = pid;
	procGlob->proc_addr_ = sizeof(ShmHead) 
		+ sizeof(ProcGlob) * MY_INPUT_PROCESS_NUM 
		+ index * procMM->GetHead()->one_pid_per_size_;
	procGlob->memory_split_count_ = MY_SPLIT_ONE_PROC;
	procGlob->memory_free_count_ = MY_SPLIT_ONE_PROC;
	if (index < MY_INPUT_PROCESS_NUM - 1) {
		procGlob->proc_data_len_ = procMM->GetHead()->one_pid_per_size_;
	} else {
		procGlob->proc_data_len_ = MY_SHM_LEN - sizeof(ShmHead) 
			- MY_INPUT_PROCESS_NUM * sizeof(ProcGlob) 
			- (MY_INPUT_PROCESS_NUM - 1) * procMM->GetHead()->one_pid_per_size_;
	}
	procGlob->proc_space_head_free_entry_ = procGlob->proc_addr_;
}

void Control::SetProcSpaceHeadMemory(pid_t pid, int index)
{
	/*procSpaceHead*/
	ProcMemoryManager* procMM = proc_memory_manager_.find(pid)->second;
	ProcGlob* procGlob = procMM->GetProcGlob();

	int64 procDataLen = procGlob->proc_data_len_;
	int splitCount = procGlob->memory_split_count_;
	int headSize = sizeof(ProcSpaceHead) * splitCount;
	int64 oneSplitLen = (procDataLen - headSize) / splitCount;
	for (int hs = 0; hs < splitCount; ++hs) {
		ProcSpaceHead* head = procMM->GetProcSpaceHead(hs);
		head->area_seq_ = hs;
		memset(head->task_run_flag_, 0, sizeof(head->task_run_flag_));
		head->use_status_ = 0;
		if (hs < splitCount - 1) {
			head->data_len_ = oneSplitLen;
		} else {
			head->data_len_ = procDataLen - headSize 
				- (splitCount - 1) * oneSplitLen;
		}

		head->start_addr_ = procGlob->proc_addr_ + headSize + oneSplitLen * hs;
	}
}

int Control::CreateOutProcAndInit()
{
	for (int i = 0; i < MY_OUTPUT_PROCESS_NUM; ++i) {
		int fd_c_r[2], fd_p_r[2];
		pipe(fd_c_r);
		pipe(fd_p_r);
		pid_t pid = fork();
		if (pid < 0) {
			LOGERR("Create Proc[%d] Error", i);
			return -3;
		}

		if (0 == pid) {
			close(fd_c_r[FDW]);
			close(fd_p_r[FDR]);

			OutputSubProcess* output = new OutputSubProcess(
				i, fd_c_r[FDR], fd_p_r[FDW], io_oper_);
			output->Process();
			io_oper_->UnInitialize();
			delete output;
			exit(0);
		}

		close(fd_c_r[FDR]);
		close(fd_p_r[FDW]);
		_pipe_read_write pipeRw;
		pipeRw.seq = i;
		pipeRw.status = IDLE;
		pipeRw.read_ = fd_p_r[FDR];
		pipeRw.write_ = fd_c_r[FDW];
		pid_out_pipe_.insert(pair<pid_t, _pipe_read_write>(pid, pipeRw));
	}

	return 0;
}

int Control::SendToInput(const MainToInput& m2i)
{
	/*find idle input and write*/
	int maxFreeCount = 0, pid = -1;
	map<pid_t, _pipe_read_write>::iterator it = pid_in_pipe_.begin();
	for (; it != pid_in_pipe_.end(); ++it) {
		/*find the idle and find the max memory_free_count_ pid*/
		if (it->second.status == IDLE) {
			int pidFreeCount = proc_memory_manager_.find(
				it->first)->second->GetProcGlob()->memory_free_count_;
			if (maxFreeCount < pidFreeCount) {
				maxFreeCount = pidFreeCount;
				pid = it->first;
			}
		}
	}

	if (pid > 0) {
		it = pid_in_pipe_.find(pid);
		PubCommon::Write(it->second.write_, &m2i, sizeof(m2i));
		it->second.status = BUSY;
		return pid;
		
	}

	/*return < 0 means that have not idle input Process*/
	return -1;
}

int Control::ScanFromInputProc(vector<InputToMain>& i2ms)
{
	int maxfd = 0, nNum = 0;
	fd_set readSet, brkSet;
	FD_ZERO(&readSet);
	FD_ZERO(&brkSet);

	map<pid_t, _pipe_read_write>::iterator it = pid_in_pipe_.begin();
	for (; it != pid_in_pipe_.end(); ++it) {
		if (maxfd < it->second.read_) maxfd = it->second.read_;
		if (maxfd < it->second.write_) maxfd = it->second.write_;

		if (it->second.status == BUSY) {
			FD_SET(it->second.read_, &readSet);
		}

		if (it->second .status != BROKEN) {
			FD_SET(it->second.read_, &brkSet);
			FD_SET(it->second.write_, &brkSet);
		}
	}
	maxfd++;

	timeval tVal = {0, 0};
	/*no block*/
	if (select(maxfd, &readSet, NULL, &brkSet, &tVal) <= 0) {
		return 0;
	}

	for (it = pid_in_pipe_.begin(); it != pid_in_pipe_.end(); ++it) {
		if (FD_ISSET(it->second.read_, &readSet)) {
			InputToMain rec;
			if (PubCommon::Read(it->second.read_, &rec, sizeof(rec)) <= 0) {
				LOGERR("Read Pid[%d] Error[%d]: %s", 
					it->first, errno, strerror(errno));
				return -1;
			}
			
			LOGINFO("Get From input pid[%d], task id[%s]", it->first, rec.task_id_);
			i2ms.push_back(rec);
			++nNum;

			/*set pid idle*/
			it->second.status = IDLE;
		} else if (FD_ISSET(it->second.read_, &brkSet) 
			|| FD_ISSET(it->second.write_, &brkSet)) {
			LOGERR("pid[%d] pipe is broken", it->first);
			it->second.status = BROKEN;
		}
	}

	return nNum;
}

int Control::SendToOutput(const MainToOutput& m2o)
{
	map<pid_t, _pipe_read_write>::iterator it = pid_out_pipe_.begin();
	for (; it != pid_out_pipe_.end(); ++it) {
		if (it->second.status == IDLE) {
			PubCommon::Write(it->second.write_, &m2o, sizeof(m2o));
			it->second.status = BUSY;
			return it->first;
		}
	}

	return -1;
}

int Control::ScanFromOutputProc(vector<OutputToMain>& o2ms)
{
	int maxfd = 0, retNum = 0;
	fd_set readSet, brkSet;
	FD_ZERO(&readSet);
	FD_ZERO(&brkSet);

	map<pid_t, _pipe_read_write>::iterator it = pid_out_pipe_.begin();
	for (; it != pid_out_pipe_.end(); ++it) {
		if (maxfd < it->second.read_) maxfd = it->second.read_;
		if (maxfd < it->second.write_) maxfd = it->second.write_;

		if (it->second.status == BUSY) {
			FD_SET(it->second.read_, &readSet);
		}

		if (it->second.status != BROKEN) {
			FD_SET(it->second.read_, &brkSet);
			FD_SET(it->second.write_, &brkSet);
		}
	}
	maxfd++;

	timeval tVal = {0, 0};
	if (select(maxfd, &readSet, NULL, &brkSet, &tVal) <= 0) {
		return 0;
	}

	for (it = pid_out_pipe_.begin(); it != pid_out_pipe_.end(); ++it) {
		if (FD_ISSET(it->second.read_, &readSet)) {
			OutputToMain tMain;
			if (PubCommon::Read(it->second.read_, &tMain, sizeof(tMain)) <= 0) {
				LOGERR("Read Pid[%d] Error[%d]: %s", 
					it->first, errno, strerror(errno));
				return -1;
			}

			LOGINFO("Get from output pid[%d], task id = %s", it->first, tMain.task_id_);
			o2ms.push_back(tMain);
			++retNum;

			it->second.status = IDLE;
		} else if (FD_ISSET(it->second.read_, &brkSet) 
			|| FD_ISSET(it->second.write_, &brkSet)) {
				LOGERR("pid[%d] pipe is broken", it->first);
				it->second.status = BROKEN;
		}
	}

	return retNum;
}

void Control::End()
{
	map<pid_t, _pipe_read_write>::iterator it = pid_in_pipe_.begin();
	for (; it != pid_in_pipe_.end(); ++it) {
		MainToInput m2i;
		memset(&m2i, 0, sizeof(m2i));
		strncpy(m2i.task_id_, "END", sizeof(m2i.task_id_) - 1);
		SendToInput(m2i);
	}

	it = pid_out_pipe_.begin();
	for (; it != pid_out_pipe_.end(); ++it) {
		MainToOutput m2o;
		memset(&m2o, 0, sizeof(m2o));
		strncpy(m2o.task_id_, "END", sizeof(m2o.task_id_) - 1);
		SendToOutput(m2o);
	}

	/*wait*/
	int status;
	pid_t pid;
	int eNum = 0;
	while (eNum < MY_INPUT_PROCESS_NUM + MY_OUTPUT_PROCESS_NUM) {
		pid = waitpid(-1, &status, WNOHANG);
		if (pid > 0) {
			LOGINFO("process[%d] quit", pid);
			++eNum;
		}
	}
}

void Control::PrintMemoryInfo(bool isAll)
{
	map<pid_t, ProcMemoryManager*>::iterator mIt = proc_memory_manager_.begin();
	for (; mIt != proc_memory_manager_.end(); ++mIt) {
		/*print head*/
		if (mIt == proc_memory_manager_.begin()) {
			LOGDEBUG("=========HEAD: SIZE[%ld] PROCESS NUM[%d] ONEPROCSIZE[%ld] PROCSPLITCOUNT[%d]===========", 
				mIt->second->GetHead()->shm_size_, 
				mIt->second->GetHead()->process_count_, 
				mIt->second->GetHead()->one_pid_per_size_, 
				mIt->second->GetHead()->split_memory_count_one_proc_
			);
		}

		/*print proc glob*/
		LOGDEBUG("=========PID[%d] PROCADDRESS[%ld] PROCDATALEN[%ld] MEMORYSPLITCOUNT[%ld] MEMORYFREECOUNT[%ld] FREEENTRY[%ld]========", 
			mIt->second->GetProcGlob()->process_pid_, 
			mIt->second->GetProcGlob()->proc_addr_, 
			mIt->second->GetProcGlob()->proc_data_len_, 
			mIt->second->GetProcGlob()->memory_split_count_, 
			mIt->second->GetProcGlob()->memory_free_count_, 
			mIt->second->GetProcGlob()->proc_space_head_free_entry_
		);

		if (!isAll && pid_in_pipe_.find(mIt->first)->second.status != BUSY) {
			continue;
		}

		/*print proc space head*/
		mIt->second->Print();
	}
}
