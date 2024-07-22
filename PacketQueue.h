#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>


#define MAX_QUEUE_SIZE 10
template<class T>
class PacketQueue {

public:
	PacketQueue() {}
	~PacketQueue() {}

public:
	void push(T t)
	{
		std::unique_lock m(m_);
		if ((write_index_ - read_index_) > MAX_QUEUE_SIZE)
		{
			cv_.wait(m, [&]() {
				return (write_index_ - read_index_) < MAX_QUEUE_SIZE;
				});
		}
		pkt_.push(t);
		++write_index_;
		cv_.notify_one();
	}

	bool empty() {
		std::lock_guard m(m_);
		return pkt_.empty();
	}

	T pop()
	{
		std::unique_lock lock(m_);
		if (pkt_.empty())
		{
			cv_.wait(lock, [&]() {
				return !pkt_.empty();
				});
		}
		T t = pkt_.front();
		++read_index_;
		pkt_.pop();
		if ((write_index_ - read_index_) < MAX_QUEUE_SIZE)
			cv_.notify_one();
		return t;
	}


private:
	std::queue<T> pkt_;
	std::mutex m_;
	std::condition_variable cv_;
	int read_index_;
	int write_index_;
};
