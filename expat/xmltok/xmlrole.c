#ifdef _MSC_VER
#define XMLTOKAPI __declspec(dllexport)
#endif
#include "xmlrole.h"

/* Doesn't check:

 that ,| are not mixed in a model group
 content of literals

Separate handler for external internalSubsets

Level 0 == outside DTD
Level 1 == in DTD internalSubset
Level 2 == in Declaration
Level 3 == in Group
*/

PROLOG_HANDLER
  prolog0, prolog1,
  doctype0, doctype1, doctype2, doctype3, doctype4, doctype5,
  internalSubset,
  entity0, entity1, entity2, entity3, entity4, entity5, entity6,
  notation0, notation1, notation2, notation3,
  attlist0, attlist1, attlist2, attlist3, attlist4, attlist5, attlist6,
  attlist7, attlist8, attlist9,
  element0, element1, element2, element3, element4, element5, element6,
  element7,
  declClose,
  error;

PROLOG_HANDLER ignore, declParamEntityRef;

PROLOG_HANDLER declParamEntityRef;

static
int syntaxError(PROLOG_STATE *);

static
int prolog0(PROLOG_STATE *state,
	    int tok,
	    const char *ptr,
	    const char *end,
	    const ENCODING *enc)
{
  switch (tok) {
  case XML_TOK_PI:
  case XML_TOK_COMMENT:
    return XML_ROLE_NONE;
  case XML_TOK_DECL_OPEN:
    if (!XmlNameMatchesAscii(enc,
			     ptr + 2 * enc->minBytesPerChar,
			     "DOCTYPE"))
      break;
    state->handler = doctype0;
    return XML_ROLE_NONE;
  }
  return syntaxError(state);
}

static
int prolog1(PROLOG_STATE *state,
	    int tok,
	    const char *ptr,
	    const char *end,
	    const ENCODING *enc)
{
  switch (tok) {
  case XML_TOK_PI:
  case XML_TOK_COMMENT:
    return XML_ROLE_NONE;
  }
  return syntaxError(state);
}

static
int doctype0(PROLOG_STATE *state,
	     int tok,
	     const char *ptr,
	     const char *end,
	     const ENCODING *enc)
{
  switch (tok) {
  case XML_TOK_NAME:
    state->handler = doctype1;
    return XML_ROLE_DOCTYPE_NAME;
  }
  return syntaxError(state);
}

int doctype1(PROLOG_STATE *state,
	     int tok,
	     const char *ptr,
	     const char *end,
	     const ENCODING *enc)
{
  switch (tok) {
  case XML_TOK_OPEN_BRACKET:
    state->handler = internalSubset;
    return XML_ROLE_NONE;
  case XML_TOK_DECL_CLOSE:
    state->handler = prolog1;
    return XML_ROLE_DOCTYPE_CLOSE;
  case XML_TOK_NAME:
    if (XmlNameMatchesAscii(enc, ptr, "SYSTEM")) {
      state->handler = doctype3;
      return XML_ROLE_NONE;
    }
    if (XmlNameMatchesAscii(enc, ptr, "PUBLIC")) {
      state->handler = doctype2;
      return XML_ROLE_NONE;
    }
    break;
  }
  return syntaxError(state);
}

int doctype2(PROLOG_STATE *state,
	     int tok,
	     const char *ptr,
	     const char *end,
	     const ENCODING *enc)
{
  switch (tok) {
  case XML_TOK_LITERAL:
    state->handler = doctype3;
    return XML_ROLE_DOCTYPE_PUBLIC_ID;
  }
  return syntaxError(state);
}

int doctype3(PROLOG_STATE *state,
	     int tok,
	     const char *ptr,
	     const char *end,
	     const ENCODING *enc)
{
  switch (tok) {
  case XML_TOK_LITERAL:
    state->handler = doctype4;
    return XML_ROLE_DOCTYPE_SYSTEM_ID;
  }
  return syntaxError(state);
}

int doctype4(PROLOG_STATE *state,
	     int tok,
	     const char *ptr,
	     const char *end,
	     const ENCODING *enc)
{
  switch (tok) {
  case XML_TOK_OPEN_BRACKET:
    state->handler = internalSubset;
    return XML_ROLE_NONE;
  case XML_TOK_DECL_CLOSE:
    state->handler = prolog1;
    return XML_ROLE_DOCTYPE_CLOSE;
  }
  return syntaxError(state);
}

int doctype5(PROLOG_STATE *state,
	     int tok,
	     const char *ptr,
	     const char *end,
	     const ENCODING *enc)
{
  switch (tok) {
  case XML_TOK_DECL_CLOSE:
    state->handler = prolog1;
    return XML_ROLE_DOCTYPE_CLOSE;
  }
  return syntaxError(state);
}

