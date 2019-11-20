#ifndef XML_DOCUMENT_H
#define XML_DOCUMENT_H

#include <cassert>
#include <iterator>
#include <map>
#include <stdexcept>
#include <string>

#include "stl_extensions.h" // for for_each_second()

// for libxml2
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <libxml/parserInternals.h> // for text nodes that should not be escaped when serialized (see node_t::set_value)

// for libxslt
#include <libxslt/transform.h>
#include <libxslt/xsltutils.h>


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// XML document
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

class xml_document_t {
  xmlDocPtr doc;
  xmlXPathContextPtr ctx;
  typedef std::map<std::pair<xmlNodePtr, std::string>, xmlXPathObjectPtr> cache_t; 
  cache_t cache; // cache of previous query results 

  /**
  * register_namespaces:
  * @xpathCtx:		the pointer to an XPath context.
  * @nsList:		the list of known namespaces in 
  *			"<prefix1>=<href1> <prefix2>=href2> ..." format.
  *
  * Registers namespaces from @nsList in @xpathCtx.
  *
  * Returns 0 on success and a negative value otherwise.
  */
  int register_namespaces(xmlXPathContextPtr xpathCtx, const xmlChar* nsList) {
    xmlChar* nsListDup;
    xmlChar* prefix;
    xmlChar* href;
    xmlChar* next;

    assert(xpathCtx);
    assert(nsList);

    nsListDup = xmlStrdup(nsList);
    if(nsListDup == NULL) {
      fprintf(stderr, "Error: unable to strdup namespaces list\n");
      return(-1);	
    }

    next = nsListDup; 
    while(next != NULL) {
      /* skip spaces */
      while((*next) == ' ') next++;
      if((*next) == '\0') break;

      /* find prefix */
      prefix = next;
      next = BAD_CAST xmlStrchr(next, '=');
      if(next == NULL) {
	fprintf(stderr,"Error: invalid namespaces list format\n");
	xmlFree(nsListDup);
	return(-1);	
      }
      *(next++) = '\0';	

      /* find href */
      href = next;
      next = BAD_CAST xmlStrchr(next, ' ');
      if(next != NULL) {
	*(next++) = '\0';	
      }

      /* do register namespace */
      if(xmlXPathRegisterNs(xpathCtx, prefix, href) != 0) {
	fprintf(stderr,"Error: unable to register NS with prefix=\"%s\" and href=\"%s\"\n", prefix, href);
	xmlFree(nsListDup);
	return(-1);	
      }
    }

    xmlFree(nsListDup);
    return(0);
  }

  void init_ctx(const char *namespaces_list = 0) {
    ctx = xmlXPathNewContext(doc);
    if (!ctx)
      throw std::runtime_error("unable to create new XPath context");
    if (namespaces_list) {
      if (register_namespaces(ctx, BAD_CAST namespaces_list) < 0)
	throw std::runtime_error("failed to register namespaces list");
    }
  }

public:

  // unopened document
  xml_document_t() : doc(NULL), ctx(NULL) {}

  // construct from contents of file and namespaces list
  xml_document_t(const char *file_name, const char *namespaces_list = 0) : doc(NULL), ctx(NULL) {
    open(file_name, namespaces_list);
  }

  // construct from XML string
  xml_document_t(const std::string &serialized_data) : doc(NULL), ctx(NULL) {
    doc = xmlParseDoc(BAD_CAST serialized_data.data());
    if (!doc)
      throw std::runtime_error(std::string("unable to parse XML in string: ") + serialized_data);
    else
      init_ctx();
  }

  ~xml_document_t() {
    close();
  }

  // open
  bool open(const char *file_name, const char *namespaces_list = 0) {
    doc = xmlParseFile(file_name);
    if (!doc)
      throw std::runtime_error(std::string("unable to parse XML in file: ") + file_name);
    init_ctx(namespaces_list);
    return true;
  }

