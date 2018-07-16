#ifndef GLOBAL_OBJECT_HELPER_H_
#define GLOBAL_OBJECT_HELPER_H_
#include <memory>
template<typename Type>
class GlobalObjectHelper
{
public:
	virtual ~GlobalObjectHelper(){}

	static Type* GetInstance()
	{
		if (NULL == instance_.get())
		{
#if __cplusplus < 201103L
			instance_ = std::auto_ptr<Type>(new Type());
#else
			instance_ = std::shared_ptr<Type>(new Type());
#endif
		}

		return instance_.get();
	}

protected:
	GlobalObjectHelper(){}

private:
#if __cplusplus < 201103L
	static std::auto_ptr<Type> instance_;
#else
	static std::shared_ptr<Type> instance_;
#endif

};

template<typename Type>
#if __cplusplus < 201103L
std::auto_ptr<Type> GlobalObjectHelper<Type>::instance_;
#else
std::shared_ptr<Type> GlobalObjectHelper<Type>::instance_;
#endif

#endif // !GLOBAL_OBJECT_HELPER_H_
