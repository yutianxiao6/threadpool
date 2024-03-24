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
			cout << "thread����ʧ��" << endl;
			break;
		}
		memset(threadIDs, 0, sizeof(pthread_t) * max); //����
		minNum = min;
		maxNum = max;
		busyNum = 0;
		liveNum = min;
		exitNum = 0;
		shutdown = false;

		//��ʼ������������������
		if (pthread_mutex_init(&mutexpool, 0) != 0 ||
			pthread_cond_init(&notEmpty, 0) != 0)
		{
			cout << "��������������������ʼ��ʧ��" << endl;
			break;
		}


		//�����߳�
		pthread_create(&this->managerID, NULL, &ThreadPool::manager, this);
		for (int i = 0; i < min; i++)
		{
			pthread_create(&threadIDs[i], NULL, &ThreadPool::worker, this);
		}
		return;
	} while (0);

	//����ʹ��do while����Ϊ����������ʼ���Ĺ����������һ������ɹ��ڶ�������ʧ�ܣ��ͻ����return�˳���������Դȴû��free������д����ʹ��break�˳�ѭ��
	//�����ͳһ����free������




	//�ͷ���Դ
	if (threadIDs) delete[] threadIDs;
	if (taskQ) delete taskQ;

}

ThreadPool::~ThreadPool()
{
	//�ر��̳߳�
	shutdown = true;
	//�������չ������߳�
	pthread_join(managerID, NULL);
	//�����������������߳�
	for (int i = 0; i < liveNum; ++i)
	{
		pthread_cond_signal(&notEmpty);
	}
	//�ͷŶ��ڴ�
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
	//�������
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
		//��ǰ��������Ƿ�Ϊ��
		while (pool->taskQ->taskNumber() == 0 && !pool->shutdown)
		{
			//������������
			pthread_cond_wait(&pool->notEmpty, &pool->mutexpool);
			//�ж��Ƿ�Ҫ�����߳�
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
		//�ж��̳߳��Ƿ񱻹ر���
		if (pool->shutdown)
		{
			pthread_mutex_unlock(&pool->mutexpool);
			pool->threadExit();
		}
		//���̳߳�ȡ��һ������
		Task task=pool->taskQ->takeTask();
		
		//����
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
		//ÿ��������һ��
		sleep(3);
		//ȡ���̳߳�������������͵�ǰ�̵߳�����
		pthread_mutex_lock(&pool->mutexpool);
		int queueSize = pool->taskQ->taskNumber();
		int liveNum = pool->liveNum;
		//ȡ��æ���߳���
		int busyNum = pool->busyNum;
		pthread_mutex_unlock(&pool->mutexpool);
		//����߳�
		//����ĵĸ���>����ĸ���&&������߳���<����߳���
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

		//�����߳�
		//æ���߳�*2<������߳���&&�����߳�>��С�߳�
		if (busyNum * 2 < liveNum && liveNum > pool->minNum)
		{
			pthread_mutex_lock(&pool->mutexpool);
			pool->exitNum = NUMBER;
			pthread_mutex_unlock(&pool->mutexpool);
			//�ù������߳���ɱ
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


