#include <dirent.h>
#include "InputSubProcess.h"
#include "PubCommon.h"
#include "TaskInfo.h"
#include "FileModule.h"

InputSubProcess::InputSubProcess(int procIndex, int fdR, int fdW, IoInterface* io)
	: proc_index_(procIndex)
	, fd_read_(fdR)
	, fd_write_(fdW)
{
	proc_memory_man_ = new ProcMemoryManager(io, proc_index_);
}

InputSubProcess::~InputSubProcess()
{
	delete proc_memory_man_;
}

int InputSubProcess::Process()
{
	while (true) {
		PubCommon::FdCanRead(fd_read_, -1);

		MainToInput m2i;
		if (PubCommon::Read(fd_read_, &m2i, sizeof(m2i)) <= 0) {
			LOGERR("pid[%d] read error, exit", getpid());
			break;
		}
		if (0 == strcmp(m2i.task_id_, "END")) break;

		proc_memory_man_->AttachNewTask(m2i.task_id_);

		/*Do File*/
		DealFiles(m2i);

		/*Set The First Space Status*/
		vector<string> vOutRule;
		PubCommon::SplitToVec(m2i.output_rule_list_, vOutRule, ";");

		/*set the use status of first space*/
		proc_memory_man_->GetProcSpaceHead(
			proc_memory_man_->GetTaskSpaceEntrySeq())->use_status_ = vOutRule.size();

		/*Write to main process*/
		InputToMain i2m;
		strncpy(i2m.task_id_, m2i.task_id_, sizeof(i2m.task_id_) - 1);
		i2m.input_index_ = proc_index_;
		i2m.data_space_index_ = proc_memory_man_->GetTaskSpaceEntrySeq();
		i2m.status = YES;

		PubCommon::Write(fd_write_, &i2m, sizeof(i2m));
	}

	return 0;
}

int InputSubProcess::DealFiles(const MainToInput& m2i)
{
	FileInputRule* inRule = FileRule::GetInstance()->GetInputRule(
		m2i.input_rule_id_);

	if (NULL == inRule) {
		LOGERR("Get File In Rule[%s] Error", m2i.input_rule_id_);
		return -1;
	}

	vector<string> files;
	if (GetFiles(m2i.task_dir_, inRule->deal_file_pre_flag_, files) <= 0) {
		LOGWARN("Dir:%s no file to deal", m2i.task_dir_);
		return -1;
	}

	LOGINFO("GetFiles:%d", files.size());

	size_t keyLen = inRule->key_length_;
	size_t valLen = inRule->val_length_;

	HashBase* pHash = new Hash<>(
		keyLen, valLen, m2i.task_hash_size_, proc_memory_man_);
	pHash->Initialize();

	char buffer[4096] = {0};
	char* keyBuffer = new char[keyLen];
	char* valBuffer = new char[valLen];
	for (vector<string>::iterator it = files.begin(); it != files.end(); ++it) {
		LOGDEBUG("Deal File:%s", it->c_str());
		FILE* fp = fopen(it->c_str(), "r");
		if (NULL == fp) {
			LOGWARN("File:%s not exists", it->c_str());
			continue;
		}

		void* head = NULL;
		void* tail = NULL;
		bool isTail = false;
		DealFileBase* pDealFile = DealFileFactory::GetInstance(
			)->GetDealFileHandle(inRule->deal_file_flag_);

		pDealFile->DealHead(fp, head);
		LOGDEBUG("Deal Head For: %s  Sucess", it->c_str());
		int fileLineNum = 0;
		while (pDealFile->DealBody(fp, buffer, sizeof(buffer), isTail) != NULL) {
			if (isTail) {
				pDealFile->DealTail(buffer, tail);
			} else {
				/*parse the file line*/
				TransToRecord(buffer, inRule, keyBuffer, keyLen, valBuffer, valLen);
				/*put into hash*/

				/*LOGDEBUG("key:%s", keyBuffer);*/
				pHash->Put(keyBuffer, valBuffer);
				if (++fileLineNum % 100000 == 0) {
					LOGDEBUG("Deal File Line:%d, now hash count = %ld", 
						fileLineNum, pHash->GetCount());
				}
			}
		}

		fclose(fp);

		LOGINFO("Deal File:%s End, total line:%d, hash count = %ld", 
			it->c_str(), fileLineNum, pHash->GetCount());
	}

	delete[] keyBuffer;
	delete[] valBuffer;
	delete pHash;
}

void InputSubProcess::TransToRecord(const char* buffer, 
									const FileInputRule* inRule, 
									char* keyBuffer, 
									int keyLen, 
									char* valBuffer,
									int valLen)
{
	memset(keyBuffer, 0, keyLen);
	memset(valBuffer, 0, valLen);
	char valTmp[32 + 1] = {0};
	if (0 == strlen(inRule->text_split_)) {
		int pos = 0;
		for (int i = 0; i < inRule->key_rule_.size(); ++i) {
			memcpy(keyBuffer + pos, buffer + inRule->key_rule_[i].start_, 
				inRule->key_rule_[i].length_);
			pos += inRule->key_rule_[i].length_;
		}

		for (int i = 0; i < inRule->value_rule_.size(); ++i) {
			strncpy(valTmp, buffer + inRule->value_rule_[i].start_, 
				inRule->value_rule_[i].length_);
			PubCommon::ReplaceStr(valTmp, " ", "", sizeof(valTmp));
			ValType tmp = CharToVal(valTmp);
			memcpy(valBuffer + i * sizeof(ValType), &tmp, sizeof(tmp));
		}
	} else {
		vector<string> splData;
		PubCommon::SplitToVec(buffer, splData, inRule->text_split_);
		int pos = 0;
		for (int i = 0; i < inRule->key_rule_.size(); ++i) {
			strncpy(keyBuffer + pos, 
				splData[inRule->key_rule_[i].start_].c_str(), 
				inRule->key_rule_[i].length_ - 1);
			pos += inRule->key_rule_[i].length_;
		}

		for (int i = 0; i < inRule->value_rule_.size(); ++i) {
			ValType tmp = CharToVal(splData[i].c_str());
			memcpy(valBuffer + i * sizeof(ValType), &tmp, sizeof(tmp));
		}
	}
}

int InputSubProcess::GetFiles(const char* dir, 
							  const char* preFlag, 
							  vector<string>& files)
{
	DIR* pDir = opendir(dir);
	if (pDir == NULL) {
		LOGERR("[%d]Can not open dir:%s", getpid(), dir);
		return -1;
	}

	dirent* dirp = NULL;
	while ((dirp = readdir(pDir)) != NULL) {
		if (0 == strcmp(".", dirp->d_name) || 0 == strcmp("..", dirp->d_name)) {
			continue;
		}

		char aFile[256 + 1] = {0};
		snprintf(aFile, sizeof(aFile), "%s/%s", dir, dirp->d_name);

		files.push_back(DealFilePreFactory::GetInstance()->
			GetDealFilePreHandle(preFlag)->Deal(aFile));
	}

	closedir(pDir);

	return files.size();
}
