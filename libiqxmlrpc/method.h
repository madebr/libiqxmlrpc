//  Libiqnet + Libiqxmlrpc - an object-oriented XML-RPC solution.
//  Copyright (C) 2004 Anton Dedov
//  
//  This library is free software; you can redistribute it and/or
//  modify it under the terms of the GNU Lesser General Public
//  License as published by the Free Software Foundation; either
//  version 2.1 of the License, or (at your option) any later version.
//  
//  This library is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
//  Lesser General Public License for more details.
//  
//  You should have received a copy of the GNU Lesser General Public
//  License along with this library; if not, write to the Free Software
//  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307  USA
//  
//  $Id: method.h,v 1.17 2006-02-24 09:40:59 bada Exp $

#ifndef _iqxmlrpc_method_h_
#define _iqxmlrpc_method_h_

#include <string>
#include <map>
#include <boost/utility.hpp>
#include "value.h"
#include "except.h"
#include "inet_addr.h"

namespace iqxmlrpc
{
class Server;
class Interceptor;
class Method;

//! Method's parameters type
typedef std::vector<Value> Param_list;

//! A type of a pointer to a function that can be wrapped
//! with Method_function_adapter class.
typedef void (*Method_function)(Method*, const Param_list&, Value&);

//! This clas provides restricted interface of class Server for Method's needs.
class Server_feedback {
  Server* server_;

public:
  // Do not use objects constructed with default ctor!
  Server_feedback():
    server_(0) {}

  Server_feedback(Server* s):
    server_(s) {}

  void set_exit_flag();
  void log_message( const std::string& );
};

//! Abstract base for server method. 
//! Inherit it to create actual server method.
class Method {
public:
  struct Data {
    std::string      method_name;
    iqnet::Inet_addr peer_addr;
    Server_feedback  server_face;
  };

  //! Introspection interface class. 
  /*! Create appropriate nested class for each of your method classes
      if you want to support an introspection in your appliaction.
  */
  class Help {
  public:
    virtual ~Help() {}
    //! Returns methods signature
    virtual Value signature() const { return iqxmlrpc::Nil(); }
    //! Returns methods help string
    virtual std::string help() const { return ""; }
  };

private:
  friend class Method_dispatcher;
  Data data_;

public:
  virtual ~Method() {}

  //! Calls customized execute() and optionally wraps it with interceptors.
  //! Is is called by a server object.
  void process_execution(Interceptor*, const Param_list& params, Value& response);

  const std::string&      name()      const { return data_.method_name; }
  const iqnet::Inet_addr& peer_addr() const { return data_.peer_addr; }
  Server_feedback&        server()          { return data_.server_face; }

private:
  //! Replace it with your actual code.
  virtual void execute( const Param_list& params, Value& response ) = 0;
};

//! Interceptor's base class
class Interceptor: boost::noncopyable {
public:
  Interceptor():
    nested(0) {}

  virtual ~Interceptor()
  {
    delete nested;
  }

  void nest(Interceptor* ic)
  {
    delete nested;
    nested = ic;
  }

  //! Customize this method. Use yield() method to pass the execution.
  //! Note: interceptor objects are shared beetwen methods and even threads.
  virtual void process(Method*, const Param_list&, Value&) = 0;

protected:
  void yield(Method* m, const Param_list& params, Value& result)
  {
    m->process_execution(nested, params, result);
  }

private:
  Interceptor* nested;
};

//! The method class that wraps the call of function specified in ctor
class Method_function_adapter: public Method {
public:
  Method_function_adapter(Method_function f):
    function(f) {}

  using Method::name;
  using Method::peer_addr;
  using Method::server;

private:
  void execute(const Param_list& params, Value& result)
  {
    function(this, params, result);
  }

  Method_function function;
};

//! Abstract factory for Method. 
/*! Method_dispatcher uses it to create Method object on demand. 
    Inherit it to create your specific factory.
    \see Method_factory
*/
class Method_factory_base {
public:
  virtual ~Method_factory_base() {}
    
  virtual Method* create() = 0;
};
  

//! Template for simple Method factory.
template <class T>
class Method_factory: public Method_factory_base {
public:
  T* create() { return new T(); }
};

//! Factory specialization. Creates funciton adapter methods.
template <>
class Method_factory<Method_function_adapter>: public Method_factory_base {
public:
  Method_factory(Method_function fn):
    function(fn) {}

  Method* create() { return new Method_function_adapter(function); }

private:
  Method_function function;
};

//! Method dispatcher.
/*! This class responsible for methods dispatching by their names.
    User must register his own methods in Method_dispatcher.
    \code
    // Usage example:
    class MyMethod;
    Method_dispatcher disp;
    disp.register_method( "my_method", new Method_factory<MyMethod> );
    \endcode
*/
class Method_dispatcher {
  typedef std::map<std::string, Method_factory_base*> Factory_map;

  Server* server;
  Factory_map fs;
  
public:
  Method_dispatcher( Server* );
  virtual ~Method_dispatcher();

  //! Register Method with its factory.
  /*! Method_dispatcher owns factory object. */
  void register_method( const std::string& name, Method_factory_base* );

  //! Create method object according to specified name. 
  //! Returns 0 if no Method registered for name.
  Method* create_method( const std::string& name, const iqnet::Inet_addr& peer );
};

} // namespace iqxmlrpc

#endif
