#ifndef FILE_MODULE_H_
#define FILE_MODULE_H_
#include <vector>
#include <map>
#include <stdio.h>
#include<string.h>
#include "GlobalObjectHelper.h"

class DealFilePreBase
{
public:
	DealFilePreBase()
	{
		class_name_ = "";
	}

	virtual ~DealFilePreBase()
	{

	}

	virtual char* Deal(char* fileName) = 0;

	const char* GetClassName()
	{
		return class_name_.c_str();
	}

protected:
	std::string class_name_;
};

class DealFilePreDefault: public DealFilePreBase, 
	public GlobalObjectHelper<DealFilePreDefault>
{
public:
	~DealFilePreDefault()
	{

	}

	virtual char* Deal(char* fileName)
	{
		return fileName;
	}

private:
	friend class GlobalObjectHelper<DealFilePreDefault>;
	DealFilePreDefault()
	{
		class_name_ = "default";
	}
};

class DealFilePreFactory: public GlobalObjectHelper<DealFilePreFactory>
{
public:
	DealFilePreBase* GetDealFilePreHandle(const char* className = "default")
	{
		std::map<std::string, DealFilePreBase*>::iterator it 
			= factory_.find(NULL == className? "default": className);


		if (it == factory_.end()) {
			LOGWARN("Can not find DealFilePreHandle:%s, Trans To default");
			it = factory_.find("default");
		}


		assert(it != factory_.end());

		return it->second;
	}

private:
	friend class GlobalObjectHelper<DealFilePreFactory>;
	DealFilePreFactory()
	{
		factory_.insert(make_pair(
			DealFilePreDefault::GetInstance()->GetClassName(), 
			DealFilePreDefault::GetInstance()));

		/*here to add more dealfilepre class instance*/
	}

private:
	std::map<std::string, DealFilePreBase*> factory_;

};

class DealFileBase
{
public:
	DealFileBase()
	{
		class_name_ = "error";
	}

	virtual ~DealFileBase()
	{

	}

	virtual int DealHead(FILE* fp, void* headInfo) = 0;
	virtual char* DealBody(FILE* fp, char* line, int maxLenght, bool& isTail) = 0;
	virtual int DealTail(char* line, void* tailInfo) = 0;

	const char* GetClassName()
	{
		return class_name_.c_str();
	}

protected:
	std::string class_name_;
};

class DealFileDefault: public DealFileBase, 
	public GlobalObjectHelper<DealFileDefault>
{
public:
	~DealFileDefault()
	{

	}

	virtual int DealHead(FILE* fp, void* headInfo)
	{
		/*have no head*/
		return 0;
	}

	virtual char* DealBody(FILE* fp, char* line, int maxLenght, bool& isTail)
	{
		/*have no tail*/
		isTail = false;
		if (fgets(line, maxLenght, fp) != NULL) {
			return line;
		} else {
			return NULL;
		}
	}

	virtual int DealTail(char* line, void* tailInfo)
	{
		/*no tail*/
		return 0;
	}

private:
	friend class GlobalObjectHelper<DealFileDefault>;
	DealFileDefault()
	{
		class_name_ =  "default";
	}
};

class DealFileFactory: public GlobalObjectHelper<DealFileFactory>
{
public:
	DealFileBase* GetDealFileHandle(const char* name = "default")
	{
		if (NULL == name) {
			return factory_.find("default")->second;
		}

		std::map<std::string, DealFileBase*>::iterator it = factory_.find(name);

		if (it == factory_.end()) {
			LOGWARN("Can not find DealFileHandle:%s, Trans To default");
			it = factory_.find("default");
		}

		assert(it != factory_.end());

		return it->second;
	}

private:
	friend class GlobalObjectHelper<DealFileFactory>;
	DealFileFactory()
	{
		factory_.insert(make_pair(
			DealFileDefault::GetInstance()->GetClassName(), 
			DealFileDefault::GetInstance()));

		/*here to add more DealFile class instance*/
	}

private:
	std::map<std::string, DealFileBase*> factory_;
};

#endif
