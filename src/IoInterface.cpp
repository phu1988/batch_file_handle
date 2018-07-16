#include "IoInterface.h"

IoInterface::IoInterface()
: is_initialize_(0)
{

}

IoInterface::~IoInterface()
{

}

void IoInterface::SetValue(int64 offset, void* value, size_t valueLen)
{
	assert(is_initialize_);
	assert(offset + valueLen <= max_length_);

	if (value == NULL) {
		memset(base_address_ + offset, 0, valueLen);
	} else {
		memcpy(base_address_ + offset, value, valueLen);
	}
}

int64 IoInterface::GetMaxLength()
{
	assert(is_initialize_);
	return max_length_;
}
