#include<stdio.h>

#include"manager.h"

using namespace MyTool;


int main()
{
	M_INIT();
	M_REGIS(int, 1024);

	int* a = M_NEW(int, 10);

	for (int i = 0; i < 10; i++)//����ȫ����
		a[i] = i;

	MPtr<int> mp(a + 1);
	for (MPtr<int>::Iterator it = mp.Start(); it != mp.End(); it++)//��ȫ����
	{
		printf(" %d ", *it);
	}
	//��ȫ����
	mp[4] = 10;
	printf("mp[4]:%d ", mp[4]);
	mp.Free();

	M_QUIT();
	getchar();
	return 0;
}