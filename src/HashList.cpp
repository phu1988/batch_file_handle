#include "HashList.h"

HashList::HashList(size_t keyLen, size_t valueLen, MemoryManager* mmOper)
	: key_len_(keyLen)
	, value_len_(valueLen)
	, mm_opr_(mmOper)
	, my_addr_(-1)
{
}

HashList::~HashList()
{

}

void HashList::Initialize(int64 addr)
{		
	/*allocate for recyle&count*/
	my_addr_ = addr;
	while (my_addr_ <= 0) {
		/*wait for free resource*/
		if ((my_addr_ = mm_opr_->Allocate(sizeof(int64) + sizeof(int64))) < 0) {
			usleep(10);
		}
	}

	if (addr <= 0) {
		/*we build memory here*/
		int64 recyle = -1, count = 0;
		IoInterface* io = mm_opr_->GetIoOper();
		io->SetValue(my_addr_, &recyle, sizeof(int64));
		io->SetValue(my_addr_ + sizeof(int64), &count, sizeof(int64));
	}
}

void HashList::SetNext(int64 pre, int64 next)
{
	IoInterface* io = mm_opr_->GetIoOper();
	io->SetValue(pre + key_len_ + value_len_, &next, sizeof(next));
}

int64 HashList::GetNext(int64 data)
{
	IoInterface* io = mm_opr_->GetIoOper();
	return *(io->GetValue<uint64>(data + key_len_ + value_len_));
}

int64 HashList::Recyle(int64 data)
{
	int recyle = GetRecyle();
	if (recyle > 0) {
		SetNext(data, recyle);
	}
	SetRecyle(data);

	int64 nowCount = GetCount();
	SetCount(nowCount - 1);

	return data;
}

int64 HashList::Allocate()
{
	/*first find in recyle*/
	int64 recyle = GetRecyle();
	if (recyle > 0) {
		SetRecyle(GetNext(recyle));
		return recyle;
	}

	int64 addr = -1;
	while (addr < 0) {
		if ((addr = mm_opr_->Allocate(
			key_len_ + value_len_ + sizeof(int64))) < 0) {
			LOGDEBUG("Can not allocate the memory, wait...");
			usleep(10);
		}
	}

	int64 nowCount = GetCount();
	SetCount(nowCount + 1);

	return addr;
}

int64 HashList::GetRecyle()
{
	IoInterface* io = mm_opr_->GetIoOper();
	return *(io->GetValue<int64>(my_addr_));
}

void HashList::SetRecyle(int64 recyle)
{
	IoInterface* io = mm_opr_->GetIoOper();
	io->SetValue(my_addr_, &recyle, sizeof(recyle));
}

int64 HashList::GetCount()
{
	IoInterface* io = mm_opr_->GetIoOper();
	return *(io->GetValue<int64>(my_addr_ + sizeof(int64)));
}

void HashList::SetCount(int64 count)
{
	IoInterface* io = mm_opr_->GetIoOper();
	io->SetValue(my_addr_ + sizeof(int64), &count, sizeof(count));
}

int64 HashList::GetAddress()
{
	return my_addr_;
}
