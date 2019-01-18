#include<atomic>
#include<exception>
#include"manager.h"




namespace MyTool
{

	struct ComMsg
	{
		uint posi;
		int cut;
	};

	  static MVector<MemoryHeadMsg> *mhm_list;
	  static MVector<MemoryMsgEx> *mme_free_list;
	  static MVector<MemoryMsgEx> *mme_ufree_list;
	 
	
	static std::atomic_char mhm_list_flag = 1;
	static  std::atomic_char mme_free_list_flag = 1;
	static  std::atomic_char mme_ufree_list_flag = 1;


	bool initflag = false;

	std::atomic_bool quit_flag=false;

#define LOCK(arg) do{arg##_flag--;while(arg##_flag<0){};}while(0);
#define ULOCK(arg) do{arg##_flag++;}while(0);

	bool str_equal(const char* str1, const char*str2)
	{
		if (str1 == str2)
			return true;
		uint len1 = strlen(str1);
		uint len2 = strlen(str2);
		if (len1 != len2)
			return false;
		else if (str1 == nullptr || str2 == nullptr||len1==0)
			return false;
		for (int i = 0; i < len1; i++)
			if (str1[i] != str2[i])
				return false;
		return true;
	}

	int M_find_id(const char* name)
	{
		if (quit_flag)
			return -1;
		LOCK(mhm_list);
		for (int i = 0; i < mhm_list->Length(); i++)
		{
			if (str_equal(mhm_list->operator[](i).mhm_name, name))
			{
				ULOCK(mhm_list);
				return i;
				
			}
		}
		ULOCK(mhm_list);
		return -1;
	}

	int M_regis_struct(const char* name, uint szof, ulong count)
	{
		


		if (quit_flag)
		{

			return -1;
		}
		if (M_find_id(name) != -1)
		{

			return -1;
		}



		LOCK(mhm_list)LOCK(mme_free_list)LOCK(mme_ufree_list);
		MemoryHeadMsg msg;
		msg.mhm_add_count = count;
		msg.mhm_szof = szof;
		msg.mhm_now_count = count;
		strcpy(msg.mhm_name, name);
		//注册头

		msg.mhm_id = mhm_list->Length();
		bit64 tlen = (bit64)szof * count; 
		byte* temp = nullptr;
		try
		{
			 temp = new byte[tlen];
		}
		catch(std::exception a)
		{

			return -1;
		}

		msg.mhm_addr = new MVector<byte*>(10);
		msg.mhm_addr->Push_back(temp);
		mhm_list->Push_back(msg);


		//注册相应的空间碎片信息
		MemoryMsgEx mme;
		mme.mhm_id = msg.mhm_id;
		mme.mme_msgs = new MVector<MemoryMsg>(count);
		MemoryMsg mm;
		mm.count = msg.mhm_now_count;
		mm.mm_addr = temp;
		mme.mme_msgs->Push_back(mm);

		mme_free_list->Push_back(mme);



		//注册相应的未释放内存信息
		mme.mhm_id = msg.mhm_id;
		mme.mme_msgs= new MVector<MemoryMsg>(count);


		mme_ufree_list->Push_back(mme);


		ULOCK(mhm_list)ULOCK(mme_free_list)ULOCK(mme_ufree_list);

		return msg.mhm_id;

	}

