#pragma once
#include <thread>
#include <vector>
#include <queue>
#include <functional>
#include <mutex>
#include <condition_variable>

class ThreadPool
{
public:
	template<typename T>
	class Queue
	{
	private:
		std::deque<T> queue;
		mutable std::mutex mutex;
		std::condition_variable cond;

	public:
		Queue() = default;
		Queue(const Queue<T>&) = delete; // 禁用复制
		Queue& operator=(const Queue<T>&) = delete; // 禁用赋值

		void push(T value)
		{
			{
				std::lock_guard<std::mutex> lock(mutex);
				queue.push_back(std::move(value));
			}
			cond.notify_one();
		}

		T pop()
		{
			std::unique_lock<std::mutex> lock(mutex);
			cond.wait(lock, [this] { return !queue.empty(); });
			T result = std::move(queue.front());
			queue.pop_front();
			return result;
		}

		T& front()
		{
			std::lock_guard<std::mutex> lock(mutex);
			if (queue.empty())
				throw std::runtime_error("Attempt to access front of empty queue");
			return queue.front();
		}

		const T& front() const
		{
			std::lock_guard<std::mutex> lock(mutex);
			if (queue.empty())
				throw std::runtime_error("Attempt to access front of empty queue");
			return queue.front();
		}

		bool empty() const
		{
			std::lock_guard<std::mutex> lock(mutex);
			return queue.empty();
		}

		size_t size() const
		{
			std::lock_guard<std::mutex> lock(mutex);
			return queue.size();
		}
	};

public:
	class Task
	{
	private:
		std::function<void()> function;
		bool done = false;
		bool running = false;

	public:
		Task(const std::function<void()>& function) : function(function) {}

		inline void Run()
		{
			running = true;
			function();
			done = true;
			running = false;
		}

		inline bool IsDone() const
		{
			return done;
		}

		inline bool IsRunning() const
		{
			return running;
		}
	};

private:
	class Thread
	{
	public:
		std::thread thread;
		bool running = false;
		bool done = false;
		ThreadPool* pool = nullptr;

		Thread(ThreadPool* pool) : pool(pool) {}

		inline void Start()
		{
			running = true;
			thread = std::thread([this]()
				{
					while (running)
					{
						Task* task = nullptr;

						{
							std::unique_lock<std::mutex> lock(pool->queueMutex);
							// Wait until there is a task to execute
							pool->taskAvailable.wait(lock, [this]() { return !pool->tasks.empty() || !running; });

							if (!running)
								break;

							if (!pool->tasks.empty())
							{
								task = pool->tasks.front();
								pool->tasks.pop();
							}
						}

						if (task)
						{
							task->Run();
							delete task;
						}
						std::this_thread::sleep_for(std::chrono::milliseconds(1));
					}
					done = true;
				});
		}

		inline bool IsRunning() const
		{
			return running;
		}

		inline bool IsDone() const
		{
			return done;
		}
	};

	const size_t threadCount = 0;
	Queue<Task*> tasks = {};
	std::vector<Thread*> threads = {};
	std::mutex queueMutex = {};
	std::condition_variable taskAvailable = {};

public:
	ThreadPool() {}
	ThreadPool(const int& threadCount) : threadCount(threadCount) {}

	~ThreadPool()
	{
		for (auto thread : threads)
		{
			thread->running = false;
			taskAvailable.notify_all(); // Notify all threads to exit
		}

		for (auto thread : threads)
		{
			thread->thread.join();
			delete thread;
		}
	}

	inline void Start()
	{
		for (int i = 0; i < threadCount; i++)
		{
			Thread* thread = new Thread(this);
			thread->Start();
			threads.push_back(thread);
		}
	}

	inline void AddTask(const std::function<void()>& function)
	{
		Task* task = new Task(function);

		{
			std::lock_guard<std::mutex> lock(queueMutex);
			tasks.push(task);
		}

		taskAvailable.notify_one(); // Notify one waiting thread that a task is available
	}

	inline void Wait()
	{
		std::lock_guard<std::mutex> lock(queueMutex);
		while (!tasks.empty())
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
		}
	}

	inline size_t ActiveThreadCount() const
	{
		size_t count = 0;
		for (auto thread : threads)
		{
			if (thread->IsRunning())
				count++;
		}
		return count;
	}

	inline size_t ThreadCount() const
	{
		return threadCount;
	}

	inline bool IsAllTasksFinish()
	{
		std::lock_guard<std::mutex> lock(queueMutex);
		return tasks.empty();
	}
};