/*
 * CircularBuffer.hpp
 *
 *  Created on: 13-Aug-2016
 *      Author: abhijit
 */

#ifndef SRC_UTIL_CIRCULARBUFFER_HPP_
#define SRC_UTIL_CIRCULARBUFFER_HPP_
#include <common.h>
#include <algorithm> // for std::min
#include <cstring>
#include <mutex>
#include "ConditonalWait.hpp"
//#include <pthread.h>

template<typename T>
class CircularBuffer {
public:
	CircularBuffer(size_t capacity);
	~CircularBuffer();

	size_t size() {
		return size_;
	}
	size_t capacity() {
		return capacity_;
	}
	// Return number of bytes written.
	size_t write(T *data, size_t bytes);
	// Return number of bytes read.
	size_t read(T *data, size_t bytes);
	void reset();
	void setEoF();
	appBool closed(){return eof_;};
private:
	size_t beg_index_, size_, capacity_;
	appBool eof_;
	T *data_;
	std::mutex lock; // for thread safety
	ConditionalWait waitToRead;
//  appByte *tmp_data_;
};

template<typename T>
CircularBuffer<T>::CircularBuffer(size_t capacity) :
		beg_index_(0)
		, size_(0)
		, capacity_(capacity)
		, eof_(FALSE){
	if(capacity_ < 2048) capacity_ = 2048;
	data_ = new T[capacity_];
//	pthread_mutex_init(&lock, 0);
}

template<typename T>
CircularBuffer<T>::~CircularBuffer() {
	delete[] data_;
}

template<typename T>
size_t CircularBuffer<T>::write(T *data, size_t bytes) {
	if (bytes == 0 || !data)
		return 0;
	APP_ASSERT(!eof_);
	T *tmp = NULL;
	lock.lock();

	// Write in a single step
	if (beg_index_ + size_ + bytes < capacity_) { //simple. just copy it and update parameter
		std::memcpy(data_ + beg_index_ + size_, data, bytes * sizeof(T));
		size_ += bytes;
		goto RETURN_BYTES;
	}

	if (size_ + bytes < capacity_) { //we dont have allocate new data but need to move
		std::memcpy(data_, data_ + beg_index_, size_ * sizeof(T));
		std::memcpy(data_ + size_, data, bytes * sizeof(T));
		size_ += bytes;
		beg_index_ = 0;
		goto RETURN_BYTES;
	}

	tmp = data_;
	while (capacity_ < size_ + bytes)
		capacity_ *= 2;
	data_ = new T[capacity_];
	std::memmove(data_, tmp + beg_index_, size_ * sizeof(T));
	std::memcpy(data_ + size_, data, bytes * sizeof(T));
	beg_index_ = 0;
	size_ += bytes;
	delete[] tmp;
RETURN_BYTES:
//	pthread_mutex_unlock(&lock);
    waitToRead.notify();
    lock.unlock();
	return bytes;
}

template<typename T>
void CircularBuffer<T>::setEoF(){
	eof_ = TRUE;
	waitToRead.notify();
}

template<typename T>
void CircularBuffer<T>::reset(){
//	pthread_mutex_lock(&lock);
    lock.lock();
	size_ = 0;
	beg_index_ = 0;
//	pthread_mutex_unlock(&lock);
	lock.unlock();
}

template<typename T>
size_t CircularBuffer<T>::read(T *data, size_t bytes) {
	if (bytes == 0 || !data)
		return 0;
	if(eof_ and size_ == 0)
	    return -1;
	size_t lenRead;
//	pthread_mutex_lock(&lock);
	lock.lock();
	while(size_ == 0 and !eof_){ //while give just for safety. if is good enough
	    lock.unlock();
	    waitToRead.wait();
	    lock.lock();
	}
	lenRead = size_ < bytes ? size_ : bytes;
	std::memcpy(data, data_ + beg_index_, lenRead * sizeof(T));
	size_ -= lenRead;
	beg_index_ += lenRead;
	//Done free space when there are less data
	if((capacity_ >> 2) > size_ && (capacity_>>1) > 2048) //i.e. if size become 25% of capacity
	{
//		LOGD("Decreasing length");
		T *tmp = new T[capacity_/2];
		std::memcpy(tmp, data_ + beg_index_, size_*sizeof(T));
		beg_index_ = 0;
		delete[] data_;
		data_ = tmp;
		capacity_ /= 2;
	}
//	pthread_mutex_unlock(&lock);
	lock.unlock();
	return lenRead;
}

#endif /* SRC_UTIL_CIRCULARBUFFER_HPP_ */
