#include<stdio.h>

#include"manager.h"

using namespace MyTool;


int main()
{
	M_INIT();
	M_REGIS(int, 1024);

	int* a = M_NEW(int, 10);

	for (int i = 0; i < 10; i++)//不安全操作
		a[i] = i;

	MPtr<int> mp(a + 1);
	for (MPtr<int>::Iterator it = mp.Start(); it != mp.End(); it++)//安全操作
	{
		printf(" %d ", *it);
	}
	//安全操作
	mp[4] = 10;
	printf("mp[4]:%d ", mp[4]);
	mp.Free();

	M_QUIT();
	getchar();
	return 0;
}