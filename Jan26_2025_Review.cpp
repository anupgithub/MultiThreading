#if 0
#include <iostream>
#include <vector>
#include <queue>
#include <thread>
#include <functional>
#include <condition_variable>
#include <atomic>
#include <memory>
#include <mutex>
#include<semaphore>

using namespace std;

class res
{
public:
	bool isComplete;
};

class ThreadPool
{
public:
	/* to guard the queue of tasks */
	mutex m;
	condition_variable cv;

	condition_variable pendingTasks;
	atomic<int> pendingCount;
	
	int threadCount;

	queue<function<class res(void*)>> tasks;

	vector<thread> pool;


	//Does stop really need to be atomic here? 
	// if its getting accessed under all mutexes, it should be fine. 
	// but if some code accesses without the mutex, it would be a problem.
	// "Mutex: Acts as a memory barrier, ensuring visibility of stop changes." -> from deepSeek.

	bool stop = false;

	void workersThread()
	{
		while (true)
		{

			function<class res(void*)> task;
			{
				unique_lock<mutex> lk(m);
				cv.wait(lk, [this] {return tasks.size() != 0 || stop; });
				if (stop)
					return;
				task = tasks.front();
				tasks.pop();
			}
			cv.notify_one(); // what if this notification is lost.
				
			try
			{
				void* params = nullptr;
				task(params);
			}
			catch (exception e)
			{
				cout << e.what() << endl;
			}
			pendingCount.fetch_sub(1);
			pendingTasks.notify_one();
		}
	}

	ThreadPool(int _threadCount)
	{
		threadCount = _threadCount;
		for (int i = 0; i < threadCount; i++)
		{
			pool.push_back(thread(&ThreadPool::workersThread,this));
		}
	}

	void enqueue(function<class res(void*)>& task)
	{
		// Good call from DeepSeek. We don't need a enqueue throttling here.
		// We don't need condition variable here.
#if 0
		{
			unique_lock<mutex> lk(m);
			cv.wait(lk, [this] {return tasks.size() <= threadCount; });
			tasks.push(task);
		}
		cv.notify_one(); // --> what if this notification is lost?
#endif 
		{
			unique_lock<mutex> lk(m);
			tasks.push(task);
			pendingCount.fetch_add(1);
		}
		cv.notify_one();

	}

	void WaitAll()
	{
		/*
			Good call here from DeepSeek that we are resuing "cv" here
			this will result in lost notification if waitAll consumes the notification.
			Use different condition variable.
		*/ 
#if 0
		{
			unique_lock<mutex> lk(m);
			cv.wait(lk, [this] {return tasks.size()==0; });
			return;
		}
#endif

		{
			unique_lock<mutex> lk(m);
			pendingTasks.wait(lk, [this] {return pendingCount.load() == 0; });
			return;
		}



	}

	~ThreadPool()
	{
		stop = true;
		cv.notify_all();

		for (auto& t : pool)
		{
			t.join();
		}
	}

};

class res foo(void* param)
{
	cout << "foo called" << endl;
	class res r;
	r.isComplete = true;
	return r;
}

int main()
{
	ThreadPool p(3);
	
	function<class res(void*)> task = foo;

	p.enqueue(task);
	p.enqueue(task);
	p.enqueue(task);

	p.WaitAll();


	return 0;
}


#endif