  // open from xmlDocPtr
  bool open(xmlDocPtr doc_, const char *namespaces_list = 0) {
    close();
    doc = doc_;
    init_ctx(namespaces_list);
    return true;
  }

  // close
  void close() {
    if (doc) {
      xmlFreeDoc(doc);
      doc = NULL;
    }
    if (ctx) {
      xmlXPathFreeContext(ctx);
      ctx = NULL;
    }
    for_each_second(cache.begin(), cache.end(), xmlXPathFreeObject);
  }

  // node in XML tree
  // NB. all methods are const to facilitate use as a shim
  class node_t : public xmlNode {
  public:

    // get name of node
    const char *get_name() const {
      if (!name)
	throw std::runtime_error(std::string("node has no name"));
      else
	return reinterpret_cast<const char *>(name);
    }

    // set name of node
    void set_name(const char *name) {
      if (xmlValidateName(BAD_CAST name, 0))
	std::runtime_error(std::string("name is not valid: ") + name);
      xmlNodeSetName(this, xmlStrdup(BAD_CAST name));
    }

    // get text value of node, or empty string if none
    operator const char *() const {
      xmlNodePtr n = const_cast<node_t *>(this);
      if (xmlNodeIsText(n))
        return reinterpret_cast<const char *>(n->content);
      if (n->children != 0 && n->children->next == 0 && xmlNodeIsText(n->children))
        return reinterpret_cast<const char *>(n->children->content);
      return "";
    }

    // get XML value of node 
    void get_XML_value(char *ptr, int length) const {
      xmlBufferPtr buf = xmlBufferCreate();
      for (xmlNodePtr n = const_cast<xmlNodePtr>(children); n; n = n->next)
	xmlNodeDump(buf, doc, n, 0, 0);
      xmlBufferAdd(buf, reinterpret_cast<const xmlChar *>(""), 1);
      const char *p = reinterpret_cast<const char *>(xmlBufferContent(buf));
      int l = strlen(p) + 1;
      if (l >= length)
	throw std::runtime_error(std::string("XML value too long: ") + reinterpret_cast<const char *>(name));
      strcpy(ptr, p);
      xmlBufferFree(buf);
    }

    // set text value of node (see code for formatting)
    const char *set_value(const char *new_content, bool escape_XML_special_characters = true) {
      xmlNodePtr n = this;
      if (n && !xmlNodeIsText(n)) {
	if (n->children)
	  n = n->children;
	else
	  n = xmlAddChild(n, xmlNewDocText(n->doc, BAD_CAST new_content));
      }
      if (!n || !xmlNodeIsText(n))
	throw std::runtime_error(std::string("not a text node"));
      else if (escape_XML_special_characters)
	xmlNodeSetContent(n, BAD_CAST new_content);
      else { // set length of raw text and copy raw text to node
	xmlNodeSetContentLen(n, BAD_CAST new_content, strlen(new_content));
	strcpy(reinterpret_cast<char *>(n->content), new_content);
	n->name = xmlStringTextNoenc;
      }
      return reinterpret_cast<const char *>(n && xmlNodeIsText(n) ? n->content : BAD_CAST "");
    }

    const char *operator=(const char *new_content) {
      return set_value(new_content);
    }

    // copy and link node (recursive)
    node_t &operator=(const node_t &n) {
      throw std::runtime_error(std::string("assignment of nodes is not implemented"));
      //xmlNodePtr new_node = xmlDocCopyNode(const_cast<node_t *>(&n), n.doc, 1);
      //xmlReplaceNode(this, new_node); // does this free the old tree?
      //return *this;
    }
  };

  // iterator for a set of nodes the satisfy an XPath expression
  class iterator : public std::iterator<std::forward_iterator_tag, node_t> {

    xmlXPathContextPtr ctx;
    xmlNodePtr ctx_node;
    xmlXPathObjectPtr obj;
    xmlNodePtr *cur; // set to NULL to indicate end of iteration
    xmlNodePtr *last;
    cache_t *cache;

