#ifndef HASH_H_
#define HASH_H_
#include "HashArray.h"
#include "HashList.h"

class DefaultHashHelper
{
public:
	static int64 Get(void* key, int keyLen, int64 hashSize)
	{
		unsigned int seed = 131; // 31 131 1313 13131 131313 etc..
		unsigned int crc = 0;

		char* InStr = (char*)key;
		for (int i = 0; i < keyLen; i++) {
			crc = crc * seed + InStr[i];
		}

		crc &= 0x7FFFFFFF;

		return crc % hashSize;
	}
};

/*the value is all int64*/
class DefaultValueCombineHelper
{
public:
	static void Combine(void* source, const void* cmb, size_t valLen)
	{
		for (int i = 0; i < valLen / sizeof(ValType); ++i) {
			ValType a = *reinterpret_cast<ValType*>(
				(char*)source + i * sizeof(ValType));
			ValType b = *reinterpret_cast<const ValType*>(
				(char*)cmb + i * sizeof(ValType));

			a += b;
			*reinterpret_cast<ValType*>((char*)source + i * sizeof(ValType)) = a;
		}
	}
};

class CallBackBase
{
public:
	CallBackBase() {}
	virtual ~CallBackBase() {}
	virtual void Print() = 0;
	virtual void Process(const void* key, const void* value) = 0;

	const char* GetName()
	{
		return name_.c_str();
	}

protected:
	string name_;

};

class HashBase
{
public:
	HashBase()
	{

	}

	virtual ~HashBase()
	{

	}

	virtual void Initialize(int64 addr = 0) = 0;
	virtual int64 Put(void* key, void* value) = 0;
	virtual void Traversal(CallBackBase* callback) = 0;
	virtual int64 GetCount() = 0;
};

template<typename valueCombineHelper = DefaultValueCombineHelper, 
	     typename hashHelper = DefaultHashHelper>
class Hash: public HashBase
{
public:
	Hash(int keyLen, int valueLen, int64 hashSize, MemoryManager* mmOper)
		: key_len_(keyLen)
		, value_len_(valueLen)
		, hash_array_(new HashArray(hashSize, mmOper))
		, hash_list_(new HashList(keyLen, valueLen, mmOper))
		, mm_oper_(mmOper)
	{
	}

	virtual ~Hash()
	{
		if (hash_array_) delete hash_array_;
		if (hash_list_) delete hash_list_; 
	}

	virtual void Initialize(int64 addr = 0)
	{
		/*array must initialize before the list*/
		if (addr <= 0) {
			/*build...*/
			hash_array_->Initialize();
			hash_list_->Initialize();
		} else {
			/*we ensure that the array space and list space is on one free space*/
			hash_array_->Initialize(addr);
			hash_list_->Initialize(
				addr + hash_array_->GetCount() * sizeof(int64));
		}

		LOGDEBUG("HashArray Addr:%ld, List Addr:%ld", 
			hash_array_->GetAddress(), hash_list_->GetAddress());
	}

	virtual int64 Put(void* key, void* value)
	{
		int64 index = hashHelper::Get(key, key_len_, hash_array_->GetCount());
		int64 listEntry = hash_array_->Get(index);

		IoInterface* io = mm_oper_->GetIoOper();
		int64 lEntry = listEntry;
		while (lEntry > 0) {
			char* cmp = io->GetValue<char>(lEntry);
			if (memcmp(cmp, key, key_len_) == 0) {
				valueCombineHelper::Combine(io->GetValue<void>(lEntry + key_len_), 
					value, value_len_);
				return lEntry;
			}

			lEntry = hash_list_->GetNext(lEntry);
		}

		int64 listAddr = hash_list_->Allocate();
		if (listAddr < 0) return -1;

		io->SetValue(listAddr, key, key_len_);
		io->SetValue(listAddr + key_len_, value, value_len_);

		if (listEntry > 0) {
			hash_list_->SetNext(listAddr, listEntry);
		} else {
			hash_list_->SetNext(listAddr, 0);
		}

		hash_array_->Set(index, listAddr);

		return listAddr;
	}

	virtual void Traversal(CallBackBase* callback)
	{
		for (int64 i = 0; i < hash_array_->GetCount(); ++i) {
			int64 listEntry = hash_array_->Get(i);
			if (listEntry <= 0) {
				continue;
			}

			while (listEntry > 0) {
				char* data = mm_oper_->GetIoOper()->GetValue<char>(listEntry);
				if (NULL == data) {
					listEntry = hash_list_->GetNext(listEntry);
					continue;
				}

				callback->Process(data, data + key_len_);
				listEntry = hash_list_->GetNext(listEntry);
			}
		}
	}

	virtual int64 GetCount()
	{
		hash_list_->GetCount();
	}

private:
	int key_len_;
	int value_len_;

	HashArray* hash_array_;
	HashList* hash_list_;
	MemoryManager* mm_oper_;

};

#endif
