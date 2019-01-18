#include<memory>
#ifndef MEMORY_MANAGER
#define MEMORY_MANAGER
#else
#error has been defined 'MEMORY_MANAGER'
#endif // !MEMORY_MANAGER
#define OPERATOR_SIZE_64


namespace MyTool
{
#define M_LOG_EXCEPTION 0x01
#define M_LOG_MSG_HEAD 0x02
#define M_LOG_UFREE 0x04
#define M_LOG_FREE 0x08

#define M_DEFAULT_ADDLEN 1000
#define M_MAX_NAME_SIZE 32
	typedef unsigned char uchar,byte;
	typedef unsigned short ushort;
	typedef unsigned int  uint;
	typedef unsigned long ulong,bit32;
	typedef unsigned long long bit64;

	template<class T>
	class MVector
	{

	private:
		T *data;
		ulong now_count;
		ulong add_count;
		long length;
	public:
		typedef bool(*EraseFunc)(T&t);
		MVector()
		{
			data = new T[M_DEFAULT_ADDLEN];
			now_count = M_DEFAULT_ADDLEN;
			add_count = M_DEFAULT_ADDLEN;
			length = 0;

		};
		MVector(ulong sz)
		{
			data = new T[sz];
			now_count = sz;
			add_count = sz;
			length = 0;
		};
		MVector(const MVector&mv)
		{
			data = new T[mv.now_count];
			now_count = mv.now_count;
			add_count = mv.add_count;
			length = mv.length;
			memcpy(data, mv.data, sizeof(T)*length);
		};

		virtual~MVector()
		{
			if (!data)
			{
				delete[]data;
			}
			data = nullptr;
			length = 0;
			now_count = 0;
			add_count = 0;
		};
		void operator=(const MVector&mv)
		{
			if (!data)
				delete[]data;
			data = new T[mv.now_count];
			now_count = mv.now_count;
			add_count = mv.add_count;
			length = mv.length;
			memcpy(data, mv.data, sizeof(T)*length);
		};
		T& operator[](ulong posi)
		{
			return data[posi];
		};

		void Push_back(T&t)
		{
			if (length > now_count)
			{
				now_count += add_count;
				T *temp = new T[now_count];
				memcpy(temp, data, sizeof(T)*length);
				delete[]data;
				data = temp;
			}
			memcpy(&data[length], &t, sizeof(T));
			length++;

		};
		void Pop_back()
		{
			if (length < 0)
				return;
			length--;
		}
		bool Erase(EraseFunc fun)
		{
			if (length < 0)
				return false;
			for (long i = 0; i < length; i++)
			{
				if (fun(data[i]))
				{
					//开始 前移

					while (i + 1 < length)
					{
						memcpy(&data[i], &data[i + 1], sizeof(T));
						i++;
					}
					length--;
					return true;

				}
			}
			return false;
		}
		bool Erase(ulong posi)
		{
			if (posi >= length)
				return false;
			for (int i = posi; i + 1 < length; i++)
			{
				memcpy(&data[i], &data[i + 1], sizeof(T));
			}
			length--;
			return true;
		}
		ulong Length()
		{
			return length;
		}
	};
#ifdef OPERATOR_SIZE_64 //64位操作系统
#pragma pack(push)
#pragma pack(8)//8字节对齐

	typedef struct MemoryHeadMsg
	{
		char mhm_name[M_MAX_NAME_SIZE];
		uint mhm_id;
		uint mhm_szof;
		MVector<byte*> *mhm_addr;
		ulong mhm_now_count;
		ulong mhm_add_count;

	}MEMORYHEADMSG;

	typedef struct MemoryMsg
	{
		byte* mm_addr;
		uint count;
	}MEMORYMSG;

	typedef struct MemoryMsgEx
	{
		uint mhm_id;
		MVector<MemoryMsg> *mme_msgs;
	}MEMORYMSGEX;

	typedef struct MemoryMsgDetailUnit
	{
		char name[M_MAX_NAME_SIZE];
		uint szof;
		int posi;
		ulong count;
	}MEMORYMSGDETAILUNIT;
#pragma pack(pop)
#endif // OPERATOR_SIZE_64

