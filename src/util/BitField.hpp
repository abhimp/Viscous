/*
 * BitField.hpp
 *
 *  Created on: 15-Feb-2017
 *      Author: abhijit
 */

#ifndef SRC_UTIL_BITFIELD_HPP_
#define SRC_UTIL_BITFIELD_HPP_
#include <common.h>
struct BitField{
    appByte *buffer;
    appInt size;
    BitField(appInt asize){
        appInt actSize = (asize + 7)/8;
        buffer = new appByte[actSize];//[asize/8 + ((asize%8)==0?0:1)];
        memset(buffer, 0, actSize);
        size = asize;
    }
    ~BitField(){
        delete[] buffer;
    }
    inline appByte getBit(appInt pos){
        APP_ASSERT(pos < size);
        appInt index = pos/8;
        appInt offset = pos%8;
        return buffer[index]&offsetValue[offset];
    }
    inline void setBit(appInt pos){
        APP_ASSERT(pos < size);
        appInt index = pos/8;
        appInt offset = pos%8;
        buffer[index] |= offsetValue[offset];
    }
    inline void resetBit(appInt pos){
        APP_ASSERT(pos < size);
        appInt index = pos/8;
        appInt offset = pos%8;
        buffer[index] &= (appByte)~(offsetValue[offset]);
    }
    inline appByte operator[](appInt pos){
        APP_ASSERT(pos < size);
        return getBit(pos);
    }
private:
    appByte offsetValue[8] = {0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01};
};




#endif /* SRC_UTIL_BITFIELD_HPP_ */
