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
//  $Id: http.cc,v 1.16 2004-03-29 06:23:18 adedov Exp $

#include <time.h>
#include <locale.h>
#include <ctype.h>
#include <iostream>
#include <functional>
#include <memory>
#include <libiqxmlrpc/http.h>
#include <libiqxmlrpc/method.h>

using namespace iqxmlrpc::http;

#ifndef DOXYGEN_SHOULD_SKIP_THIS
class Header::Option_eq: public std::unary_function<bool, Header::Option> {
  std::string name;
  
public:
  Option_eq( const std::string& n ): name(n) {}

  bool operator ()( const Header::Option& op )
  {
    return op.name == name;
  }
};
#endif

Header::Header():
  content_length_(0)
{
  init_parser();
  
  set_option( "connection:", "close" );
}


Header::~Header()
{
}


void Header::set_version( const std::string& v )
{
  version_ = v;
}


void Header::set_content_length( unsigned lth )
{
  content_length_ = lth;
  std::ostringstream ss;
  ss << lth;
  
  set_option( "content-length:", ss.str() );
  
  if( lth )
    set_option( "content-type:", "text/xml" );
  else
    unset_option( "content-type:" );
}


inline void Header::init_parser()
{
  parsers["content-length:"] = Header::parse_content_length;
  parsers["content-type:"]   = Header::parse_content_type;
}


void Header::register_parser( const std::string& option, Parser parser )
{
  parsers[option] = parser;
}


void Header::parse( const std::string& s )
{
  std::istringstream ss( s );
  parse( ss );
}


void Header::parse( std::istringstream& ss )
{
  while( ss )
  {
    std::string word = read_option_name( ss );
    
    if( word.empty() )
    {
      read_eol(ss);
      break;
    }
    
    set_option( word, read_option_content(ss) );
  }
  
  for( Options_box::const_iterator i = options.begin(); i != options.end(); ++i )
  {
    Parsers_box::const_iterator j = parsers.find( i->name );

    if( j != parsers.end() )
    {
      std::istringstream os(i->value);
      j->second( this, os );
      continue;
    }
    
    if( i->name.find(":") == std::string::npos )
      throw Malformed_packet();
  }
}


void Header::set_option( const std::string& name, const std::string& value )
{
  Option_eq eq( name );
  Options_box::iterator i = std::find_if( options.begin(), options.end(), eq );
  
  if( i == options.end() )
  {
    options.push_back( Option(name, value) );
    return;
  }
  
  i->value = value;
}


void Header::unset_option( const std::string& name )
{
  Option_eq eq( name );
  Options_box::iterator i = std::find_if( options.begin(), options.end(), eq );

  if( i != options.end() )
    options.erase( i );
}


std::string Header::read_option_name( std::istringstream& ss )
{
  std::string word;
  
  while( ss )
  {
    char c = ss.get();
    switch( c )
    {
      case ' ':
      case '\t':
        return word;
      
      case '\r':
      case '\n':
        ss.putback(c);
        return word;
      
      default:
        word += tolower(c);
    }
  }

  throw Malformed_packet();
}


void Header::read_eol( std::istringstream& ss )
{
  char c = ss.get();
  switch( c )
  {
    case '\r':
      if( ss.get() != '\n' )
        throw Malformed_packet();
      break;
      
    case '\n':
      break;
    
    default:
      throw Malformed_packet();
  }
}


std::string Header::read_option_content( std::istringstream& ss )
{
  for( ; ss && (ss.peek() == ' ' || ss.peek() == '\t'); ss.get() )
  
  if( !ss )
    throw Malformed_packet();
  
  std::string option;
  while( ss )
  {
    if( ss.peek() == '\r' || ss.peek() == '\n' )
      break;
    
    option += ss.get();
  }

  read_eol( ss );
  return option;
}


void Header::ignore_line( std::istringstream& ss )
{
  while( ss )
  {
    bool cr = false;
    
    switch( ss.get() )
    {
      case '\r':
        if( cr )
          throw Malformed_packet();
        cr = true;
        break;
      
      case '\n':
        return;
      
      default:
        if( cr )
          throw Malformed_packet();
    }
  }
}


