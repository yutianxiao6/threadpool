#include<iostream>
#include "threadpool.h"
#include<pthread.h>
#include<unistd.h>
using  namespace std;


void taskFunc(void* arg)
{
	int num = *(int*)arg;
	cout << "线程" << num << "已经启动，id = " << pthread_self() << endl;
	usleep(1000);
}


int main()
{
	//创建线程池
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

