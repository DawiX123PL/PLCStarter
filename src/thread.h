#pragma once

#include <thread>
#include <mutex>


class Thread {
	std::thread* thread;
	std::mutex stopMutex;
	bool stopFlag;


public:
	Thread() :thread(nullptr), stopFlag(false) {}
	~Thread() { if (thread) delete thread; }

	void Start() {
		thread = new std::thread(&Thread::threadJob, this);
	}


	void Stop() {
		stopMutex.lock();
		stopFlag = true;
		stopMutex.unlock();
	}


	bool IsRun() {
		stopMutex.lock();
		bool isRun = !stopFlag;
		stopMutex.unlock();
		return isRun;
	}

	void Join() {
		thread->join();
	}

private:
	virtual void threadJob() = 0;

};

