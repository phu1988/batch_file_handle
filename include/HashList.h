#ifndef HASH_LIST_H_
#define HASH_LIST_H_
#include "MemoryManager.h"
#include <assert.h>

class HashList
{
public:
	HashList(size_t keyLen, size_t valueLen, MemoryManager* mmOper);
	~HashList();

	void Initialize(int64 addr = 0);
	void SetNext(int64 pre, int64 next);
	int64 GetNext(int64 data);
	int64 Recyle(int64 data);
	int64 Allocate();
	int64 GetCount();
	int64 GetAddress();

protected:
	int64 GetRecyle();
	void SetRecyle(int64 recyle);
	void SetCount(int64 count);

protected:
	/*fix:  recyle|count*/
	/*body: key|value|next*/
	/*int64 free_; free is sign in ProcMemoryManager, so list only set the recyle*/
	size_t key_len_;
	size_t value_len_;

	MemoryManager* mm_opr_;
	int64 my_addr_;
};

#endif