    // copy
    void copy(const iterator &old) {
      ctx = old.ctx;
      ctx_node = old.ctx_node;
      cache = old.cache;
      if (old.obj) {
        obj = xmlXPathObjectCopy(old.obj);
	cur = obj->nodesetval->nodeTab + (old.cur - old.obj->nodesetval->nodeTab);
	last = obj->nodesetval->nodeTab + obj->nodesetval->nodeNr;
      } else if (old.cur) { // single value (obj = NULL, cur != NULL) uses pointer to self
	obj = NULL;
	cur = &ctx_node;
	last = cur + 1;
      } else {
        obj = NULL;
	cur = NULL;
	last = NULL;
      }
    }

    // extract XPath namespace from name and find corresponding document name and namespace
    // if no namespace is passed, use default document namespace for current node
    // NB.  if XPath_name starts with '@' it is ignored
    struct XPath_to_doc_name_t {
      const xmlChar *doc_name;
      xmlNsPtr doc_name_space;
      XPath_to_doc_name_t(const iterator &i, const char *XPath_name) {
	if (!i.cur || !*i.cur)
	  throw std::runtime_error(std::string("no current position of iterator"));
	if (*XPath_name == '@')
	  ++XPath_name;
	const char *colon = strchr(XPath_name, ':');
	if (!colon) {
	  doc_name = BAD_CAST XPath_name;
	  doc_name_space = (*i.cur)->ns; // use default document name space
	} else {
	  xmlChar prefix[256];
	  size_t prefix_length = colon - XPath_name;
	  assert(prefix_length < sizeof(prefix));
	  memcpy(prefix, XPath_name, prefix_length);
	  prefix[prefix_length] = 0;
	  const xmlChar *href = xmlXPathNsLookup(i.ctx, prefix);
	  // skip over default name space in search loop
	  for (xmlNsPtr ns = (*i.cur)->ns; ns; ns = ns->next)
	    if (ns->prefix != NULL && xmlStrEqual(href, ns->href)) {
	      doc_name = BAD_CAST colon + 1;
	      doc_name_space = ns;
	      return;
	    }
	  throw std::runtime_error(std::string("unable to find XPath namespace in document with href = ") + reinterpret_cast<const char *>(href));
	}
      }
    };

  public:

    // construct null iterator (end of iteration)
    iterator() : obj(NULL), cur(NULL), cache(NULL) {}

    // construct iterator with context set to ctx_node
    iterator(xmlXPathContextPtr ctx_, xmlNodePtr ctx_node_)
      : ctx(ctx_), ctx_node(ctx_node_), cur(&ctx_node), last(&ctx_node + 1), obj(NULL), cache(NULL)
    {}

    // construct iterator for nodes satisfying XPath expression with context set to ctx_node
    // maintain document-wide cache of query results
    iterator(xmlXPathContextPtr ctx_, xmlNodePtr ctx_node_, const char *xpath_expr, cache_t *cache_)
      : ctx(ctx_), ctx_node(ctx_node_), cur(NULL), cache(cache_)
    {
      cache_t::iterator i = cache->find(std::make_pair(ctx_node_, xpath_expr));
      if (i != cache->end()*/)
	obj = xmlXPathObjectCopy(i->second);
      else {
	xmlNodePtr save = ctx_->node;
	ctx_->node = ctx_node_;
	obj = xmlXPathEvalExpression(BAD_CAST xpath_expr, ctx_);
	ctx_->node = save;
	if (obj == NULL || obj->nodesetval == NULL || obj->nodesetval->nodeNr == 0)
	  throw std::runtime_error(std::string(xpath_expr) + " not found");
	(*cache)[make_pair(ctx_node_, xpath_expr)] = xmlXPathObjectCopy(obj); // save copy to cache
      }
      cur = obj->nodesetval->nodeTab;
      last = cur + obj->nodesetval->nodeNr;
    }

    // copy constructor
    iterator(const iterator &old) {
      copy(old);
    }