	byte* M_Malloc(uint id, ulong count)
	{

		if (quit_flag)
		{

			return nullptr;
		}
		LOCK(mhm_list)LOCK(mme_free_list)LOCK(mme_ufree_list);
		if (id >= mme_free_list->Length())
		{
			ULOCK(mhm_list)ULOCK(mme_free_list)ULOCK(mme_ufree_list);

			return nullptr;
		}


		MemoryMsgEx &mme = mme_free_list->operator[](id);//获取该类型空间碎片表
		
		byte* result=nullptr;
		ComMsg cm;
		ComMsg temp = { -1,-1 };
		cm.posi = 0;
		cm.cut = mme.mme_msgs->operator[](0).count - count;

		for (int i = 1; i < mme.mme_msgs->Length(); i++)
		{
			if (cm.cut == 0)
				break;

			MemoryMsg &mm = mme.mme_msgs->operator[](i);
			temp.posi = i;
			temp.cut = mm.count - count;
			if (temp.cut == 0)
			{
				cm.cut = 0;
				cm.posi = i;
				break;
			}
			else if (temp.cut > 0)
			{
				if (cm.cut < 0)
					memcpy(&cm, &temp, sizeof(ComMsg));
				else
					if(temp.cut<cm.cut)
						memcpy(&cm, &temp, sizeof(ComMsg));
			}
		}

		// 如果空间碎片不足长度,向系统申请新的空间

		if (cm.cut < 0)
		{

			MemoryHeadMsg &mhm = mhm_list->operator[](id);
			ulong newlen = mhm.mhm_add_count*mhm.mhm_szof;
			ulong adlen = newlen;
			while (newlen < count*mhm.mhm_szof)
			{
				newlen += adlen;
			}
			byte *temp = new byte[newlen];
			mhm.mhm_addr->Push_back(temp);


			result = temp;

			//添加新的空间碎片

			MemoryMsg newmm;
			newmm.mm_addr = temp + count*mhm.mhm_szof+1;
			newmm.count = newlen/mhm.mhm_szof-count;

			if(newmm.count)
				mme_free_list->operator[](id).mme_msgs->Push_back(newmm);


		}
		else if (cm.cut == 0)
		{
			result = mme.mme_msgs->operator[](cm.posi).mm_addr;
			//删除碎片信息

			mme.mme_msgs->Erase(cm.posi);

			
		}
		else
		{
			result = mme.mme_msgs->operator[](cm.posi).mm_addr;
			//修改碎片信息

			MemoryMsg&mm = mme.mme_msgs->operator[](cm.posi);

			MemoryHeadMsg &mhm = mhm_list->operator[](id);
			mm.mm_addr = mm.mm_addr + count*mhm.mhm_szof+1;

			mm.count = cm.cut;
		}

		//result = cm.cut < 0 ? result : mme.mme_msgs->operator[](cm.posi).mm_addr;

		//加入当前活跃内存信息

		MemoryMsg umm;
		umm.count = count;
		umm.mm_addr = result;
		mme_ufree_list->operator[](id).mme_msgs->Push_back(umm);

		ULOCK(mhm_list)ULOCK(mme_free_list)ULOCK(mme_ufree_list);

		return result;

	}

	MemoryMsgEx* find_msg(byte* ptr,int *posi)
	{
		if (quit_flag)
			return nullptr;
		LOCK(mhm_list)LOCK(mme_free_list)LOCK(mme_ufree_list);
		for (int i = 0; i < mme_ufree_list->Length(); i++)
		{
			MemoryMsgEx& msg = mme_ufree_list->operator[](i);
			for (int j = 0; j < msg.mme_msgs->Length(); j++)
			{
				MemoryMsg &mm = msg.mme_msgs->operator[](j);
				if (mm.mm_addr == ptr)
				{
					*posi = j;
					ULOCK(mhm_list)ULOCK(mme_free_list)ULOCK(mme_ufree_list);
					return &msg;
				}
			}
		}
		ULOCK(mhm_list)ULOCK(mme_free_list)ULOCK(mme_ufree_list);
		*posi = -1;
		return nullptr;
	}

	void M_free(byte* ptr)
	{
		if (quit_flag)
			return;
		int posi;
		
		MemoryMsgEx* msgptr = find_msg(ptr, &posi);
		LOCK(mhm_list)LOCK(mme_free_list)LOCK(mme_ufree_list);
		if (msgptr == nullptr)
		{
			ULOCK(mhm_list)ULOCK(mme_free_list)ULOCK(mme_ufree_list);
			return;
		}
		MemoryMsg &mm=msgptr->mme_msgs->operator[](posi);

		//加入到空间碎片信息 或者 合并碎片空间信息

		MemoryMsgEx&mex = mme_free_list->operator[](msgptr->mhm_id);
		uint szof = mhm_list->operator[](msgptr->mhm_id).mhm_szof;
		bool flag = false;
		//判断是否需要整合碎片
		for (int i = 0; i < mex.mme_msgs->Length(); i++)
		{
			MemoryMsg &temp = mex.mme_msgs->operator[](i);
			if (temp.mm_addr == (mm.mm_addr + (mm.count*szof))+1)//头尾相接
			{
				temp.mm_addr = mm.mm_addr;
				
				temp.count += mm.count;

				flag = true;
				break;
			}
			else if ((temp.mm_addr + (temp.count*szof))+1 == mm.mm_addr)//尾头相接
			{
				temp.count += mm.count;
				flag = true;
				break;
			}
		}
		if (!flag)//不存在整合碎片的情况
		{
			mex.mme_msgs->Push_back(mm);
		}
		

		//从动态的内存信息表移除
		bool f=mme_ufree_list->operator[](msgptr->mhm_id).mme_msgs->Erase(posi);

		int i = 0;


		ULOCK(mhm_list)ULOCK(mme_free_list)ULOCK(mme_ufree_list);
	}

