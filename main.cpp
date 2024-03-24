#include<iostream>
#include "threadpool.h"
#include<pthread.h>
#include<unistd.h>
using  namespace std;


void taskFunc(void* arg)
{
	int num = *(int*)arg;
	cout << "�߳�" << num << "�Ѿ�������id = " << pthread_self() << endl;
	usleep(1000);
}


int main()
{
	//�����̳߳�
	ThreadPool pool(3, 10, 100);
	for (int i = 0; i < 100; i++)
	{
		int* num = (int*)malloc(sizeof(int));
		*num = i + 100;
		pool.threadPoolAdd(Task(taskFunc, num));
	}

	sleep(20);
	return 0;
}

