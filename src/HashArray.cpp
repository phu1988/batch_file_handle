#include "HashArray.h"

HashArray::HashArray(int64 hashCount, MemoryManager* mOper)
	: hash_count_(hashCount)
	, mm_oper_(mOper)
	, my_addr_(-1)
{
}

HashArray::~HashArray()
{

}

void HashArray::Initialize(int64 addr)
{
	my_addr_ = addr;
	while (my_addr_ <= 0) {
		if ((my_addr_ = mm_oper_->Allocate(hash_count_ * sizeof(int64))) < 0) {
			/*wait for free resource*/
			usleep(10);
		}
	}
}

void HashArray::Set(int64 index, int64 listAddress)
{
	assert(index < hash_count_);

	mm_oper_->GetIoOper()->SetValue(my_addr_ + index * sizeof(int64), 
		&listAddress, sizeof(int64));
}

int64 HashArray::Get(int64 index)
{
	assert(index < hash_count_);

	return *mm_oper_->GetIoOper()->GetValue<int64>(
		my_addr_ + index * sizeof(int64));
}

int64 HashArray::GetCount()
{
	return hash_count_;
}

int64 HashArray::GetAddress()
{
	return my_addr_;
}
