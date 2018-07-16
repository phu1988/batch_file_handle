#include "IoShmMemory.h"

IoShmMemory::IoShmMemory(key_t shmKey, uint64 len)
	: shm_key_(shmKey)
	, shm_id_ (-1)
	, owner_pid_(-1)
{
	max_length_ = len;
	io_type_ = IO_TYPE_SHM;
}

IoShmMemory::~IoShmMemory()
{
	if (is_initialize_) UnInitialize();
	if (getpid() == owner_pid_) {
		shmctl(shm_id_, IPC_RMID, NULL);
		LOGINFO("Remove shmId[%d]", shm_id_);
	}
}

int IoShmMemory::Initialize()
{
	if (!__sync_bool_compare_and_swap(&is_initialize_, 0, 1)) return 1;

	shm_id_ = shmget(shm_key_, max_length_, 0666 | IPC_CREAT);
	if (shm_id_ <= 0) {
		LOGERR("Create Shm Memory Error[%d]: %s", errno, strerror(errno));
		return -1;
	}

	base_address_ = (char*)shmat(shm_id_, NULL, 0);
	if ((void*)base_address_ == (void*)-1) {
		LOGERR("Shmat Memory Error[%d]:%s", errno, strerror(errno));
		return -1;
	}

	/*set 0*/
	SetValue(0, NULL, max_length_);

	owner_pid_ = getpid();

	return 0;
}

int IoShmMemory::UnInitialize()
{
	if (!__sync_bool_compare_and_swap(&is_initialize_, 1, 0)) return 1;
	return shmdt(base_address_);
}
