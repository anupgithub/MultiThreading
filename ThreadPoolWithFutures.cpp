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
#include<future>

using namespace std;

class ThreadState
{
public:
	function<void(void*)> f;
	void* data;
	promise<bool> pr;
	future<bool> ft;

	ThreadState()
	{
		ft = pr.get_future();
	}

	ThreadState(function<void(void*)>& _f, void* _data)
	{
		ft = pr.get_future();
		f = _f;
		data = _data;
	}

	future <bool>& get_Threadfuture()
	{
		return ft;
	}
};



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

	queue<ThreadState*> tasks;

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

			ThreadState* ts = nullptr;
			{
				unique_lock<mutex> lk(m);
				cv.wait(lk, [this] {return tasks.size() != 0 || stop; });
				if (stop)
					return;
				ts = tasks.front();
				tasks.pop();
			}
			cv.notify_one(); // what if this notification is lost.

			try
			{
				void* params = nullptr;
				ts->f(ts->data);
			}
			catch (exception e)
			{
				cout << e.what() << endl;
			}

			pendingCount.fetch_sub(1);
			pendingTasks.notify_one();

			ts->pr.set_value(true);

		}
	}

	ThreadPool(int _threadCount)
	{
		threadCount = _threadCount;
		for (int i = 0; i < threadCount; i++)
		{
			pool.push_back(thread(&ThreadPool::workersThread, this));
		}
	}

	future<bool>& enqueue(function<void(void*)>& task, void* data)
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
		class ThreadState* ts = new ThreadState;
		ts->f = task;
		ts->data = data;
		{
			unique_lock<mutex> lk(m);
			tasks.push(ts);
			pendingCount.fetch_add(1);
		}
		cv.notify_one();
		
		return ts->ft;
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
			cv.wait(lk, [this] {return tasks.size() == 0; });
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

void foo(void* param)
{
	std::this_thread::sleep_for(std::chrono::seconds(4));
	cout << "foo called" << endl;
}

int main()
{
	ThreadPool p(3);

	function<void(void*)> task = foo;
	void* data = (void*)new int[10];

	vector<future<bool>> futures;
	/*
	Only MOVE will work as futures are not copyable.!
	*/
	futures.emplace_back(std::move(p.enqueue(task, data)));
	futures.emplace_back(std::move(p.enqueue(task, data)));
	futures.emplace_back(std::move(p.enqueue(task, data)));

	bool allcomplete = false;
	while (!allcomplete)
	{
		cout << "Not Ready " << endl;
		for (int i = 0; i < futures.size(); i++)
		{
			if (futures[i].wait_for(chrono::milliseconds(100)) != future_status::ready)
				break;

			allcomplete = true;
		}
		
	}


	p.WaitAll();


	return 0;
}