std::string Header::dump() const
{
  std::ostringstream ss;
  
  for( Options_box::const_iterator i = options.begin(); i != options.end(); ++i )
    ss << i->name << " " << i->value << "\r\n";

  return ss.str() + "\r\n";
}


void Header::parse_content_type( Header* obj, std::istringstream& ss )
{
  std::string opt;
  ss >> opt;

  if( opt != "text/xml" )
    throw Unsupported_content_type();
}


void Header::parse_content_length( Header* obj, std::istringstream& ss )
{
  unsigned i;
  ss >> i;
  obj->set_content_length(i);
}


// ----------------------------------------------------------------------------
Request_header::Request_header( const std::string& rstr )
{
  std::istringstream ss( rstr );
  parse( ss );
}


Request_header::Request_header( std::istringstream& ss )
{
  parse( ss );
}


Request_header::Request_header( 
  const std::string& req_uri, 
  const std::string& client_host 
):
  uri_(req_uri),
  host_(client_host),
  user_agent_(PACKAGE " " VERSION)
{
  set_version( "HTTP/1.0" );
  set_option( "user-agent:", user_agent_ );
  set_option( "host:", host_ );
}


Request_header::~Request_header()
{
}


std::string Request_header::dump() const
{
  return "POST " + uri() + " " + version() + "\r\n" + Header::dump();
}


void Request_header::parse( std::istringstream& ss )
{
  register_parser( "user-agent:", Request_header::parse_user_agent );
  register_parser( "host:", Request_header::parse_host );

  parse_method( ss );
  Header::parse( ss );
}


void Request_header::parse_method( std::istringstream& ss )
{
  std::istringstream rs( read_option_content(ss) );
  std::string method, version;

  rs >> method;
  if( method != "POST" )
    throw Method_not_allowed();

  rs >> uri_;
  rs >> version;
  set_version( version );
}


void Request_header::parse_host( Header* obj, std::istringstream& ss )
{
  Request_header* req = static_cast<Request_header*>(obj);
  ss >> req->host_;
}


void Request_header::parse_user_agent( Header* obj, std::istringstream& ss )
{
  Request_header* req = static_cast<Request_header*>(obj);
  ss >> req->user_agent_;
}


// ---------------------------------------------------------------------------
Response_header::Response_header( std::istringstream& ss )
{
  parse( ss );
}


Response_header::Response_header( const std::string& rstr )
{
  std::istringstream ss( rstr );
  parse( ss );
}


Response_header::Response_header( int c, const std::string& p ):
  code_(c),
  phrase_(p),
  server_(PACKAGE " " VERSION)
{
  set_version( "HTTP/1.1" );
  set_option( "date:", current_date() );
  set_option( "server:", server() );
}


Response_header::~Response_header()
{
}


void Response_header::parse( std::istringstream& ss )
{
  register_parser( "server:", Response_header::parse_server );
  
  std::string version;
  ss >> version;
  set_version( version );
  ss >> code_;
  phrase_ = read_option_content( ss );
  
  Header::parse( ss );
}


std::string Response_header::current_date() const
{
  time_t t;
  time( &t );
  struct tm* bdt = gmtime( &t );
  char *oldlocale = setlocale( LC_TIME, 0 );
  setlocale( LC_TIME, "C" );

  char date_str[30];
  date_str[29] = 0;
  strftime( date_str, 30, "%a, %d %b %Y %T %Z", bdt );
  
  setlocale( LC_TIME, oldlocale );
  return date_str;
}


std::string Response_header::dump() const
{
  std::ostringstream ss;
  ss << version() << " " << code() <<  " " << phrase() << "\r\n";
  return ss.str() + Header::dump();
}


void Response_header::parse_server( Header* obj, std::istringstream& ss )
{
  Response_header* resp = static_cast<Response_header*>(obj);
  ss >> resp->server_;
}


// ---------------------------------------------------------------------------
Packet::Packet( Header* h, const std::string& co ):
  header_(h), 
  content_(co) 
{
  header_->set_content_length(content_.length());
}