    ~iterator() {
      if (obj != NULL)
	xmlXPathFreeObject(obj);
    }

    iterator operator=(const iterator &old) {
      copy(old);
      return *this;
    }

    // prefix increment iterator; set to null if at end
    iterator &operator++(void) {
      if (cur) {
	++cur;
	if (cur == last)
	  cur = NULL;
      }
      if (cur == NULL && obj != NULL) { // free obj as soon as possible
	xmlXPathFreeObject(obj);
	obj = NULL;
      }
      return *this;
    }

    // postfix increment iterator
    // TODO: use shared XPath node sets
    iterator operator++(int) {
      iterator retval(*this);
      this->operator++();
      return retval;
    }

    operator bool(void) const {
      return cur != NULL;
    }

    bool operator!=(const iterator &i) const {
      return cur != i.cur;
    }

    node_t *operator->() const {
      return &operator*();
    }

    // get current node
    node_t &operator*() const {
      if (!cur || !*cur)
	throw std::runtime_error(std::string("no current position of iterator"));
      else
        return reinterpret_cast<node_t &>(**cur);
    }

    // return iterator for node set satisfying XPath expression with current context
    iterator find(const char *xpath_expr) const {
      return cur ? iterator(ctx, *cur, xpath_expr, cache) : iterator();
    }

    iterator find(const std::string &xpath_expr) const {
      return find(xpath_expr.c_str());
    }

    iterator begin(const char *xpath_expr) const {
      return find(xpath_expr);
    }

    iterator end() const {
      return iterator();
    }

    // return iterator for new child node with given name (in given name space) and content with current context
    iterator insert(const char *XPath_name, node_t &content) const {
      if (*XPath_name == '@')
	throw std::runtime_error(std::string("node name may not start with '@'; use insert_attribute: ") + XPath_name);
      XPath_to_doc_name_t x(*this, XPath_name);
      xmlSetNs(&content, x.doc_name_space);
      xmlNodeSetName(&content, x.doc_name);
      return iterator(ctx, xmlAddChild(*cur, &content));
    }

    iterator insert(const char *XPath_name, const node_t &content) const {
      return insert(XPath_name, reinterpret_cast<node_t &>(*xmlCopyNode((const xmlNodePtr)&content, 1))); 
    }

    iterator insert(const char *XPath_name, const char *content) const {
      if (*XPath_name == '@')
	throw std::runtime_error(std::string("node name may not start with '@'; use insert_attribute: ") + XPath_name);
      XPath_to_doc_name_t x(*this, XPath_name);
      return iterator(ctx, xmlNewTextChild(*cur, x.doc_name_space, x.doc_name, BAD_CAST content));
    }

    iterator insert(const std::string &XPath_name, const std::string &content) const {
      return insert(XPath_name.c_str(), content.c_str());
    }

     void erase() {
      if (!cur || !*cur)
	throw std::runtime_error(std::string("no current position of iterator"));
      xmlUnlinkNode(*cur);
      xmlFreeNode(*cur);
    }

    const iterator &insert_attribute(const char *XPath_name, const char *content) const {
      XPath_to_doc_name_t x(*this, XPath_name);
      xmlNewNsProp(*cur, x.doc_name_space, x.doc_name, BAD_CAST content);
      return *this;
    }

    const iterator &insert_attribute(const std::string &XPath_name, const std::string &content) const {
      return insert_attribute(XPath_name.c_str(), content.c_str());
    }

    const iterator &erase_attribute(const char *XPath_name) const {
      XPath_to_doc_name_t x(*this, XPath_name);
      xmlUnsetNsProp(*cur, x.doc_name_space, x.doc_name);
      return *this;
    }

    const iterator &erase_attribute(const std::string &XPath_name) const {
      return erase_attribute(XPath_name.c_str());
    }
  };

  // beginning of node set satisfying XPath expression with root context
  iterator begin(const char *xpath_expr) {
    return iterator(ctx, reinterpret_cast<xmlNodePtr>(NULL), xpath_expr, &cache);
  }

