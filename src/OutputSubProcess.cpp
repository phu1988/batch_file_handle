#include "OutputSubProcess.h"
#include "PubCommon.h"

OutputSubProcess::OutputSubProcess(int index, int fdR, int fdW, IoInterface* io)
	: proc_index_(index)
	, fd_read_(fdR)
	, fd_write_(fdW)
	, io_oper_(io)
{

}

OutputSubProcess::~OutputSubProcess()
{

}

int OutputSubProcess::Process()
{
	while (true) {
		PubCommon::FdCanRead(fd_read_, -1);
		MainToOutput m2o;
		if (PubCommon::Read(fd_read_, &m2o, sizeof(m2o)) <= 0) {
			LOGERR("out pid[%d] read error, exit", getpid());
			break;
		}
		if (0 == strcmp("END", m2o.task_id_)) break;

		LOGINFO("TaskId[%s] InputIdex[%d] DataSpaceIndex[%d] OutputRule[%s] Begin", 
			m2o.task_id_, m2o.input_index_, m2o.data_space_index_, m2o.output_rule_id_);

		ProcMemoryManager* procMM = new ProcMemoryManager(
			io_oper_, m2o.input_index_);

		/*deal*/
		FileInputRule* inRule = FileRule::GetInstance()->GetInputRule(
			m2o.input_rule_id_);
		FileOutputRule* outRule = FileRule::GetInstance()->GetOutputRule(
			m2o.output_rule_id_);

		HashBase* pHash = new  Hash<>(inRule->key_length_, 
			inRule->val_length_, m2o.task_hash_size_, procMM);
		ProcSpaceHead* taskHeadEntry = procMM->GetProcSpaceHead(m2o.data_space_index_);
		pHash->Initialize(taskHeadEntry->start_addr_);
		/*may not be default, we reserve the interface*/
		CallBackBase* callback = new CallBackDefault(inRule, outRule);
		pHash->Traversal(callback);
		callback->Print();
		delete callback;
		delete pHash;

		OutputToMain o2m;
		strncpy(o2m.task_id_, m2o.task_id_, sizeof(o2m.task_id_) - 1);
		o2m.status = YES;

		/*update memory*/
		if (--taskHeadEntry->use_status_ > 0) {
			LOGINFO("TaskId[%s] remain num = %d", m2o.task_id_, taskHeadEntry->use_status_);
			/*write to main process*/
			PubCommon::Write(fd_write_, &o2m, sizeof(o2m));
			continue;
		}

		/*output is done, then update the task use status, and set the free count*/
		LOGINFO("TaskId[%s] update use status", m2o.task_id_);
		ProcSpaceHead* procSpaceEntry = procMM->GetProcSpaceHeadEntry();
		for (int i = 0; i < procMM->GetProcGlob()->memory_split_count_; ++i) {
			ProcSpaceHead* head = procMM->GetProcSpaceHead(i);
			if (0 == strcmp(head->task_run_flag_, m2o.task_id_)) {
					memset(head->task_run_flag_, 0, sizeof(head->task_run_flag_));
					head->use_status_ = 0;
					/*increatment the space free count*/
					++(procMM->GetProcGlob()->memory_free_count_);
			}
		}

		/*write to main process*/
		PubCommon::Write(fd_write_, &o2m, sizeof(o2m));
	}

	return 0;
}

CallBackDefault::CallBackDefault(FileInputRule* inRule, FileOutputRule* outRule)
	: out_rule_(outRule)
{
	data_.clear();
	in_key_pos_.clear();

	int nowStart = 0;
	for (int i = 0; i < inRule->key_rule_.size(); ++i) {
		InputRulePos pos;
		pos.start_ = nowStart;
		pos.len_ = inRule->key_rule_[i].length_;
		in_key_pos_.push_back(pos);

		nowStart += pos.len_;
	}

	name_ = "default";
}

CallBackDefault::~CallBackDefault()
{
	map<string, void*>::iterator it = data_.begin();
	for (; it != data_.end(); ++it) {
		free(it->second);
	}
}

void CallBackDefault::Print()
{
	map<string, void*>::iterator it = data_.begin();
	for (; it != data_.end(); ++it) {
		printf(it->first.c_str());
		for (int i = 0; i < out_rule_->val_in_.size(); ++i) {
			printf("%.2f%s", *reinterpret_cast<ValType*>(
				(char*)it->second + i * sizeof(ValType)), 
				out_rule_->out_text_split_);
		}

		printf("\n");
	}
}

void CallBackDefault::Process(const void* key, const void* value)
{
	/*GetKey*/
	char tmpBuffer[512 + 1] = {0}, tmpElm[128 + 1] = {0};
	vector<int>::iterator it = out_rule_->key_in_.begin();
	for (; it != out_rule_->key_in_.end(); ++it) {
		memset(tmpElm, 0, sizeof(tmpElm));
		strncpy(tmpElm, (char*)key + in_key_pos_[*it].start_, 
			in_key_pos_[*it].len_);
		PubCommon::TrimChr(tmpElm, sizeof(tmpElm));

		strncat(tmpBuffer, tmpElm, sizeof(tmpBuffer) - 1);
		strcat(tmpBuffer, out_rule_->out_text_split_);
	}

	/*GetValue, find record from data*/
	map<string, void*>::iterator itFind = data_.find(tmpBuffer);
	if (itFind != data_.end()) {
		ValType* posData = reinterpret_cast<ValType*>(itFind->second);
		it = out_rule_->val_in_.begin();
		for (; it != out_rule_->val_in_.end(); ++it) {
			*posData += *reinterpret_cast<ValType*>(
				(char*)value + (*it) * sizeof(ValType));

			posData++;
		}

		return;
	}

	/*not find record, add it*/
	void* valData = (void*)malloc(out_rule_->val_in_.size() * sizeof(int64));
	memset(valData, 0, out_rule_->val_in_.size() * sizeof(int64));
	it = out_rule_->val_in_.begin();
	int pos = 0;
	for (; it != out_rule_->val_in_.end(); ++it) {
		memcpy((char*)valData + pos * sizeof(ValType), 
			(char*)value + (*it) * sizeof(ValType), sizeof(ValType));
	}

	data_.insert(make_pair(tmpBuffer, valData));
}
