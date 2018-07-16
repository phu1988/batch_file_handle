/*
head = base
int64 shm_size_;
int64 one_pid_per_size_ = (shm_size_ - sizeof(head) - head->process_count_ * sizeof(process_glob))/head->process_count_;
int64 split_memory_count_one_proc_;
unsigned int process_count_;
*/
struct ShmHead
{
	int64 shm_size_;
	int64 one_pid_per_size_;
	int64 split_memory_count_one_proc_;
	unsigned int process_count_;
};

/*
process_glob(i) = base + sizeof(head) + sizeof(process_glob) * i
pid_t process_pid_;
int64 process_address;
int64 data_len_;
*/
struct ProcGlob
{
	pid_t process_pid_;
	int64 proc_addr_;
	int64 data_len_;
	int memory_split_count_;
	int memory_free_count_;
	int64 proc_space_head_free_entry_;
};

/*
private space(i) = base + sizeof(head) + head->process_count_ * sizeof(process_glob) + one_pid_per_size_ * i
int area_seq_;
char task_run_flag_[64]
int64 memory_size_;
int use_status_;
*/
struct ProcSpaceHead
{
	int area_seq_; //start from 0, limit to memory_split_count_ - 1
	char task_run_flag_[128];
	int use_status_;//0:un use 1:using 2: used
	int64 start_addr_;
	int64 data_len_;
};

class IoInterface
{
public:
	void Initialize(void* address, int64 maxLen)
	{
		base_address_ = address;
		max_length_ = maxLen;
	}
	
	void SetValue(int64 offset, void* value, size_t valueLen)
	{
		assert(offset + valueLen <= max_length_);
		
		if (value == null) {
			memset(base_address_ + offset, 0, valueLen);
		} else {
			memcpy(base_address_ + offset, value, valueLen);
		}
	}
	
	template<typename Type>
	Type* GetValue(int64 offset)
	{
		assert(offset + sizeof(Type) < max_length_);
		
		return reinterpret_cast<Type*>(base_address_ + offset);
	}
	
	inline int64 GetMaxLength()
	{
		return max_length_;
	}
	
private:
	char* base_address_;
	int64 max_length_;
	
};

class MemoryManager
{
public:
	MemoryManager()
		: io_type_(IO_TYPE_UNKOWN)
		, io_oper(NULL)
	{
	}
	
	virtual ~MemoryManager()
	{
	}
	
	virtual void Initialize(IO_TYPE ioType)
	{
		io_type_ = ioType;
		if (IO_TYPE_SHM == io_type_) {
			io_oper_ = IoShmMemory::GetInstance();
		} else if (IO_TYPE_MEMORY == io_type_) {
			io_oper_ = IoShmMemory::GetInstance();
		} else if (IO_TYPE_FILEMAPPING == io_type_) {
			io_oper_ = IoFileMapping::GetInstance();
		}
	}
	
	IoInterface* GetIoOper()
	{
		return io_oper_;
	}
	
	void* GetHead()
	{
		return io_operator_->GetValue(0);
	}
	
	virtual int64 Allocate(int mSize) = 0;
	
protected:
	IO_TYPE io_type_;
	IoInterface* io_oper_;
	
};

