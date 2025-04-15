#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <atomic>
#include <iostream>
#include <mutex>
#include <condition_variable>

class RingBuffer {
public:
    RingBuffer(int size) : capacity_(size), buffer_(new char[size]), w_size_(0), r_size_(0) {
        memset(buffer_, 0, capacity_);
    }

    ~RingBuffer() {
        delete[] buffer_;
    }

    int write(const char* src, int len, bool wait = true) {
        std::unique_lock<std::mutex> lock(mutex_);
        
        if (wait) {
            not_full_.wait(lock, [this, len]() 
                {
                    return idle_size() >= len; 
                }
            );
        } else if (idle_size() < len) {
            return -1;
        }

        int write_len = std::min(len, idle_size());
        int first_part = std::min(write_len, capacity_ - write_index());
        memcpy(buffer_ + write_index(), src, first_part);

        if (first_part < write_len) {
            memcpy(buffer_, src + first_part, write_len - first_part);
        }

        w_size_ += write_len;
        not_empty_.notify_one();
        return write_len;
    }

    int read(char* dst, int len, bool wait = true) {
        std::unique_lock<std::mutex> lock(mutex_);

        if (wait) {
            not_empty_.wait(lock, [this, len]() { return size() >= len; });
        } else if (size() < len) {
            return -1;
        }

        int first_part = std::min(len, capacity_ - read_index());
        memcpy(dst, buffer_ + read_index(), first_part);

        if (first_part < len) {
            memcpy(dst + first_part, buffer_, len - first_part);
        }

        r_size_ += len;
        not_full_.notify_one();
        return len;
    }

    bool empty() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return w_size_ == r_size_;
    }

    int size() const {
        //std::lock_guard<std::mutex> lock(mutex_);
        return (w_size_ >= r_size_) ? 
            (w_size_ - r_size_) : 
            (capacity_ - r_size_ + w_size_);
    }

    int capacity() const { return capacity_; }

private:
    int write_index() const { return w_size_ % capacity_; }
    int read_index() const { return r_size_ % capacity_; }
    int idle_size() const { return capacity_ - size(); }

private:
    const int capacity_;
    char* const buffer_;
    std::atomic<int> w_size_;
    std::atomic<int> r_size_;
    mutable std::mutex mutex_;
    std::condition_variable not_full_;
    std::condition_variable not_empty_;
};

#endif // RING_BUFFER_H
