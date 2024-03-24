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
	//添加任务
	void addTask(Task task);
	void addTask(callback f, void* arg);
	//取出任务
	Task takeTask();
	//获取当前任务的个数
	inline int taskNumber()
	{
		return m_taskQ.size();
	}
private:
	queue<Task> m_taskQ;
	pthread_mutex_t m_mutex;
};


//线程池结构体
class ThreadPool
{
public:
	ThreadPool(int min, int max, int queueSize);
	~ThreadPool();
	//生产者线程
	void threadPoolAdd(Task task);
	
	//获取线程池中工作的线程个数
	int threadPoolBusyNum();
	//获取线程池中活着的线程个数
	int threadPoolLiveNum();
private:
	//消费者线程
	static void* worker(void* arg);
	//管理者线程
	static void* manager(void* arg);
	//单个线程退出
	void threadExit();
private:
	//任务队列
	TaskQueue* taskQ;

	pthread_t managerID;   //管理者线程id
	pthread_t* threadIDs;  //工作的线程Id
	int minNum;     //最小线程数
	int maxNum;     //最大线程数
	int busyNum;    //工作的线程
	int liveNum;    //存活的线程
	int exitNum;    //要杀死的线程个数
	pthread_mutex_t mutexpool;   //锁整个线程池
	pthread_cond_t notEmpty;   //任务队列是否为空
	bool shutdown;                //是不是要销毁线程池，销毁为1，不销毁为0
	static const int NUMBER = 2;



};