template<int ProcSeq>
class ProcMemoryManager: public MemoryManager, 
												 public GlobObjectHelper< ProcMemoryManager<ProcSeq> >
{
public:
	ProcMemoryManager()
		: now_proc_space_(NULL)
		, use_len_(0)
	{
	}
	
	~ProcMemoryManager()
	{
	}
	
	void Initialize(IOTYPE type)
	{
		__supper::Initialize(type);
	}
	
	void AttachNewTask(const char* taskInfo)
	{
		strncpy(task_run_flag_, taskInfo, sizeof(task_run_flag_) - 1);
		/*reset the space and len*/
		now_proc_space_ = NULL;
		use_len_ = 0;
	}
	
	int64 Allocate(int64 mSize)
	{
		if (now_proc_space_) {
			if (use_len_ + mSize <= now_proc_space_->data_len_) {
				use_len_ += mSize;
				return now_proc_space_->start_addr_ + use_len_ - mSize;
			}
		}
		
		while (true) {
			/*1.get the free process space*/
			now_proc_space_ = GetProcSpaceHeadFree();
			/*there is no free space*/
			if (NULL == now_proc_space_) return -1;
			/*2.judge have enough space*/
			if (mSize > now_proc_space_->data_len_) continue;
			/*3.set the task info*/
			strncpy(now_proc_space_->task_run_flag_, 
				task_run_flag, sizeof(now_proc_space_->task_run_flag_) - 1);
			/*4.set use status*/
			now_proc_space_->use_status_ = 1;
			/*5.set the free point, free count*/
			ProcGlob()->memory_free_count_--;
			/*6.set the free space entry*/
			for (int i = 0; i < ProcGlob()->memory_split_count_; ++i) {
				if (0 == GetProcSpaceHead(i)->use_status_) {
					ProcGlob()->proc_space_head_free_entry_ = 
						ProcGlob()->proc_addr_ + sizeof(ProcSpaceHead) * i;
				}
			}
			if (i == ProcGlob()->memory_split_count_) {
				ProcGlob()->proc_space_head_free_entry_ = -1;
			}
			
			use_len_ = mSize;
			return now_proc_space_->start_addr_;
		}
		
		return -1;
	}
	
private:
	ProcGlob* ProcGlob()
	{
		return io_oper_->GetValue(sizeof(ShmHead) + procSeq * sizeof(ProcessGlob));
	}
	
	ProcSpaceHead* GetProcSpaceHeadEntry()
	{
		return io_oper_->GetValue(ProcGlob()->proc_addr_);
	}
	
	ProcSpaceHead* GetProcSpaceHead(int seq)
	{
		assert(seq < ProcGlob()->memory_split_count_);
		
		return io_oper_->GetValue(ProcGlob()->proc_addr_ + sizeof(ProcSpaceHead) * seq);
	}
	
	ProcSpaceHead* GetProcSpaceHeadFree()
	{
		if (GetProcGlob()->proc_space_head_free_entry_ < 0) return NULL;
		
		return io_oper_->GetValue(GetProcGlob()->proc_space_head_free_entry_);
	}
	
private:
	ProcSpaceHead* now_proc_space_;
	int64 use_len_;
	char task_run_flag_[128];
	
};

//the hash memory locate continues memory space
class HashArray
{
public:
	HashArray(int64 hashCount, MemoryManager* mOper)
		: hash_count_(hashCount)
		, mm_oper_(mOper)
		, my_addr_(-1)
	{
	}
	
	~HashArray()
	{
	}
	
	void Initialize()
	{
		my_addr_ = mm_oper_->Allocate(hash_count_ * sizeof(int64));
	}
	
	/*index from 0 to hashCount*/
	void Set(int64 index, int64 listAddress)	
	{
		assert(index < hashCount);
		
		mm_oper_->GetIoOper()->SetValue(my_addr_ + index * sizeof(int64), 
													 &listAddress, sizeof(int64));
	}
	
	/*index from 0 to hashCount*/
	int64 Get(int64 index)
	{
		assert(index < hashCount);
		
		return mm_oper_->GetIoOper()->GetValue(my_addr_ + index * sizeof(int64));
	}
	
private:
	int64 hash_count_;
	MemoryManager* mm_opr_;
	
	int64 my_addr_;
};

//the list memory space is not necessary continues
class HashList
{
	HashList(size_t keyLen, size_t valueLen, MemoryManager* mmOper)
		: key_len_(keyLen)
		, value_len_(valueLen)
		, mm_opr_(mmOper)
		, my_addr_(-1)
	{
	}
	
	~HashList() {}
	
	void Initialize()
	{		
		/*allocate for recyle&count*/
		my_addr_ = mm_opr_->Allocate(sizeof(int64) + sizeof(int64));
		
		int64 recyle = -1, count = 0;
		IoInterface* io = mm_opr_->GetIoOper();
		io->SetValue(my_addr_, &recyle, sizeof(int64));
		io->SetValue(my_addr_ + sizeof(int64), &count, sizeof(int64));
	}
	
