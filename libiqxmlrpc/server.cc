#include <libiqxmlrpc/server.h>

using namespace iqxmlrpc;


Method_dispatcher::~Method_dispatcher()
{
  for( Factory_map::const_iterator i = fs.begin(); i != fs.end(); ++i )
    delete i->second;
}


void Method_dispatcher::register_method
  ( const std::string& name, Method_factory_base* fb )
{
  fs[name] = fb;
}


Method* Method_dispatcher::create_method( const std::string& name )
{
  if( fs.find(name) == fs.end() )
    return 0;

  return fs[name]->create();
}
