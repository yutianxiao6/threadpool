# threadpool
编写的一个c++版本的线程池，使用linux环境下的
一个线程池包含三个部分：
生产者-产生工作函数并把它添加到任务队列中
消费者-从任务队列中取出任务函数运行
管理者-在合适的时机对工作的线程进行销毁和创建
