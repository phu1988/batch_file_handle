#include "Control.h"
#include "ConfigSet.h"
#include "InputSubProcess.h"

int main(int argc, char** argv)
{
	if (argc != 2) {
		printf("USAGE: Process ConfigFile\n");
		exit(-1);
	}

	if (ConfigSet::GetInstance()->Initialize(argv[1]) < 0) {
		printf("Error Init Config\n");
		exit(-2);
	}

	ConfigSet::GetInstance()->Print();

	if (LogInstance::GetInstance()->Initialize(
		ConfigSet::GetInstance()->Get("TASKINFO", "LOG_PATH"), "TEST") < 0) {
		printf("LOG Initialize Error\n");
		exit(-3);
	}

	if (FileRule::GetInstance()->Load() < 0) {
		printf("Error Load File Rule\n");
		exit(-4);
	}
	if (Control::GetInstance()->Initialize() < 0) {
		printf("Control Initialize Error\n");
		exit(-5);
	}

	TaskInfo info;
	memset(&info, 0, sizeof(info));
	ConfigSet* pConf = ConfigSet::GetInstance();
	//strcpy(info.task_id_, pConf->Get("TASKINFO", "TASK_ID"));
	strcpy(info.task_dir_, pConf->Get("TASKINFO", "TASK_DIR"));
	strcpy(info.input_rule_id_, pConf->Get("TASKINFO", "INPUT_RULE"));
	strcpy(info.output_rule_list_, pConf->Get("TASKINFO", "OUTPUT_RULE"));
	info.task_hash_size_ = 10000;

	int ret = 0;
/*
	for (int i = 0; i < 3; ++i) {
		snprintf(info.task_id_, sizeof(info.task_id_), "%s_%d", 
			pConf->Get("TASKINFO", "TASK_ID"), i);
		while ((ret = Control::GetInstance()->SendToInput(info)) < 0) {
			LOGINFO("There is no input process idle");
			sleep(1);
		}

		LOGINFO("Send To Input[%d] Sucess", ret);
	}

	sleep(10);
	Control::GetInstance()->PrintMemoryInfo(false);

	vector<InputToMain> i2m;
	while (i2m.size() < 3) {
		if (Control::GetInstance()->ScanFromInputProc(i2m) < 0) {
			LOGERR("Error Scan Input");
			break;
		}

		sleep(1);

	}
*/

	vector<InputToMain> i2m;
	for (int i = 0; i < 2; ++i) {
		snprintf(info.task_id_, sizeof(info.task_id_), "%s_%d", pConf->Get("TASKINFO", "TASK_ID"), i);
		while ((ret = Control::GetInstance()->SendToInput(info)) < 0) {
			LOGINFO("There is no input process idle");
			sleep(1);
		}

		sleep(2);
		Control::GetInstance()->PrintMemoryInfo();

		while (Control::GetInstance()->ScanFromInputProc(i2m) <= 0) {
			LOGINFO("There Is No Input Proc Ready");
			sleep(1);
		}
	}

	LOGINFO("Scan From Input:%d", i2m.size());
	Control::GetInstance()->PrintMemoryInfo();

	vector<InputToMain>::iterator it = i2m.begin();
	int outNum = 0;
	for (; it != i2m.end(); ++it) {
		MainToOutput m2o;
		memset(&m2o, 0, sizeof(m2o));
		strcpy(m2o.task_id_, it->task_id_);
		m2o.input_index_ = it->input_index_;
		m2o.data_space_index_ = it->data_space_index_;
		strcpy(m2o.input_rule_id_, info.input_rule_id_);
		m2o.task_hash_size_ = 10000;

		vector<string> outList;
		PubCommon::SplitToVec(info.output_rule_list_, outList, ";");
		vector<string>::iterator outIt = outList.begin();
		for (; outIt != outList.end(); ++outIt) {
			strncpy(m2o.output_rule_id_, outIt->c_str(), sizeof(m2o.output_rule_id_) - 1);
			while ((ret = Control::GetInstance()->SendToOutput(m2o)) < 0) {}
			LOGINFO("TaskId[%s] OutRule[%s] Send To %d", m2o.task_id_, m2o.output_rule_id_, ret);
			++outNum;
		}
	}

	vector<OutputToMain> o2mList;
	ret = 0;
	while (ret < outNum) {
		int num = Control::GetInstance()->ScanFromOutputProc(o2mList);
		if (num < 0) {
			LOGERR("Scan From Out Error");
			return -1;
		}
		ret += num;
		sleep(1);
	}

	LOGINFO("ALLDONE!");
	Control::GetInstance()->End();
	Control::GetInstance()->PrintMemoryInfo();
	
	return 0;
}
