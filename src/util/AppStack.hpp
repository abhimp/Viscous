/*
 * AppStack.hpp
 *
 *  Created on: 26-Jan-2017
 *      Author: abhijit
 */

#ifndef SRC_UTIL_APPSTACK_HPP_
#define SRC_UTIL_APPSTACK_HPP_
#include <common.h>
#include "Macros.h"

template<typename T>
class AppStack
{
private:
    T *p_;
    appSInt32 top_,length_;

public:
    AppStack(int size = 0);
    ~AppStack();

    inline void push(T);
    inline void pop();
    inline T top();
    inline bool empty();
};

template<typename T>
AppStack<T>::AppStack(int size):p_(NULL), top_(-1), length_(size)
{
    if(size == 0)
        p_ = 0;
    else
        p_= (T *)appMalloc(length_ * sizeof(T));
}

template<typename T>
AppStack<T>::~AppStack()
{
    appCondFree(p_);
}

template<typename T>
void AppStack<T>::push(T elem)
{
    if(top_==(length_-1))     //If the top reaches to the maximum stack size
    {
        length_ = length_ == 0 ? 256 : length_ * 2;
        p_ = (T *)appRealloc(p_, length_ * sizeof(T));
    }
    top_++;
    p_[top_]=elem;
}

template<typename T>
void AppStack<T>::pop()
{
    APP_ASSERT(top_ != -1);
    top_--;
}

template<typename T>
T AppStack<T>::top()
{
    APP_ASSERT(top_ != -1);
    T ret=p_[top_];
    return ret;
}

template<typename T>
bool AppStack<T>::empty()
{
    return (top_ <= -1);
}
#endif /* SRC_UTIL_APPSTACK_HPP_ */