Packet::Packet( const Packet& p ):
  header_(p.header_->clone()),
  content_(p.content_)
{
}


Packet::~Packet() 
{
  delete header_;
}


Packet& Packet::operator =( const Packet& p )
{
  delete header_;
  header_  = p.header_->clone();
  content_ = p.content_;
  
  return *this;
}


void Packet::set_keep_alive( bool keep_alive )
{
  if( keep_alive )
    header_->set_option( "connection:", "keep-alive" );
  else
    header_->set_option( "connection:", "close" );
}

  
// ---------------------------------------------------------------------------
template <class Header_type>
class iqxmlrpc::http::Packet_reader {
  std::string header_cache;
  std::string content_cache;
  Header* header;
  bool constructed;
  
public:
  Packet_reader():
    header(0), constructed(false) {}
      
  ~Packet_reader()
  {
    if( !constructed )
      delete header;
  }
      
  Packet* read_packet( const std::string& );

private:
  void clear();
  void read_header( const std::string& );
};


template <class Header_type>
inline void Packet_reader<Header_type>::clear()
{
  header = 0;
  content_cache.erase();
  header_cache.erase();
  constructed = false;
}


template <class Header_type>
Packet* Packet_reader<Header_type>::read_packet( const std::string& s )
{
  if( constructed )
    clear();
  
  if( !header )
    read_header(s);
  else
    content_cache += s;
  
  if( header && content_cache.length() >= header->content_length() )  
  {
    content_cache.erase( header->content_length(), std::string::npos );
    Packet* packet = new Packet( header, content_cache );
    constructed = true;
    return packet;
  }
  
  return 0;
}


template <class Header_type>
void Packet_reader<Header_type>::read_header( const std::string& s )
{
  header_cache += s;
  unsigned i = header_cache.find( "\r\n\r\n" );
  
  if( i == std::string::npos )
    i = header_cache.find( "\n\n" );
  
  if( i == std::string::npos )
    return;
  
  std::istringstream ss( header_cache );
  header = new Header_type( ss );
  
  for( char c = ss.get(); ss && !ss.eof(); c = ss.get() )
    content_cache += c;
}


// ---------------------------------------------------------------------------
Server::Server( Method_dispatcher* d ):
  iqxmlrpc::Server(d),
  preader(new Packet_reader<Request_header>),
  packet(0)
{
}


Server::~Server()
{
  delete packet;
  delete preader;
}


bool Server::read_request( const std::string& s )
{
  try {
    packet = preader->read_packet( s );
    return packet;
  }
  catch( const Malformed_packet& )
  {
    throw Bad_request();
  }
}


Packet* Server::execute()
{
  iqxmlrpc::Response resp( iqxmlrpc::Server::execute(packet->content()) );
  std::auto_ptr<xmlpp::Document> xmldoc( resp.to_xml() );
  std::string resp_str = xmldoc->write_to_string_formatted();
  
  return new Packet( new Response_header(), resp_str );
}


// ---------------------------------------------------------------------------
Client::Client( const std::string& uri ):
  uri_(uri), 
  host_(), 
  preader(new Packet_reader<Response_header>), 
  packet(0) 
{
}
    

Client::~Client()
{
  delete packet;
  delete preader;
}


std::string Client::do_execute( const Request& req )
{
  std::auto_ptr<xmlpp::Document> xmldoc( req.to_xml() );
  std::string req_xml_str( xmldoc->write_to_string_formatted() );
  Packet req_p( new Request_header( uri_, host_ ), req_xml_str );

  send_request( req_p );
  recv_response();
  
  // Received packet
  std::auto_ptr<Packet> p( packet );
  packet = 0;
  
  const Response_header* res_h = 
    static_cast<const Response_header*>(p->header());
  
  if( res_h->code() != 200 )
    throw Error_response( res_h->phrase(), res_h->code() );

  return p->content();
}


bool Client::read_response( const std::string& s )
{
  packet = preader->read_packet( s );
  return packet;
}
