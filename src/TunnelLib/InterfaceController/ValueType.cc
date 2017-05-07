/*
 * ValueType.cc
 *
 *  Created on: 08-Dec-2016
 *      Author: abhijit
 */
#include "../InterfaceController/ValueType.hpp"
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
