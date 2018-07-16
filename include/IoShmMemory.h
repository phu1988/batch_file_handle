#ifndef IO_SHM_MEMORY_H_
#define IO_SHM_MEMORY_H_
#include <sys/types.h>
#include <sys/shm.h>
#include "IoInterface.h"

class IoShmMemory: public IoInterface
{
public:
	IoShmMemory(key_t shmKey, uint64 len);
	virtual ~IoShmMemory();
	virtual int Initialize();
	virtual int UnInitialize();

private:
	key_t shm_key_;
	int shm_id_;
	pid_t owner_pid_;

};

#endif // !IO_SHM_MEMORY_H_
