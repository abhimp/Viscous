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
 * BinaryHeap.hh
 *
 *  Created on: 13-Feb-2017
 *      Author: abhijit
 */

#ifndef SRC_UTIL_BINARYHEAP_HPP_
#define SRC_UTIL_BINARYHEAP_HPP_
//#include <set>
#include <common.h>
namespace util{
template<typename key, typename data=void*>
struct less {
    bool operator() (const key& lhs, const key& rhs, const data dt) const
    {return lhs<rhs;}
  };

template<typename key, typename comp=less<key>, typename data=void*>
struct BinaryHeap{
    key *heapList_;
    int capa_, nmem_;
    data dt;
    comp compare_;
    BinaryHeap(int capa, data dt);
    void percUp(int index);
    void insert(key k);
    void percDown(int index);
    int minChild(int index);
    key delMin();
//    void buildHeap()
};

template<typename key, typename comp, typename data>
inline BinaryHeap<key, comp, data>::BinaryHeap(int capa, data dt) : heapList_(0), capa_(capa), nmem_(0), dt(dt), compare_() {
    heapList_ = new key[capa+1];
}

template<typename key, typename comp, typename data>
inline void BinaryHeap<key, comp, data>::percUp(int index) {
    int parent = index / 2;
    while(parent > 0) // index/2 > 0
    {
        if(compare_(heapList_[index], heapList_[parent])){
            key tmp = heapList_[parent];
            heapList_[parent] = heapList_[index];
            heapList_[index] = tmp;
        }
        index = parent;
    }
}

template<typename key, typename comp, typename data>
inline void BinaryHeap<key, comp, data>::insert(key k) {
    APP_ASSERT(nmem_ <= capa_);
    heapList_[nmem_] = k;
    nmem_ ++;
    percUp(nmem_);
}

template<typename key, typename comp, typename data>
inline void BinaryHeap<key, comp, data>::percDown(int index) {
    while((index * 2) <= nmem_)
    {
        int mc = minChild(index);
        if(compare_(heapList_[mc], heapList_[index]))
        {
            key tmp = heapList_[index];
            heapList_[index] = heapList_[mc];
            heapList_[mc] = tmp;
        }
        index = mc;
    }
}

template<typename key, typename comp, typename data>
inline int BinaryHeap<key, comp, data>::minChild(int index) {
    if ((index*2 + 1) > nmem_)
        return index*2;
    if (compare_(heapList_[index*2], heapList_[index*2 + 1]))
        return index*2;
    return index*2 + 1;
}
}
#endif /* SRC_UTIL_BINARYHEAP_HPP_ */