static
int internalSubset(PROLOG_STATE *state,
	   int tok,
	   const char *ptr,
	   const char *end,
	   const ENCODING *enc)
{
  switch (tok) {
  case XML_TOK_DECL_OPEN:
    if (XmlNameMatchesAscii(enc,
			    ptr + 2 * enc->minBytesPerChar,
			    "ENTITY")) {
      state->handler = entity0;
      return XML_ROLE_NONE;
    }
    if (XmlNameMatchesAscii(enc,
			    ptr + 2 * enc->minBytesPerChar,
			    "ELEMENT")) {
      state->handler = element0;
      return XML_ROLE_NONE;
    }
    if (XmlNameMatchesAscii(enc,
			    ptr + 2 * enc->minBytesPerChar,
			    "ATTLIST")) {
      state->handler = attlist0;
      return XML_ROLE_NONE;
    }
    if (XmlNameMatchesAscii(enc,
			    ptr + 2 * enc->minBytesPerChar,
			    "NOTATION")) {
      state->handler = notation0;
      return XML_ROLE_NONE;
    }
    break;
  case XML_TOK_PI:
  case XML_TOK_COMMENT:
  case XML_TOK_PARAM_ENTITY_REF:
    return XML_ROLE_NONE;
  case XML_TOK_CLOSE_BRACKET:
    state->handler = doctype5;
    return XML_ROLE_NONE;
  }
  return syntaxError(state);
}

static
int entity0(PROLOG_STATE *state,
	    int tok,
	    const char *ptr,
	    const char *end,
	    const ENCODING *enc)
{
  switch (tok) {
  case XML_TOK_PERCENT:
    state->handler = entity1;
    return XML_ROLE_NONE;
  case XML_TOK_NAME:
    state->handler = entity2;
    return XML_ROLE_GENERAL_ENTITY_NAME;
  }
  return syntaxError(state);
}

static
int entity1(PROLOG_STATE *state,
	    int tok,
	    const char *ptr,
	    const char *end,
	    const ENCODING *enc)
{
  switch (tok) {
  case XML_TOK_NAME:
    state->handler = entity2;
    return XML_ROLE_PARAM_ENTITY_NAME;
  }
  return syntaxError(state);
}

static
int entity2(PROLOG_STATE *state,
	    int tok,
	    const char *ptr,
	    const char *end,
	    const ENCODING *enc)
{
  switch (tok) {
  case XML_TOK_NAME:
    if (XmlNameMatchesAscii(enc, ptr, "SYSTEM")) {
      state->handler = entity4;
      return XML_ROLE_NONE;
    }
    if (XmlNameMatchesAscii(enc, ptr, "PUBLIC")) {
      state->handler = entity3;
      return XML_ROLE_NONE;
    }
    break;
  case XML_TOK_LITERAL:
    state->handler = entity4;
    return XML_ROLE_ENTITY_VALUE;
  }
  return syntaxError(state);
}

static
int entity3(PROLOG_STATE *state,
	    int tok,
	    const char *ptr,
	    const char *end,
	    const ENCODING *enc)
{
  switch (tok) {
  case XML_TOK_LITERAL:
    state->handler = entity4;
    return XML_ROLE_ENTITY_PUBLIC_ID;
  }
  return syntaxError(state);
}


static
int entity4(PROLOG_STATE *state,
	    int tok,
	    const char *ptr,
	    const char *end,
	    const ENCODING *enc)
{
  switch (tok) {
  case XML_TOK_LITERAL:
    state->handler = entity5;
    return XML_ROLE_ENTITY_SYSTEM_ID;
  }
  return syntaxError(state);
}

static
int entity5(PROLOG_STATE *state,
	    int tok,
	    const char *ptr,
	    const char *end,
	    const ENCODING *enc)
{
  switch (tok) {
  case XML_TOK_DECL_CLOSE:
    state->handler = internalSubset;
    return XML_ROLE_NONE;
  case XML_TOK_NAME:
    if (XmlNameMatchesAscii(enc, ptr, "NDATA")) {
      state->handler = entity6;
      return XML_ROLE_NONE;
    }
    break;
  }
  return syntaxError(state);
}

static
int entity6(PROLOG_STATE *state,
	    int tok,
	    const char *ptr,
	    const char *end,
	    const ENCODING *enc)
{
  switch (tok) {
  case XML_TOK_NAME:
    state->handler = declClose;
    return XML_ROLE_ENTITY_NOTATION_NAME;
  }
  return syntaxError(state);
}

