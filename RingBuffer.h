#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <atomic>
#include <iostream>

class RingBuffer {
public:
	RingBuffer(int size):capacity_(size)
	{
		buffer_ = new char[capacity_];
		memset(buffer_, 0, capacity_);
		w_size_ = 0;
		r_size_ = 0;
	}
	~RingBuffer() {}

	int write(const char* src, int len) {
		auto s = idle_size();
		if (w_size_ == 256)
		{
			int kk = 0;
		}
		s = s > len ? len : s;
		memcpy(buffer_ + write_index(), src, s);
		w_size_ += s;
		return s;
	}
	int read(char* dst, int len) {
		if (size() > len)
		{
			if ((capacity_ - read_index()) >= len)
			{
				memcpy(dst, buffer_ + read_index(), len);
			}
			else
			{
				memcpy(dst, buffer_ + read_index(), capacity_ - read_index());
				memcpy(dst + capacity_ - read_index(), buffer_, len - (capacity_ - read_index()));
			}
			r_size_ += len;
		}
		else
		{
			len = -1;
			std::cout << "data buffer empty" << std::endl;
		}
		return len;
	}
	bool empty() { return w_size_ == r_size_; }

	int size() {
		if (w_size_ >= r_size_)
		{
			return w_size_ - r_size_;
		}
		else
		{
			return capacity_ - r_size_ + w_size_;
		}
	}

private:
	int write_index() {
		return w_size_ % capacity_;
	}
	int read_index() {
		return r_size_ % capacity_;
	}

	int idle_size() {
		return capacity_ - size();
	}


private:
	char* buffer_;
	int capacity_;
	std::atomic<int> w_size_;
	std::atomic<int> r_size_;
};

#endif // RING_BUFFER_H
