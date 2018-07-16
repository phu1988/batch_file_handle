#ifndef MEMORY_MANAGER_H_
#define MEMORY_MANAGER_H_
#include "IoInterface.h"

class MemoryManager
{
public:
	MemoryManager(IoInterface* io);
	virtual ~MemoryManager();
	IoInterface* GetIoOper();
	virtual int64 Allocate(int64 mSize) = 0;

protected:
	IoInterface* io_oper_;

};

#endif