static
int notation0(PROLOG_STATE *state,
	      int tok,
	      const char *ptr,
	      const char *end,
	      const ENCODING *enc)
{
  switch (tok) {
  case XML_TOK_NAME:
    state->handler = notation1;
    return XML_ROLE_NOTATION_NAME;
  }
  return syntaxError(state);
}

static
int notation1(PROLOG_STATE *state,
	      int tok,
	      const char *ptr,
	      const char *end,
	      const ENCODING *enc)
{
  switch (tok) {
  case XML_TOK_NAME:
    if (XmlNameMatchesAscii(enc, ptr, "SYSTEM")) {
      state->handler = notation3;
      return XML_ROLE_NONE;
    }
    if (XmlNameMatchesAscii(enc, ptr, "PUBLIC")) {
      state->handler = notation2;
      return XML_ROLE_NONE;
    }
    break;
  }
  return syntaxError(state);
}

static
int notation2(PROLOG_STATE *state,
	      int tok,
	      const char *ptr,
	      const char *end,
	      const ENCODING *enc)
{
  switch (tok) {
  case XML_TOK_LITERAL:
    state->handler = notation3;
    return XML_ROLE_NOTATION_PUBLIC_ID;
  }
  return syntaxError(state);
}

static
int notation3(PROLOG_STATE *state,
	      int tok,
	      const char *ptr,
	      const char *end,
	      const ENCODING *enc)
{
  switch (tok) {
  case XML_TOK_LITERAL:
    state->handler = declClose;
    return XML_ROLE_NOTATION_SYSTEM_ID;
  }
  return syntaxError(state);
}

static
int attlist0(PROLOG_STATE *state,
	     int tok,
	     const char *ptr,
	     const char *end,
	     const ENCODING *enc)
{
  switch (tok) {
  case XML_TOK_NAME:
    state->handler = attlist1;
    return XML_ROLE_ATTLIST_ELEMENT_NAME;
  }
  return syntaxError(state);
}

static
int attlist1(PROLOG_STATE *state,
	     int tok,
	     const char *ptr,
	     const char *end,
	     const ENCODING *enc)
{
  switch (tok) {
  case XML_TOK_DECL_CLOSE:
    state->handler = internalSubset;
    return XML_ROLE_NONE;
  case XML_TOK_NAME:
    state->handler = attlist2;
    return XML_ROLE_ATTRIBUTE_NAME;
  }
  return syntaxError(state);
}

static
int attlist2(PROLOG_STATE *state,
	     int tok,
	     const char *ptr,
	     const char *end,
	     const ENCODING *enc)
{
  switch (tok) {
  case XML_TOK_NAME:
    {
      static const char *types[] = {
	"CDATA",
        "ID",
        "IDREF",
        "IDREFS",
        "ENTITY",
        "ENTITIES",
        "NMTOKEN",
        "NMTOKENS",
      };
      int i;
      for (i = 0; i < sizeof(types)/sizeof(types[0]); i++)
	if (XmlNameMatchesAscii(enc, ptr, types[i])) {
	  state->handler = attlist8;
	  return XML_ROLE_ATTRIBUTE_TYPE_CDATA + i;
	}
    }
    if (XmlNameMatchesAscii(enc, ptr, "NOTATION")) {
      state->handler = attlist5;
      return XML_ROLE_NONE;
    }
    break;
  case XML_TOK_OPEN_PAREN:
    state->handler = attlist3;
    return XML_ROLE_NONE;
  }
  return syntaxError(state);
}

static
int attlist3(PROLOG_STATE *state,
	     int tok,
	     const char *ptr,
	     const char *end,
	     const ENCODING *enc)
{
  switch (tok) {
  case XML_TOK_NMTOKEN:
  case XML_TOK_NAME:
    state->handler = attlist4;
    return XML_ROLE_ATTRIBUTE_ENUM_VALUE;
  }
  return syntaxError(state);
}

static
int attlist4(PROLOG_STATE *state,
	     int tok,
	     const char *ptr,
	     const char *end,
	     const ENCODING *enc)
{
  switch (tok) {
  case XML_TOK_CLOSE_PAREN:
    state->handler = attlist8;
    return XML_ROLE_NONE;
  case XML_TOK_OR:
    state->handler = attlist3;
    return XML_ROLE_NONE;
  }
  return syntaxError(state);
}

static
int attlist5(PROLOG_STATE *state,
	     int tok,
	     const char *ptr,
	     const char *end,
	     const ENCODING *enc)
{
  switch (tok) {
  case XML_TOK_OPEN_PAREN:
    state->handler = attlist6;
    return XML_ROLE_NONE;
  }
  return syntaxError(state);
}


