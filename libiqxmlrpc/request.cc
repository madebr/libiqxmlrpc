#include <libiqxmlrpc/request.h>
#include <libiqxmlrpc/parser.h>
#include <libiqxmlrpc/value.h>
#include <libiqxmlrpc/except.h>

using namespace iqxmlrpc;


Request::Request( const xmlpp::Document* doc )
{
  parse( doc->get_root_node() );
}


Request::Request( const xmlpp::Node* node )
{
  parse( node );
}


Request::Request( const std::string& name_, const Param_list& params_ ):
  name(name_),
  params(params_)
{
}


Request::~Request()
{
}


xmlpp::Document* Request::to_xml() const
{
  using namespace xmlpp;

  Document* doc = new Document();

  try 
  {
    Element* el = doc->create_root_node( "methodCall" );
    Element* name_el = el->add_child( "methodName" );
    name_el->add_child_text( name );
    Element* params_el = el->add_child( "params" );
    
    for( const_iterator i = params.begin(); i != params.end(); ++i )
    {
      Element* p_el = params_el->add_child( "param" );
      i->to_xml( p_el );
    }
  }
  catch(...)
  {
    delete doc;
    throw;
  }
  
  return doc;
}


inline void Request::parse( const xmlpp::Node* node )
{
  if( node->get_name() != "methodCall" )
    throw Parse_error::at_node(node);
  
  xmlpp::Node::NodeList nlist = Parser::instance()->elements_only( node );
  if( nlist.size() != 2 )
    throw Parse_error::at_node(node);

  parse_name( nlist.front() );
  parse_params( nlist.back() );
}


inline void Request::parse_name( const xmlpp::Node* node )
{
  using namespace xmlpp;

  if( node->get_name() != "methodName" )
    throw Parse_error::at_node(node);
  
  Node::NodeList childs = node->get_children();
  if( childs.size() != 1 )
    throw Parse_error::at_node(node);
  
  const TextNode *text = dynamic_cast<const TextNode*>(childs.front());
  if( !text )
    throw Parse_error::at_node(node);
  
  name = text->get_content();
}


inline void Request::parse_params( const xmlpp::Node* node )
{
  using namespace xmlpp;
  Parser* parser = Parser::instance();
  
  if( node->get_name() != "params" )
    throw Parse_error::at_node(node);
  
  Node::NodeList childs = parser->elements_only( node );
  for( Node::NodeList::const_iterator i=childs.begin(); i!=childs.end(); ++i )
  {
    if( (*i)->get_name() != "param" )
      throw Parse_error::at_node(*i);
    
    Node* param = parser->single_element( *i );
    Value* value = parser->parse_value(param);
    params.push_back( *value );
    delete value;
  }
}
