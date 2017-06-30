/*
 * This is an implemetation of Viscous protocol.
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
 * ValueType.cc
 *
 *  Created on: 08-Dec-2016
 *      Author: abhijit
 */
#include "../InterfaceController/ValueType.hh"
#include <netinet/ether.h>
#include <arpa/inet.h>

Value::Value(){

}

Value::~Value(){

}


std::ostream& operator<< (std::ostream& os, Value *v){
    os << v->toString();
    return os;
}

std::istream& operator>> (std::istream& is, Value *v){
    std::string str;
    is >> str;
    v->fromString(str);
    return is;
}

VALUE_IMPLEMENT(int);

std::ostream& operator<< (std::ostream& os, ether_addr e_addr){
    os << ether_ntoa(&e_addr);
    return os;
}

std::istream& operator>> (std::istream& os, ether_addr e_addr){

    return os;
}


std::ostream& operator<< (std::ostream& os, in_addr i_addr){
    os << inet_ntoa(i_addr);
    return os;
}

std::istream& operator>> (std::istream& os, in_addr i_addr){

    return os;
}




VALUE_IMPLEMENT_WITH_NAME(ether_addr, etherAddr);
VALUE_IMPLEMENT_WITH_NAME(in_addr, inAddr);

VALUE_IMPLEMENT(appString);
