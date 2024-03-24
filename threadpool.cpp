#include "threadpool.h"
#include<pthread.h>
#include<stdlib.h>
#include<iostream>
#include<cstring>
#include <unistd.h>


using namespace std;


ThreadPool::ThreadPool(int min, int max, int queueSize)
{
	taskQ = new TaskQueue;
	do
	{
		if (taskQ == nullptr)
		{
			cout << "taskQ fial" << endl;
			break;
		}
		threadIDs = new pthread_t[max];
		if (threadIDs == nullptr)
		{
			cout << "thread分配失败" << endl;
			break;
		}
		memset(threadIDs, 0, sizeof(pthread_t) * max); //置零
		minNum = min;
		maxNum = max;
		busyNum = 0;
		liveNum = min;
		exitNum = 0;
		shutdown = false;

		//初始化互斥锁和条件变量
		if (pthread_mutex_init(&mutexpool, 0) != 0 ||
			pthread_cond_init(&notEmpty, 0) != 0)
		{
			cout << "互斥锁或者条件变量初始化失败" << endl;
			break;
		}


		//创建线程
		pthread_create(&this->managerID, NULL, &ThreadPool::manager, this);
		for (int i = 0; i < min; i++)
		{
			pthread_create(&threadIDs[i], NULL, &ThreadPool::worker, this);
		}
		return;
	} while (0);

	//这里使用do while是因为如果在上面初始化的过程中如果第一个分配成功第二个分配失败，就会调用return退出，但是资源却没有free，这样写可以使用break退出循环
	//在最后统一进行free操作。




	//释放资源
	if (threadIDs) delete[] threadIDs;
	if (taskQ) delete taskQ;

}

ThreadPool::~ThreadPool()
{
	//关闭线程池
	shutdown = true;
	//阻塞回收管理者线程
	pthread_join(managerID, NULL);
	//唤醒阻塞的消费者线程
	for (int i = 0; i < liveNum; ++i)
	{
		pthread_cond_signal(&notEmpty);
	}
	//释放堆内存
	if (taskQ)
	{
		delete taskQ;
	}
	if (threadIDs)
	{
		delete[] threadIDs;
	}
	pthread_mutex_destroy(&mutexpool);
	pthread_cond_destroy(&notEmpty);
}

void ThreadPool::threadPoolAdd(Task task)
{
	if (shutdown)
	{
		pthread_mutex_unlock(&this->mutexpool);
		return;
	}
	//添加任务
	taskQ->addTask(task);
	pthread_cond_signal(&notEmpty);

}



int ThreadPool::threadPoolBusyNum()
{
	pthread_mutex_lock(&mutexpool);
	int busyNum = busyNum;
	pthread_mutex_unlock(&mutexpool);
	return busyNum;
}

int ThreadPool::threadPoolLiveNum()
{
	pthread_mutex_lock(&mutexpool);
	int aLiveNum = liveNum;
	pthread_mutex_unlock(&mutexpool);
	return aLiveNum;
}

void* ThreadPool::worker(void* arg)
{
	ThreadPool* pool = static_cast<ThreadPool*>(arg);
	while (1)
	{
		pthread_mutex_lock(&pool->mutexpool);
		//当前任务队列是否为空
		while (pool->taskQ->taskNumber() == 0 && !pool->shutdown)
		{
			//阻塞工作函数
			pthread_cond_wait(&pool->notEmpty, &pool->mutexpool);
			//判断是否要销毁线程
			if (pool->exitNum > 0)
			{
				pool->exitNum--;
				if (pool->liveNum > pool->minNum)
				{
					pool->liveNum--;
					pthread_mutex_unlock(&pool->mutexpool);
					pool->threadExit();
				}
				
			}
		}
		//判断线程池是否被关闭了
		if (pool->shutdown)
		{
			pthread_mutex_unlock(&pool->mutexpool);
			pool->threadExit();
		}
		//从线程池取出一个任务
		Task task=pool->taskQ->takeTask();
		
		//解锁
		pool->busyNum++;
		pthread_cond_signal(&pool->notEmpty);
		pthread_mutex_unlock(&pool->mutexpool);
		cout << "thread start work"<<pthread_self() << endl;


		task.function(task.arg);
		delete task.arg;
		task.arg = nullptr;
		cout << "thread end work" << pthread_self() << endl;

		pthread_mutex_lock(&pool->mutexpool);
		pool->busyNum--;
		pthread_mutex_unlock(&pool->mutexpool);
	}
	return 0;
}

void* ThreadPool::manager(void* arg)
{
	ThreadPool* pool = (ThreadPool*)arg;
	while (!pool->shutdown)
	{
		//每隔三秒检测一次
		sleep(3);
		//取出线程池中任务的数量和当前线程的数量
		pthread_mutex_lock(&pool->mutexpool);
		int queueSize = pool->taskQ->taskNumber();
		int liveNum = pool->liveNum;
		//取出忙的线程数
		int busyNum = pool->busyNum;
		pthread_mutex_unlock(&pool->mutexpool);
		//添加线程
		//任务的的个数>存货的个数&&存货的线程数<最大线程数
		if (queueSize > liveNum && liveNum < pool->maxNum)
		{
			pthread_mutex_lock(&pool->mutexpool);
			int conter = 0;
			for (int i = 0; i < pool->maxNum && conter < NUMBER
				&& pool->liveNum < pool->maxNum; ++i)
			{
				if (pool->threadIDs[i] == 0)
				{
					pthread_create(&pool->threadIDs[i], NULL, ThreadPool::worker, pool);
					conter++;
					pool->liveNum++;
				}
			}
			pthread_mutex_unlock(&pool->mutexpool);
		}

		//销毁线程
		//忙的线程*2<存货的线程数&&存活的线程>最小线程
		if (busyNum * 2 < liveNum && liveNum > pool->minNum)
		{
			pthread_mutex_lock(&pool->mutexpool);
			pool->exitNum = NUMBER;
			pthread_mutex_unlock(&pool->mutexpool);
			//让工作的线程自杀
			for (int i = 0; i < NUMBER; i++)
			{
				pthread_cond_signal(&pool->notEmpty);
			}
		}
	}
	return 0;
}

void ThreadPool::threadExit()
{
	pthread_t tid = pthread_self();
	for (int i = 0; i < maxNum; ++i)
	{
		if (threadIDs[i] == tid)
		{
			threadIDs[i] = 0;
			cout <<"thread Exit=" << tid << endl;
			break;
		}
	}
	pthread_exit(NULL);
}

TaskQueue::TaskQueue()
{
	pthread_mutex_init(&m_mutex, NULL);

}

TaskQueue::~TaskQueue()
{
	pthread_mutex_destroy(&m_mutex);
}

void TaskQueue::addTask(Task task)
{
	pthread_mutex_lock(&m_mutex);
	m_taskQ.push(task);
	pthread_mutex_unlock(&m_mutex);
}

void TaskQueue::addTask(callback f, void* arg)
{
	pthread_mutex_lock(&m_mutex);
	m_taskQ.push(Task(f, arg));
	pthread_mutex_unlock(&m_mutex);
}

Task TaskQueue::takeTask()
{
	Task t;
	pthread_mutex_lock(&m_mutex);
	if (!m_taskQ.empty())
	{
		t = m_taskQ.front();
		m_taskQ.pop();
	}
	pthread_mutex_unlock(&m_mutex);
	return t;
}

Task::Task()
{
	function = nullptr;
	arg = nullptr;
}

Task::Task(callback f, void* arg)
{
	this->arg = arg;
	function = f;
}

Task::~Task()
{

}