static
int attlist6(PROLOG_STATE *state,
	     int tok,
	     const char *ptr,
	     const char *end,
	     const ENCODING *enc)
{
  switch (tok) {
  case XML_TOK_NAME:
    state->handler = attlist7;
    return XML_ROLE_ATTRIBUTE_NOTATION_VALUE;
  }
  return syntaxError(state);
}

static
int attlist7(PROLOG_STATE *state,
	     int tok,
	     const char *ptr,
	     const char *end,
	     const ENCODING *enc)
{
  switch (tok) {
  case XML_TOK_CLOSE_PAREN:
    state->handler = attlist8;
    return XML_ROLE_NONE;
  case XML_TOK_OR:
    state->handler = attlist6;
    return XML_ROLE_NONE;
  }
  return syntaxError(state);
}

/* default value */
static
int attlist8(PROLOG_STATE *state,
	     int tok,
	     const char *ptr,
	     const char *end,
	     const ENCODING *enc)
{
  switch (tok) {
  case XML_TOK_POUND_NAME:
    if (XmlNameMatchesAscii(enc,
			    ptr + enc->minBytesPerChar,
			    "IMPLIED")) {
      state->handler = attlist1;
      return XML_ROLE_IMPLIED_ATTRIBUTE_VALUE;
    }
    if (XmlNameMatchesAscii(enc,
			    ptr + enc->minBytesPerChar,
			    "REQUIRED")) {
      state->handler = attlist1;
      return XML_ROLE_REQUIRED_ATTRIBUTE_VALUE;
    }
    if (XmlNameMatchesAscii(enc,
			    ptr + enc->minBytesPerChar,
			    "FIXED")) {
      state->handler = attlist9;
      return XML_ROLE_NONE;
    }
    break;
  case XML_TOK_LITERAL:
    state->handler = attlist1;
    return XML_ROLE_DEFAULT_ATTRIBUTE_VALUE;
  }
  return syntaxError(state);
}

static
int attlist9(PROLOG_STATE *state,
	     int tok,
	     const char *ptr,
	     const char *end,
	     const ENCODING *enc)
{
  switch (tok) {
  case XML_TOK_LITERAL:
    state->handler = attlist1;
    return XML_ROLE_FIXED_ATTRIBUTE_VALUE;
  }
  return syntaxError(state);
}

static
int element0(PROLOG_STATE *state,
	     int tok,
	     const char *ptr,
	     const char *end,
	     const ENCODING *enc)
{
  switch (tok) {
  case XML_TOK_NAME:
    state->handler = element1;
    return XML_ROLE_ELEMENT_NAME;
  }
  return syntaxError(state);
}

static
int element1(PROLOG_STATE *state,
	     int tok,
	     const char *ptr,
	     const char *end,
	     const ENCODING *enc)
{
  switch (tok) {
  case XML_TOK_NAME:
    if (XmlNameMatchesAscii(enc, ptr, "EMPTY")) {
      state->handler = declClose;
      return XML_ROLE_CONTENT_EMPTY;
    }
    if (XmlNameMatchesAscii(enc, ptr, "ANY")) {
      state->handler = declClose;
      return XML_ROLE_CONTENT_ANY;
    }
    break;
  case XML_TOK_OPEN_PAREN:
    state->handler = element2;
    return XML_ROLE_CONTENT_GROUP_OPEN;
  }
  return syntaxError(state);
}

static
int element2(PROLOG_STATE *state,
	     int tok,
	     const char *ptr,
	     const char *end,
	     const ENCODING *enc)
{
  switch (tok) {
  case XML_TOK_POUND_NAME:
    if (XmlNameMatchesAscii(enc,
			    ptr + enc->minBytesPerChar,
			    "PCDATA")) {
      state->handler = element3;
      return XML_ROLE_CONTENT_PCDATA;
    }
    break;
  case XML_TOK_OPEN_PAREN:
    state->level = 2;
    state->handler = element6;
    return XML_ROLE_CONTENT_GROUP_OPEN;
  case XML_TOK_NAME:
    state->level = 1;
    state->handler = element7;
    return XML_ROLE_CONTENT_ELEMENT;
  case XML_TOK_NAME_QUESTION:
    state->level = 1;
    state->handler = element7;
    return XML_ROLE_CONTENT_ELEMENT_OPT;
  case XML_TOK_NAME_ASTERISK:
    state->level = 1;
    state->handler = element7;
    return XML_ROLE_CONTENT_ELEMENT_REP;
  case XML_TOK_NAME_PLUS:
    state->level = 1;
    state->handler = element7;
    return XML_ROLE_CONTENT_ELEMENT_PLUS;
  }
  return syntaxError(state);
}

