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
//  $Id: http.h,v 1.12 2004-03-29 06:23:18 adedov Exp $

#ifndef _libiqxmlrpc_http_h_
#define _libiqxmlrpc_http_h_

#include <string>
#include <sstream>
#include <map>
#include <list>
#include <libiqxmlrpc/except.h>
#include <libiqxmlrpc/null_transport.h>


namespace iqxmlrpc
{
  //! XML-RPC HTTP transport-independent infrastructure.
  /*! Contains classes which responsible for transport-indepenent 
      HTTP collaboration functionality. Such as packet parsing/constructing, 
      wrapping XML-RPC message into HTTP-layer one and vice versa.
  */
  namespace http
  {
    class Header;
    class Request_header;
    class Response_header;
    class Packet;
    
    template <class Header_type>
    class Packet_reader;
      
    class Malformed_packet;
    class Error_response;
    class Bad_request;
    class Method_not_allowed;
    class Unsupported_content_type;
      
    class Server;
    class Client;
  };
};

//! HTTP header. Responsible for parsing, 
//! creating generic HTTP headers.
class iqxmlrpc::http::Header {
public:
  typedef void (*Parser)( Header*, std::istringstream& );
  typedef std::map<std::string, Parser> Parsers_box;

private:
  struct Option {
    std::string name;
    std::string value;
    
    Option( const std::string& n, const std::string& v ):
      name(n), value(v) {}
  };

  typedef std::list<Option> Options_box;
  class Option_eq;

private:
  Parsers_box parsers;
  Options_box options;

  std::string version_;
  unsigned    content_length_;
  
public:
  Header();
  virtual ~Header();

  virtual Header* clone() const { return new Header(*this); }
  
  const std::string& version()        const { return version_; }
  unsigned           content_length() const { return content_length_; }
  
  void set_version( const std::string& v );
  void set_content_length( unsigned ln );

  //! Adds/alters header option.
  /*! For example: 
      \code set_option( "allow:", "POST" ); \endcode
  */
  void set_option( const std::string& name, const std::string& value );
  
  //! Remove option from header.
  void unset_option( const std::string& name );

  //! Return text representation of header including final CRLF.
  virtual std::string dump() const;
  
protected:
  //! Register parser for specific option name.
  void register_parser( const std::string&, Parser );

  void parse( const std::string& );
  void parse( std::istringstream& );

  void read_eol( std::istringstream& );
  void ignore_line( std::istringstream& );
  std::string read_option_name( std::istringstream& );
  std::string read_option_content( std::istringstream& );

private:
  void init_parser();

  static void parse_content_type( Header*, std::istringstream& );
  static void parse_content_length( Header*, std::istringstream& );
};


//! HTTP request's header.
class iqxmlrpc::http::Request_header: public http::Header {
  std::string uri_;
  std::string host_;
  std::string user_agent_;
  
public:
  Request_header( const std::string& to_parse );
  Request_header( std::istringstream& to_parse );
  Request_header( const std::string& uri, const std::string& client_host );

  ~Request_header();

  Request_header* clone() const { return new Request_header(*this); }

  const std::string& uri()   const { return uri_; }
  const std::string& host()  const { return host_; }
  const std::string& agent() const { return user_agent_; }

  std::string dump() const;

private:
  void parse( std::istringstream& );
  void parse_method( std::istringstream& );
  static void parse_host( Header*, std::istringstream& );
  static void parse_user_agent( Header*, std::istringstream& );
};


//! HTTP response's header.
class iqxmlrpc::http::Response_header: public http::Header {
  int code_;
  std::string phrase_;
  std::string server_;

public:
  Response_header( const std::string& to_parse );
  Response_header( std::istringstream& to_parse );
  Response_header( int = 200, const std::string& = "OK" );

  ~Response_header();

  Response_header* clone() const { return new Response_header(*this); }

  int                code()   const { return code_; }
  const std::string& phrase() const { return phrase_; }
  const std::string& server() const { return server_; }
  
  std::string dump() const;

private:
  void parse( std::istringstream& );
  std::string current_date() const;

  static void parse_server( Header*, std::istringstream& );
};


//! HTTP packet: Header + Content.
class iqxmlrpc::http::Packet {
protected:
  http::Header* header_;
  std::string content_;
  
public:
  Packet( const Packet& );
  Packet( http::Header* header, const std::string& content );
  virtual ~Packet();

  Packet& operator =( const Packet& );

  //! Sets header option "connection: {keep-alive|close}".
  //! By default connection is close.
  void set_keep_alive( bool = true );
  
  const http::Header* header()  const { return header_; }
  const std::string&  content() const { return content_; }

  virtual std::string dump() const
  {
    return header_->dump() + content_;
  }  
};


//! HTTP XML-RPC server independet from 
//! low level transport implementation.
class iqxmlrpc::http::Server: private iqxmlrpc::Server {
  http::Packet_reader<Request_header>* preader;
  http::Packet* packet;

public:
  Server( Method_dispatcher* );
  ~Server();

  //! Considers parameter as a piece of request.
  //! \return true if whole request has been read,
  //! \return false if more data is needed.
  bool read_request( const std::string& );

  //! Executes request which has been read.
  http::Packet* execute();
};


//! HTTP XML-RPC client code independet from 
//! low-level transport implementation.
class iqxmlrpc::http::Client: public iqxmlrpc::Client {
  std::string uri_;
  std::string host_;
  Packet_reader<Response_header>* preader;
  Packet* packet;
  
public:
  Client( const std::string& uri );
  ~Client();
      
  //! Real client should use this function to
  //! set client's host name sent to server.
  void set_client_host( const std::string& h )
  {
    host_ = h;
  }

protected:
  //! Is called from Client::execute().
  //! Can throw Error_response.
  std::string do_execute( const Request& );

  //! Considers parameter as a piece of response.
  //! \return true if whole response has been read,
  //! \return false if more data is needed.
  bool read_response( const std::string& );

  virtual void send_request( const http::Packet& ) = 0;
  virtual void recv_response() = 0;
};


//! Exception which is thrown on syntax error during HTTP packet parsing.
class iqxmlrpc::http::Malformed_packet: public Exception {
public:
  Malformed_packet():
    Exception( "Malformed HTTP packet received." ) {}
};


//! Exception related to HTTP protocol. 
//! Can be sent as error response to client.
class iqxmlrpc::http::Error_response: public http::Packet, public Exception {
public:
  Error_response( const std::string& phrase, int code ):
    Packet( new Response_header(code, phrase), "" ),
    Exception( "HTTP: " + phrase ) {}
  
  virtual ~Error_response() throw() {}
};


//! HTTP/1.1 400 Bad request
class iqxmlrpc::http::Bad_request: public http::Error_response {
public:
  Bad_request():
    Error_response( "Bad request", 400 ) {}
};


//! HTTP/1.1 405 Method not allowed
class iqxmlrpc::http::Method_not_allowed: public http::Error_response {
public:
  Method_not_allowed():
    Error_response( "Method not allowed", 405 )
  {
    header_->set_option( "allowed:", "POST" );
  }
};


//! HTTP/1.1 415 Unsupported media type
class iqxmlrpc::http::Unsupported_content_type: public http::Error_response {
public:
  Unsupported_content_type():
    Error_response( "Unsupported media type", 415 ) {}
};


#endif