	void M_get_msg(byte* ptr,MemoryMsgDetailUnit&msg)
	{
		if (quit_flag)
			return;
		LOCK(mhm_list)LOCK(mme_free_list)LOCK(mme_ufree_list);
		for (int i = 0; i < mme_ufree_list->Length(); i++)
		{
			MEMORYMSGEX &mex = mme_ufree_list->operator[](i);
			for (int j = 0; j < mex.mme_msgs->Length(); j++)
			{
				MemoryMsg &mm = mex.mme_msgs->operator[](j);
				byte* end = mm.mm_addr + mhm_list->operator[](mex.mhm_id).mhm_szof*mm.count;
				if (ptr >= mm.mm_addr&&ptr <= end)
				{
					memcpy(msg.name, mhm_list->operator[](mex.mhm_id).mhm_name, M_MAX_NAME_SIZE);
					msg.szof = mhm_list->operator[](mex.mhm_id).mhm_szof;
					msg.posi = (ptr - mm.mm_addr) / 4;
					msg.count = mm.count;
					ULOCK(mhm_list)ULOCK(mme_free_list)ULOCK(mme_ufree_list);
					return;
				}
			}
		}
		msg.posi = -1;
		ULOCK(mhm_list)ULOCK(mme_free_list)ULOCK(mme_ufree_list);
	}

	void M_quit()
	{
		if (quit_flag)
			return;
		LOCK(mhm_list)LOCK(mme_free_list)LOCK(mme_ufree_list);
		//删除总空间
		for (int i = 0; i < mhm_list->Length(); i++)
		{
			MemoryHeadMsg &mhm = mhm_list->operator[](i);
			for (int j = 0; j < mhm.mhm_addr->Length(); j++)
			{
				byte* &temp = mhm.mhm_addr->operator[](j);
				if (temp)
					delete[]temp;
				temp = nullptr;
			}
			delete mhm.mhm_addr;
		}
		//释放碎片信息
		for (int i = 0; i < mme_free_list->Length(); i++)
		{
			MemoryMsgEx &mme = mme_free_list->operator[](i);
			for (int j = 0; j < mme.mme_msgs->Length(); j++)
			{
				//添加日志信息

			}
			delete mme.mme_msgs;
		}
		//释放活跃内存信息

		for (int i = 0; i < mme_ufree_list->Length(); i++)
		{
			MemoryMsgEx &mme = mme_ufree_list->operator[](i);
			for (int j = 0; j < mme.mme_msgs->Length(); j++)
			{
				//添加日志信息

			}
			delete mme.mme_msgs;
		}

		delete mhm_list, mhm_list = nullptr;
		delete mme_free_list, mme_free_list = nullptr;
		delete mme_ufree_list, mme_ufree_list = nullptr;

		ULOCK(mhm_list)ULOCK(mme_free_list)ULOCK(mme_ufree_list);
		quit_flag = true;
	}
	void M_init()
	{
		LOCK(mhm_list)LOCK(mme_free_list)LOCK(mme_ufree_list);
		if (!initflag)
		{
			mhm_list = new MVector<MemoryHeadMsg>();
			mme_free_list = new  MVector<MemoryMsgEx>();
			mme_ufree_list = new  MVector<MemoryMsgEx>();
			initflag = true;
		}
		ULOCK(mhm_list)ULOCK(mme_free_list)ULOCK(mme_ufree_list);
	}

	void M_printf_log()
	{

	}
}