	int M_regis_struct(const char* name, uint szof, ulong count);
	int M_find_id(const char* name);
	byte* M_Malloc(uint id, ulong count);
	void M_free(byte* ptr);
	void M_get_msg(byte* ptr, MemoryMsgDetailUnit&msg);
	void M_quit();
	void M_printf_log();
	void M_init();

#define M_REGIS(type,count) M_regis_struct((#type),sizeof(type),(count))

#define M_FIND(type) M_find_id(#type)
#define M_NEW(type,count) (type*)M_Malloc(M_FIND(type),count)
#define M_FREE(ptr) M_free((byte*)(ptr))
#define M_MSG(ptr,msgstruct) M_get_msg((byte*)(ptr),msgstruct)
#define M_QUIT() M_quit()
#define M_PRINTF_LOG() M_printf_log()
#define M_INIT() M_init();

	template<class T>
	class MPtr
	{
	private:
		uint len;
		T* data;
	public:
		MPtr()
		{
			data = nullptr;
			len = 0;
		}
		MPtr(T* ptr)
		{
			if (ptr == nullptr)
			{
				data = nullptr;
				return;
			}
			MemoryMsgDetailUnit mmdu;
			M_MSG(ptr, mmdu);
			if (mmdu.posi < 0)
				return;
			if (mmdu.posi != 0)
				ptr -= (mmdu.posi);
			data = ptr;
			len = mmdu.count;
		}
		MPtr(MPtr&mmptr)
		{
			data = mmptr.data;
			len = mmptr.len;
		}
		~MPtr()
		{
			data = nullptr;
		}

		void Free()
		{
			M_FREE(data);
			data = nullptr;
		}

		T& operator[](uint posi)
		{
			if (posi >= len)
				//添加错误信息日志
				;
			return data[posi];
		}
		bool operator==(MPtr&mp)
		{
			if (mp.data == data)
				return true;
		}
		void operator=(MPtr&mp)
		{
			data = mp.data;
			len = mp.len;
		}
		class Iterator
		{
		private:
			T *data;
		public:
			Iterator()
			{
				data = nullptr;
			};
			Iterator(T*t)
			{
				data = t;
			};
			Iterator(Iterator&&it)
			{
				data = it.data;
			}
			Iterator(Iterator&it)
			{
				data = it.data;
			};
			
			T& operator*()const
			{
				if (data == nullptr)
				{
					T t;
					return t;
				}	
				return *data;
			};
			Iterator operator++()
			{
				if(data!=nullptr)
					data++;
				return *this;
			};
			Iterator operator--()
			{
				if (data != nullptr)
					data--;
				return *this;
			};

			Iterator operator++(int)
			{
				Iterator it = *this;
				if (data != nullptr)
					data++;
				return it;
			};
			Iterator operator--(int)
			{
				Iterator it = *this;
				if (data != nullptr)
					data--;
				return it;
			};



			~Iterator() {};

			bool operator==(Iterator&it)
			{
				if (it.data == data)
					return true;
				return false;
			};
			bool operator==(Iterator&&it)
			{
				if (it.data == data)
					return true;
				return false;
			};
			bool operator!=(Iterator&it)
			{
				if (it.data != data)
					return true;
				return false;
			};
			bool operator!=(Iterator&&it)
			{
				if (it.data != data)
					return true;
				return false;
			};
			void operator=(Iterator&it)
			{
				data = it.data;
			};
			void operator=(Iterator&&it)
			{
				data = it.data;
			};
		};
		Iterator Start()
		{
			Iterator it(data);
			
			return it;
		}
		Iterator End()
		{
			if (data == nullptr)
			{
				Iterator it(nullptr);
				return it;

			}

			T* temp = data;
			MemoryMsgDetailUnit mmdu;
			M_MSG(temp, mmdu);
			if (mmdu.posi < 0)
				//add exception log
				;
			temp += (mmdu.count);
			Iterator it(temp);
			return it;
		}
	};

}