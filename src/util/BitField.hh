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
 * BitField.hh
 *
 *  Created on: 15-Feb-2017
 *      Author: abhijit
 */

#ifndef SRC_UTIL_BITFIELD_HPP_
#define SRC_UTIL_BITFIELD_HPP_
#include <common.h>
namespace util {

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

}  // namespace util





#endif /* SRC_UTIL_BITFIELD_HPP_ */
