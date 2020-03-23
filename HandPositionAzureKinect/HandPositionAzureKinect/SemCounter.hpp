#include <mutex>
#include <chrono>

class SemCounter
{
private:
	int _val;
	std::mutex _mutex;
	std::mutex _mutex_block;

	void block();
	void unblock();

public:
	SemCounter(int value);
	~SemCounter();
	void wait();
	void signal();
	int getValue();
};


SemCounter::SemCounter(int value) : _val(value) {};
SemCounter::~SemCounter() {};

void SemCounter::unblock() {
	this->_mutex_block.unlock();
}

void SemCounter::block() {
	this->_mutex_block.lock();
}


void SemCounter::wait() {
	this->_mutex.lock();
	if (--_val <= 0) {
		this->_mutex.unlock();
		block();
		this->_mutex.lock();
	}
	this->_mutex.unlock();
}

void SemCounter::signal() {
	this->_mutex.lock();
	if (++_val <= 0) {
		this->_mutex.unlock();
		unblock();
	//	std::this_thread::sleep_for(std::chrono::milliseconds(300));
		this->_mutex.lock();
	}
	this->_mutex.unlock();
}

int SemCounter::getValue() {
	return _val;
}


