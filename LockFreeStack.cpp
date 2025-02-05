#if 1
#include <iostream>
#include <thread>
#include <queue>
#include <chrono>
#include <vector>
#include <Windows.h>
#include <mutex>
using namespace std;

/*
 
Fixed sized with dynamic allocation. 

*/
class LockFreeStack
{
	class Node
	{
	public:
		int data;
		atomic<Node*> next; //not needed atomic here in Lock Free Stack.
		Node(int _data) : data(_data) , next(nullptr) {}
	};

	atomic<Node*> _stackHead;
	Node* dummy = nullptr;
	atomic<int> stackSize;
	int capacity;

	LockFreeStack(int maxsize)
	{
		dummy = new Node(-1);
		_stackHead.store(dummy);
	}

	bool push(int data)
	{
		if (stackSize.fetch_add(1) >= capacity)
		{
			stackSize.fetch_sub(1);
			return false;
		}

		Node* n = new Node(data);

		while (true)
		{
			Node* oldhead = _stackHead.load();
			n->next = oldhead;
			if (_stackHead.compare_exchange_strong(oldhead, n))
				return;
		}
	}

	int pop()
	{
		while (true)
		{
			Node* n = _stackHead.load();

			if (n==nullptr || n == dummy)
				return -1;

			if (_stackHead.compare_exchange_strong(n, n->next))
			{
				int res = n->data;
				delete n;
				stackSize.fetch_sub(1);
				return res;
			}
		}
	}

};




#endif