static
int element3(PROLOG_STATE *state,
	     int tok,
	     const char *ptr,
	     const char *end,
	     const ENCODING *enc)
{
  switch (tok) {
  case XML_TOK_CLOSE_PAREN:
  case XML_TOK_CLOSE_PAREN_ASTERISK:
    state->handler = declClose;
    return XML_ROLE_END_GROUP_REP;
  case XML_TOK_OR:
    state->handler = element4;
    return XML_ROLE_NONE;
  }
  return syntaxError(state);
}

static
int element4(PROLOG_STATE *state,
	     int tok,
	     const char *ptr,
	     const char *end,
	     const ENCODING *enc)
{
  switch (tok) {
  case XML_TOK_NAME:
    state->handler = element5;
    return XML_ROLE_CONTENT_ELEMENT;
  }
  return syntaxError(state);
}

static
int element5(PROLOG_STATE *state,
	     int tok,
	     const char *ptr,
	     const char *end,
	     const ENCODING *enc)
{
  switch (tok) {
  case XML_TOK_CLOSE_PAREN_ASTERISK:
    state->handler = declClose;
    return XML_ROLE_END_GROUP_REP;
  case XML_TOK_OR:
    state->handler = element4;
    return XML_ROLE_NONE;
  }
  return syntaxError(state);
}

static
int element6(PROLOG_STATE *state,
	     int tok,
	     const char *ptr,
	     const char *end,
	     const ENCODING *enc)
{
  switch (tok) {
  case XML_TOK_OPEN_PAREN:
    state->level += 1;
    return XML_ROLE_CONTENT_GROUP_OPEN;
  case XML_TOK_NAME:
    state->handler = element7;
    return XML_ROLE_CONTENT_ELEMENT;
  case XML_TOK_NAME_QUESTION:
    state->handler = element7;
    return XML_ROLE_CONTENT_ELEMENT_OPT;
  case XML_TOK_NAME_ASTERISK:
    state->handler = element7;
    return XML_ROLE_CONTENT_ELEMENT_REP;
  case XML_TOK_NAME_PLUS:
    state->handler = element7;
    return XML_ROLE_CONTENT_ELEMENT_PLUS;
  }
  return syntaxError(state);
}

static
int element7(PROLOG_STATE *state,
	     int tok,
	     const char *ptr,
	     const char *end,
	     const ENCODING *enc)
{
  switch (tok) {
  case XML_TOK_CLOSE_PAREN:
    state->level -= 1;
    if (state->level == 0)
      state->handler = declClose;
    return XML_ROLE_END_GROUP;
  case XML_TOK_CLOSE_PAREN_ASTERISK:
    state->level -= 1;
    if (state->level == 0)
      state->handler = declClose;
    return XML_ROLE_END_GROUP_REP;
  case XML_TOK_CLOSE_PAREN_QUESTION:
    state->level -= 1;
    if (state->level == 0)
      state->handler = declClose;
    return XML_ROLE_END_GROUP_OPT;
  case XML_TOK_CLOSE_PAREN_PLUS:
    state->level -= 1;
    if (state->level == 0)
      state->handler = declClose;
    return XML_ROLE_END_GROUP_PLUS;
  case XML_TOK_COMMA:
    state->handler = element6;
    return XML_ROLE_GROUP_SEQUENCE;
  case XML_TOK_OR:
    state->handler = element6;
    return XML_ROLE_GROUP_CHOICE;
  }
  return syntaxError(state);
}

static
int declClose(PROLOG_STATE *state,
	      int tok,
	      const char *ptr,
	      const char *end,
	      const ENCODING *enc)
{
  switch (tok) {
  case XML_TOK_DECL_CLOSE:
    state->handler = internalSubset;
    return XML_ROLE_NONE;
  }
  return syntaxError(state);
}

static
int ignore(PROLOG_STATE *state,
	   int tok,
	   const char *ptr,
	   const char *end,
	   const ENCODING *enc)
{
  switch (tok) {
  case XML_TOK_DECL_CLOSE:
    state->handler = internalSubset;
    return 0;
  default:
    return XML_ROLE_NONE;
  }
  return syntaxError(state);
}

static
int error(PROLOG_STATE *state,
	   int tok,
	   const char *ptr,
	   const char *end,
	   const ENCODING *enc)
{
  return XML_ROLE_NONE;
}

static
int syntaxError(PROLOG_STATE *state)
{
  state->handler = error;
  return XML_ROLE_ERROR;
}

void XmlPrologStateInit(PROLOG_STATE *state)
{
  state->handler = prolog0;
}
