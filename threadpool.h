#pragma once

#include<queue>
#include<pthread.h>
using namespace std;
using callback = void(*)(void* arg);

class Task
{
public:
	Task();
	Task(callback f, void* arg);
	~Task();

	callback function;
	void* arg;
};

class TaskQueue
{
public:
	TaskQueue();
	~TaskQueue();
	//�������
	void addTask(Task task);
	void addTask(callback f, void* arg);
	//ȡ������
	Task takeTask();
	//��ȡ��ǰ����ĸ���
	inline int taskNumber()
	{
		return m_taskQ.size();
	}
private:
	queue<Task> m_taskQ;
	pthread_mutex_t m_mutex;
};


//�̳߳ؽṹ��
class ThreadPool
{
public:
	ThreadPool(int min, int max, int queueSize);
	~ThreadPool();
	//�������߳�
	void threadPoolAdd(Task task);
	
	//��ȡ�̳߳��й������̸߳���
	int threadPoolBusyNum();
	//��ȡ�̳߳��л��ŵ��̸߳���
	int threadPoolLiveNum();
private:
	//�������߳�
	static void* worker(void* arg);
	//�������߳�
	static void* manager(void* arg);
	//�����߳��˳�
	void threadExit();
private:
	//�������
	TaskQueue* taskQ;

	pthread_t managerID;   //�������߳�id
	pthread_t* threadIDs;  //�������߳�Id
	int minNum;     //��С�߳���
	int maxNum;     //����߳���
	int busyNum;    //�������߳�
	int liveNum;    //�����߳�
	int exitNum;    //Ҫɱ�����̸߳���
	pthread_mutex_t mutexpool;   //�������̳߳�
	pthread_cond_t notEmpty;   //��������Ƿ�Ϊ��
	bool shutdown;                //�ǲ���Ҫ�����̳߳أ�����Ϊ1��������Ϊ0
	static const int NUMBER = 2;



};