	void SetNext(int64 pre, int64 next)
	{
		IoInterface* io = mm_opr_->GetIoOper();
		io->SetValue(pre + key_len_ + value_len_, &next, sizeof(next));
	}
	
	int64 GetNext(int64 data)
	{
		IoInterface* io = mm_opr_->GetIoOper();
		return io->GetValue(data + key_len_ + value_len_);
	}
	
	int64 Recyle(int64 data)
	{
		int recyle = GetRecyle();
		if (recyle > 0) {
			SetNext(data, recyle);
		}
		SetRecyle(data);
		return data;
	}
	
	int64 Allocate()
	{
		/*first find in recyle*/
		int64 recyle = GetRecyle();
		if (recyle > 0) {
			SetRecyle(GetNext(recyle));
			return recyle;
		}
		
		return mm_opr_->Allocate(key_len_ + value_len_ + sizeof(int64));
	}

protected:
	inline int64 GetRecyle()
	{
		IoInterface* io = mm_opr_->GetIoOper();
		return io->GetValue(my_addr_);
	}
	
	inline void SetRecyle(int64 recyle)
	{
		IoInterface* io = mm_opr_->GetIoOper();
		io->SetValue(my_addr_, &recyle, sizeof(recyle));
	}

	inline int64 GetCount()
	{
		IoInterface* io = mm_opr_->GetIoOper();
		return io->GetValue(my_addr + sizeof(int64));
	}
	
	inline void SetCount(int64 count)
	{
		IoInterface* io = mm_opr_->GetIoOper();
		io->SetValue(my_addr_ + sizeof(int64), &count, sizeof(count));
	}
		
protected:
	/*fix:  recyle|count*/
	/*body: key|value|next*/
	/*int64 free_; free is sign in ProcMemoryManager, so list only set the recyle*/
	size_t key_len_;
	size_t value_len_;
	
	MemoryManager* mm_opr_;
	int64 my_addr_;
};

template<typename valueCombineHelper, typename helper = DefaultHashHelper>
class Hash
{
public:
	Hash(int keyLen, int valueLen, MemoryManager* mmOper)
		: key_len_(keyLen)
		, value_len_(valueLen)
		, hash_array_(new HashArray(helper::GetCount(), mmOper))
		, hash_list_(new HashList(keyLen, valueLen, mmOper))
		, mm_oper_(mmOper)
	{
	}
	
	~Hash()
	{
		if (hash_array_) delete hash_array_;
		if (hash_list_) delete hash_list_; 
	}
	
	void Initialize()
	{
		/*array must initialize before the list*/
		hash_array_->Initialize();
		hash_list_->Initialize();
	}
	
	int64 Put(void* key, void* value)
	{
		int64 index = DefaultHashHelper::Get(key, key_len_);
		int64 listEntry = hash_array_->Get(index);
		
		IoInterface* io = mm_opr_->GetIoOper();
		int64 lEntry = listEntry;
		while (lEntry > 0) {
			char* cmp = io->GetValue(lEntry);
			if (memcmp(cmp, key, key_len_) == 0) {
				valueCombineHelper::Combine(io->GetValue(lEntry + ken_len_), 
					                          value, value_len_);
				return lEntry;
			}
			
			lEntry = hash_list_->GetNext(lEntry);
		}
			
		int64 listAddr = hash_list_->Allocate();
		if (listAddr < 0) return -1;
			
		io->SetValue(listAddr, key, ken_len_);
		io->SetValue(listAddr + key_len_, value, value_len_);
		
		if (listEntry > 0) {
			hash_list_->SetNext(listAddr, listEntry);
		} else {
			hash_list_->SetNext(listAddr, 0);
		}
		
		hash_array_->Set(index, listAddr);
		
		return listAddr;
	}
	
private:
	int key_len_;
	int value_len_;
	
	HashArray* hash_array_;
	HashList* hash_list_;
	MemoryManager* mm_oper_;
	
};