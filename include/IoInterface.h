#ifndef IO_INTERFACE_H_
#define IO_INTERFACE_H_
#include <assert.h>
#include <memory.h>
#include <stdio.h>
#include "TypeInfo.h"

enum IO_TYPE {
	IO_TYPE_SHM = 0,
	IO_TYPE_MEMORY,
	IO_TYPE_FILEMAPPING
};

class IoInterface
{
public:
	IoInterface();
	virtual ~IoInterface();
	virtual int Initialize() = 0;
	virtual int UnInitialize() = 0;

	void SetValue(int64 offset, void* value, size_t valueLen);
	inline int64 GetMaxLength();
	template<typename Type>
	inline Type* GetValue(int64 offset)
	{
		assert(is_initialize_);
		/*assert(offset + sizeof(Type) < max_length_);*/

		return reinterpret_cast<Type*>(base_address_ + offset);
	}

protected:
	char* base_address_;
	int64 max_length_;
	IO_TYPE io_type_;
	volatile bool is_initialize_;

};

#endif
