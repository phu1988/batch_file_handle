#ifndef HASH_ARRAY_H_
#define HASH_ARRAY_H_
#include "TypeInfo.h"
#include <assert.h>
#include "MemoryManager.h"

class HashArray
{
public:
	HashArray(int64 hashCount, MemoryManager* mOper);
	~HashArray();
	/*if addr equal, we will build the memory
	  else, we just attah the exists memory*/
	void Initialize(int64 addr = 0);

	/*index from 0 to hashCount*/
	void Set(int64 index, int64 listAddress);
	/*index from 0 to hashCount*/
	int64 Get(int64 index);

	int64 GetCount();
	int64 GetAddress();

private:
	int64 hash_count_;
	MemoryManager* mm_oper_;

	int64 my_addr_;
};

#endif
