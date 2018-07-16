#include "MemoryManager.h"

MemoryManager::MemoryManager(IoInterface* io)
	: io_oper_(io)
{

}

MemoryManager::~MemoryManager()
{

}

IoInterface* MemoryManager::GetIoOper()
{
	return io_oper_;
}