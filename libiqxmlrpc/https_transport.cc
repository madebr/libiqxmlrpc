#include <libiqxmlrpc/https_transport.h>

using namespace iqxmlrpc;
using namespace iqnet;


Https_reaction_connection::Https_reaction_connection( 
    int fd, 
    const iqnet::Inet_addr& addr, 
    ssl::Ctx* ctx 
  ):
    ssl::Reaction_connection( fd, addr, ctx ),
    server(0),
    response(0),
    recv_buf_sz(256),
    recv_buf(new char[recv_buf_sz]),
    send_buf(0)
{
}


Https_reaction_connection::~Https_reaction_connection()
{
  delete[] send_buf;
  delete[] recv_buf;
  delete response;
  delete server;
}


inline void Https_reaction_connection::my_reg_recv()
{
  bzero( recv_buf, recv_buf_sz );
  reg_recv( recv_buf, recv_buf_sz-1 );  
}


void Https_reaction_connection::accept_succeed()
{
  my_reg_recv();
}


void Https_reaction_connection::recv_succeed
  ( bool& terminate, int req_len, int real_len )
{
  try 
  {
    if( !real_len )
    {
      terminate = true;
      return;
    }
    
    if( !server->read_request( std::string(recv_buf, real_len) ) )
    {
      my_reg_recv();
      return;
    }

    response = server->execute();
  }
  catch( const http::Error_response& e )
  {
    delete response;
    response = new http::Packet(e);
  }
  
  std::string s( response->dump() );
  send_buf = new char[s.length()+1];
  s.copy( send_buf, std::string::npos );
  reg_send( send_buf, s.length() );
}


void Https_reaction_connection::send_succeed( bool& terminate )
{
  delete[] send_buf;
  send_buf = 0;
  delete response;
  response = 0;
  terminate = true;
}


// --------------------------------------------------------------------------
Https_server::Https_server( int port, ssl::Ctx* ctx, Method_dispatcher* disp ):
  reactor(),
  acceptor(0),
  cfabric( new C_fabric( ctx, disp, &reactor ) ),
  exit_flag(false) 
{
  acceptor = new Acceptor( port, cfabric, &reactor );
}


Https_server::~Https_server()
{
  delete acceptor;
  delete cfabric;
}


void Https_server::work()
{
  while( !exit_flag )
    reactor.handle_events();
}