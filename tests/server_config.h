#ifndef _iqxmlrpc_test_server_config_h_
#define _iqxmlrpc_test_server_config_h_

#include <stdexcept>
#include "libiqxmlrpc/value.h"

namespace iqxmlrpc {
  class Executor_factory_base; 
}

//! Test server configuration structure
struct Test_server_config {
  class Malformed_config;
  class Malformed_cmd_line;
  class Malformed_xmlrpc_arg;
  
  // not owned by this structure
  iqxmlrpc::Executor_factory_base* exec_factory;
  bool use_ssl;
  int port;

  Test_server_config():
    exec_factory(0), use_ssl(false), port(0) {}

  ~Test_server_config();

  static Test_server_config create(int argc, char** argv);
  static Test_server_config create(const iqxmlrpc::Value&);
};

class Test_server_config::Malformed_config: public std::runtime_error {
public:
  Malformed_config(const std::string& usage):
    runtime_error(usage) {}
};

class Test_server_config::Malformed_cmd_line: 
  public Test_server_config::Malformed_config 
{
public:
  Malformed_cmd_line();
};

class Test_server_config::Malformed_xmlrpc_arg: 
  public Test_server_config::Malformed_config
{
public:
  Malformed_xmlrpc_arg();
};

#endif