  iterator begin(const std::string &xpath_expr) {
    return begin(xpath_expr.c_str());
  }

  // end of any node set
  iterator end() const {
    return iterator();
  }

  // save XML to file
  void save(const char *output_file, const char *encoding = 0) {
    std::string error;
    if (xmlSaveFileEnc(output_file, doc, encoding) == -1)
      error = std::string("unable to write to file ") + output_file;
    if (!error.empty())
      throw std::runtime_error(error);
    return;
  }

  // dump XML to file
  void dump(FILE *file) {
    std::string error;
    if (xmlDocDump(file, doc) == -1)
      error = std::string("unable to dump to file");
    if (!error.empty())
      throw std::runtime_error(error);
    return;
  }

  // transform XML using XSLT from file and write to file
  void transform(const char *xslt_file, const char *output_file_name, const char **params) const {
    std::string error;
    xsltStylesheetPtr sty = xsltParseStylesheetFile(BAD_CAST xslt_file);
    if (!sty)
      error = std::string("unable to open XSLT file ") + xslt_file;
    else {
      transform(sty, output_file_name, params);
      xsltFreeStylesheet(sty);
    }
  }

  // transform XML using XSLT from string and write to file
  void transform(const std::string &xslt, const char *output_file_name, const char **params) const {
    std::string error;
    xmlDocPtr xml = xmlParseDoc(BAD_CAST xslt.c_str());
    if (!xml)
      error = std::string("unable to parse XML string ") + xslt;
    else {
      xsltStylesheetPtr sty = xsltParseStylesheetDoc(xml);
      if (!sty)
	error = std::string("unable to parse XSLT stylesheet ") + xslt;
      else {
	transform(sty, output_file_name, params);
	xsltFreeStylesheet(sty);
      }
      xmlFreeDoc(xml);
    }
    if (!error.empty())
      throw std::runtime_error(error);
    return;
  }

  // transform XML using XSLT and output to file
  void transform(xsltStylesheetPtr sty, const char *output_file_name, const char **params) const {
    std::string error;
    xmlDocPtr res = xsltApplyStylesheet(sty, doc, params);
    if (!res)
      error = std::string("unable to apply XSLT");
    else {
      if (xmlSaveFile(output_file_name, res) == -1)
	error = std::string("unable to write to file");
      xmlFreeDoc(res);
    }
    if (!error.empty())
      throw std::runtime_error(error);
    return;
  }

  // transform XML using XSLT and output to stream
  void transform(xsltStylesheetPtr sty, FILE *file, const char **params) const {
    std::string error;
    xmlDocPtr res = xsltApplyStylesheet(sty, doc, params);
    if (!res)
      error = std::string("unable to apply XSLT");
    else {
      for (xmlNodePtr node = res->children; node; node = node->next)
	xmlElemDump(file, res, node);
      xmlFreeDoc(res);
    }
    if (!error.empty())
      throw std::runtime_error(error);
    return;
  }

  // transform XML using XSLT
  void transform(const xml_document_t &xml_doc, xsltStylesheetPtr sty, const char **params) {
    std::string error;
    xmlDocPtr res = xsltApplyStylesheet(sty, xml_doc.doc, params);
    if (!res)
      error = std::string("unable to apply XSLT");
    else
      open(res);
    if (!error.empty())
      throw std::runtime_error(error);
    return;
  }

  // parse current XML doc into style sheet and return it (must be freed by free_style_sheet() below)
  xsltStylesheetPtr allocate_style_sheet(void) {
    xsltStylesheetPtr sty = xsltParseStylesheetDoc(doc);
    if (!sty)
      throw std::runtime_error(std::string("unable to parse XSLT stylesheet"));
    else
      return sty;
  }

  // free previously allocated style sheet
  void free_style_sheet(xsltStylesheetPtr sty) {
    xsltFreeStylesheet(sty);
  }

};
#endif // XML_DOCUMENT_H
