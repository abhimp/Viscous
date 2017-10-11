/*
 * This is an implementation of Viscous protocol.
 * Copyright (C) 2017  Abhijit Mondal
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


/*
 * AppStack.hh
 *
 *  Created on: 26-Jan-2017
 *      Author: abhijit
 */

#ifndef SRC_UTIL_APPSTACK_HPP_
#define SRC_UTIL_APPSTACK_HPP_
#include <common.h>
#include "Macros.h"

namespace util {

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

}  // namespace util

#endif /* SRC_UTIL_APPSTACK_HPP_ */
