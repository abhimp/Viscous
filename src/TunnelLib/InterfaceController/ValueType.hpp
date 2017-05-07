/*
 * ValueType.hpp
 *
 *  Created on: 08-Dec-2016
 *      Author: abhijit
 */

#ifndef VALUETYPE_HPP_
#define VALUETYPE_HPP_
#include <string>
#include <cassert>
#include <sstream>
#include <net/ethernet.h>
#include <netinet/in.h>
#include <common.h>

class Value{
public:
    Value(void);
    virtual ~Value(void);
    virtual std::string toString(void) const = 0;
    virtual bool fromString(std::string value) = 0;
//    virtual void get(void) = 0;
};


std::ostream& operator<< (std::ostream& os, Value *v);
std::istream& operator>> (std::istream& is, Value *v);

#define VALUE_DEFINE_WITH_NAME(type,name)                               \
  class name ## Value : public Value                                    \
  {                                                                     \
  public:                                                               \
    name ## Value ();                                                   \
    name ## Value (const type &value);                                  \
    void set (const type &value);                                       \
    type get (void) const;                                              \
    virtual std::string                                                 \
      toString (void) const;                                            \
    virtual bool                                                        \
      fromString (std::string value);                                   \
  private:                                                              \
    type m_value;                                                       \
  };

#define VALUE_DEFINE(Name)                                              \
  VALUE_DEFINE_WITH_NAME (Name,Name)

#define VALUE_IMPLEMENT_WITH_NAME(type,name)                            \
  name ## Value::name ## Value ()                                       \
    : m_value () {}                                                     \
  name ## Value::name ## Value (const type &value)                      \
    : m_value (value) {}                                                \
  void name ## Value::set (const type &v) {                             \
    m_value = v;                                                        \
  }                                                                     \
  type name ## Value::get (void) const {                                \
    return m_value;                                                     \
  }                                                                     \
  std::string name ## Value::toString                                   \
    (void) const {                                                      \
      std::ostringstream oss;                                           \
      oss << m_value;                                                   \
      return oss.str ();                                                \
  }                                                                     \
  bool name ## Value::fromString                                        \
    (std::string value) {                                               \
      std::istringstream iss;                                           \
      iss.str (value);                                                  \
      iss >> m_value;                                                   \
      assert (!iss.eof () && "Attribute value "  "\""  #name            \
                    "\"" " is not properly formatted");                 \
      return !iss.bad () && !iss.fail ();                               \
  }                                                                     \


#define VALUE_IMPLEMENT(type)                                           \
  VALUE_IMPLEMENT_WITH_NAME (type,type)




//Define different value type
VALUE_DEFINE(int);

std::ostream& operator<< (std::ostream& os, ether_addr e_addr);
std::istream& operator>> (std::istream& os, ether_addr e_addr);
std::ostream& operator<< (std::ostream& os, in_addr i_addr);
std::istream& operator>> (std::istream& os, in_addr i_addr);

VALUE_DEFINE_WITH_NAME(ether_addr, etherAddr);
VALUE_DEFINE_WITH_NAME(in_addr, inAddr);

VALUE_DEFINE(appString);


#endif /* VALUETYPE_HPP_ */
