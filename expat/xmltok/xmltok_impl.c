#define DO_LEAD_CASE(n, ptr, end, ret) \
    case BT_LEAD ## n: \
      if (end - ptr < n) \
	return ret; \
      ptr += n; \
      break;
#define MULTIBYTE_CASES(ptr, end, ret) \
  DO_LEAD_CASE(2, ptr, end, ret) \
  DO_LEAD_CASE(3, ptr, end, ret) \
  DO_LEAD_CASE(4, ptr, end, ret) \
  DO_LEAD_CASE(5, ptr, end, ret) \
  DO_LEAD_CASE(6, ptr, end, ret)


#define INVALID_CASES(ptr, nextTokPtr) \
  case BT_NONXML: \
  case BT_MALFORM: \
  case BT_TRAIL: \
    *(nextTokPtr) = (ptr); \
    return XML_TOK_INVALID;

#define CHECK_NAME_CASE(n, enc, ptr, end, nextTokPtr) \
   case BT_LEAD ## n: \
     if (end - ptr < n) \
       return XML_TOK_PARTIAL_CHAR; \
     if (!IS_NAME_CHAR(enc, ptr, n)) { \
       *nextTokPtr = ptr; \
       return XML_TOK_INVALID; \
     } \
     ptr += n; \
     break;

#define CHECK_NAME_CASES(enc, ptr, end, nextTokPtr) \
  case BT_NONASCII: \
    if (!IS_NAME_CHAR(enc, ptr, MINBPC)) { \
      *nextTokPtr = ptr; \
      return XML_TOK_INVALID; \
    } \
  case BT_NMSTRT: \
  case BT_HEX: \
  case BT_DIGIT: \
  case BT_NAME: \
  case BT_MINUS: \
    ptr += MINBPC; \
    break; \
  CHECK_NAME_CASE(2, enc, ptr, end, nextTokPtr) \
  CHECK_NAME_CASE(3, enc, ptr, end, nextTokPtr) \
  CHECK_NAME_CASE(4, enc, ptr, end, nextTokPtr) \
  CHECK_NAME_CASE(5, enc, ptr, end, nextTokPtr) \
  CHECK_NAME_CASE(6, enc, ptr, end, nextTokPtr)

#define CHECK_NMSTRT_CASE(n, enc, ptr, end, nextTokPtr) \
   case BT_LEAD ## n: \
     if (end - ptr < n) \
       return XML_TOK_PARTIAL_CHAR; \
     if (!IS_NMSTRT_CHAR(enc, ptr, n)) { \
       *nextTokPtr = ptr; \
       return XML_TOK_INVALID; \
     } \
     ptr += n; \
     break;

#define CHECK_NMSTRT_CASES(enc, ptr, end, nextTokPtr) \
  case BT_NONASCII: \
    if (!IS_NMSTRT_CHAR(enc, ptr, MINBPC)) { \
      *nextTokPtr = ptr; \
      return XML_TOK_INVALID; \
    } \
  case BT_NMSTRT: \
  case BT_HEX: \
    ptr += MINBPC; \
    break; \
  CHECK_NMSTRT_CASE(2, enc, ptr, end, nextTokPtr) \
  CHECK_NMSTRT_CASE(3, enc, ptr, end, nextTokPtr) \
  CHECK_NMSTRT_CASE(4, enc, ptr, end, nextTokPtr) \
  CHECK_NMSTRT_CASE(5, enc, ptr, end, nextTokPtr) \
  CHECK_NMSTRT_CASE(6, enc, ptr, end, nextTokPtr)

#ifndef PREFIX
#define PREFIX(ident) ident
#endif

/* ptr points to character following "<!-" */

static
int PREFIX(scanComment)(const ENCODING *enc, const char *ptr, const char *end,
			const char **nextTokPtr)
{
  if (ptr != end) {
    if (!CHAR_MATCHES(enc, ptr, '-')) {
      *nextTokPtr = ptr;
      return XML_TOK_INVALID;
    }
    ptr += MINBPC;
    while (ptr != end) {
      switch (BYTE_TYPE(enc, ptr)) {
      MULTIBYTE_CASES(ptr, end, XML_TOK_PARTIAL_CHAR)
      INVALID_CASES(ptr, nextTokPtr)
      case BT_MINUS:
	if ((ptr += MINBPC) == end)
	  return XML_TOK_PARTIAL;
	if (CHAR_MATCHES(enc, ptr, '-')) {
	  if ((ptr += MINBPC) == end)
	    return XML_TOK_PARTIAL;
	  if (!CHAR_MATCHES(enc, ptr, '>')) {
	    *nextTokPtr = ptr;
	    return XML_TOK_INVALID;
	  }
	  *nextTokPtr = ptr + MINBPC;
	  return XML_TOK_COMMENT;
	}
	/* fall through */
      default:
	ptr += MINBPC;
	break;
      }
    }
  }
  return XML_TOK_PARTIAL;
}

/* ptr points to character following "<!" */

static
int PREFIX(scanDecl)(const ENCODING *enc, const char *ptr, const char *end,
		     const char **nextTokPtr)
{
  if (ptr != end) {
    if (BYTE_TYPE(enc, ptr) == BT_MINUS)
      return PREFIX(scanComment)(enc, ptr + MINBPC, end, nextTokPtr);
    do {
      switch (BYTE_TYPE(enc, ptr)) {
      MULTIBYTE_CASES(ptr, end, (*nextTokPtr = ptr, XML_TOK_PROLOG_CHARS))
      INVALID_CASES(ptr, nextTokPtr)
      case BT_APOS:
      case BT_QUOT:
      case BT_LT:
	*nextTokPtr = ptr;
	return XML_TOK_PROLOG_CHARS;
      }
    } while ((ptr += MINBPC) != end);
    *nextTokPtr = ptr;
    return XML_TOK_PROLOG_CHARS;
  }
  return XML_TOK_PARTIAL;
}

/* ptr points to character following "<?" */

static
int PREFIX(scanPi)(const ENCODING *enc, const char *ptr, const char *end,
		   const char **nextTokPtr)
{
  if (ptr == end)
    return XML_TOK_PARTIAL;
  switch (BYTE_TYPE(enc, ptr)) {
  CHECK_NMSTRT_CASES(enc, ptr, end, nextTokPtr)
  default:
    *nextTokPtr = ptr;
    return XML_TOK_INVALID;
  }
  while (ptr != end) {
    switch (BYTE_TYPE(enc, ptr)) {
    CHECK_NAME_CASES(enc, ptr, end, nextTokPtr)
    case BT_S: case BT_CR: case BT_LF:
      ptr += MINBPC;
      while (ptr != end) {
        switch (BYTE_TYPE(enc, ptr)) {
        MULTIBYTE_CASES(ptr, end, XML_TOK_PARTIAL_CHAR)
        INVALID_CASES(ptr, nextTokPtr)
	case BT_QUEST:
	  ptr += MINBPC;
	  if (ptr == end)
	    return XML_TOK_PARTIAL;
	  if (CHAR_MATCHES(enc, ptr, '>')) {
	    *nextTokPtr = ptr + MINBPC;
	    return XML_TOK_PI;
	  }
	  break;
	default:
	  ptr += MINBPC;
	  break;
	}
      }
      return XML_TOK_PARTIAL;
    case BT_QUEST:
      ptr += MINBPC;
      if (ptr == end)
	return XML_TOK_PARTIAL;
      if (CHAR_MATCHES(enc, ptr, '>')) {
	*nextTokPtr = ptr + MINBPC;
	return XML_TOK_PI;
      }
      /* fall through */
    default:
      *nextTokPtr = ptr;
      return XML_TOK_INVALID;
    }
  }
  return XML_TOK_PARTIAL;
}

/* ptr points to character following "<![" */

static
int PREFIX(scanCdataSection)(const ENCODING *enc, const char *ptr, const char *end,
			     const char **nextTokPtr)
{
  int i;
  /* CDATA[]]> */
  if (end - ptr < 9 * MINBPC)
    return XML_TOK_PARTIAL;
  for (i = 0; i < 6; i++, ptr += MINBPC) {
    if (!CHAR_MATCHES(enc, ptr, "CDATA["[i])) {
      *nextTokPtr = ptr;
      return XML_TOK_INVALID;
    }
  }
  end -= 2 * MINBPC;
  while (ptr != end) {
    switch (BYTE_TYPE(enc, ptr)) {
    MULTIBYTE_CASES(ptr, end, XML_TOK_PARTIAL_CHAR)
    INVALID_CASES(ptr, nextTokPtr)
    case BT_RSQB:
      if (CHAR_MATCHES(enc, ptr + MINBPC, ']')
	  && CHAR_MATCHES(enc, ptr + 2 * MINBPC, '>')) {
	*nextTokPtr = ptr + 3 * MINBPC;
	return XML_TOK_CDATA_SECTION;
      }
    /* fall through */
    default:
      ptr += MINBPC;
    }
  }
  return XML_TOK_PARTIAL;
}

/* ptr points to character following "</" */

static
int PREFIX(scanEndTag)(const ENCODING *enc, const char *ptr, const char *end,
		       const char **nextTokPtr)
{
  if (ptr == end)
    return XML_TOK_PARTIAL;
  switch (BYTE_TYPE(enc, ptr)) {
  CHECK_NMSTRT_CASES(enc, ptr, end, nextTokPtr)
  default:
    *nextTokPtr = ptr;
    return XML_TOK_INVALID;
  }
  while (ptr != end) {
    switch (BYTE_TYPE(enc, ptr)) {
    CHECK_NAME_CASES(enc, ptr, end, nextTokPtr)
    case BT_S: case BT_CR: case BT_LF:
      for (ptr += MINBPC; ptr != end; ptr += MINBPC) {
	switch (BYTE_TYPE(enc, ptr)) {
	case BT_S: case BT_CR: case BT_LF:
	  break;
	case BT_GT:
	  *nextTokPtr = ptr + MINBPC;
          return XML_TOK_END_TAG;
	default:
	  *nextTokPtr = ptr;
	  return XML_TOK_INVALID;
	}
      }
      return XML_TOK_PARTIAL;
    case BT_GT:
      *nextTokPtr = ptr + MINBPC;
      return XML_TOK_END_TAG;
    default:
      *nextTokPtr = ptr;
      return XML_TOK_INVALID;
    }
  }
  return XML_TOK_PARTIAL;
}

/* ptr points to character following "&#X" */

static
int PREFIX(scanHexCharRef)(const ENCODING *enc, const char *ptr, const char *end,
			   const char **nextTokPtr)
{
  if (ptr != end) {
    switch (BYTE_TYPE(enc, ptr)) {
    case BT_DIGIT:
    case BT_HEX:
      break;
    default:
      *nextTokPtr = ptr;
      return XML_TOK_INVALID;
    }
    for (ptr += MINBPC; ptr != end; ptr += MINBPC) {
      switch (BYTE_TYPE(enc, ptr)) {
      case BT_DIGIT:
      case BT_HEX:
	break;
      case BT_SEMI:
	*nextTokPtr = ptr + MINBPC;
	return XML_TOK_CHAR_REF;
      default:
	*nextTokPtr = ptr;
	return XML_TOK_INVALID;
      }
    }
  }
  return XML_TOK_PARTIAL;
}

/* ptr points to character following "&#" */

static
int PREFIX(scanCharRef)(const ENCODING *enc, const char *ptr, const char *end,
			const char **nextTokPtr)
{
  if (ptr != end) {
    if (CHAR_MATCHES(enc, ptr, 'x'))
      return PREFIX(scanHexCharRef)(enc, ptr + MINBPC, end, nextTokPtr);
    switch (BYTE_TYPE(enc, ptr)) {
    case BT_DIGIT:
      break;
    default:
      *nextTokPtr = ptr;
      return XML_TOK_INVALID;
    }
    for (ptr += MINBPC; ptr != end; ptr += MINBPC) {
      switch (BYTE_TYPE(enc, ptr)) {
      case BT_DIGIT:
	break;
      case BT_SEMI:
	*nextTokPtr = ptr + MINBPC;
	return XML_TOK_CHAR_REF;
      default:
	*nextTokPtr = ptr;
	return XML_TOK_INVALID;
      }
    }
  }
  return XML_TOK_PARTIAL;
}

/* ptr points to character following "&" */

static
int PREFIX(scanRef)(const ENCODING *enc, const char *ptr, const char *end,
		    const char **nextTokPtr)
{
  if (ptr == end)
    return XML_TOK_PARTIAL;
  switch (BYTE_TYPE(enc, ptr)) {
  CHECK_NMSTRT_CASES(end, ptr, end, nextTokPtr)
  case BT_NUM:
    return PREFIX(scanCharRef)(enc, ptr + MINBPC, end, nextTokPtr);
  default:
    *nextTokPtr = ptr;
    return XML_TOK_INVALID;
  }
  while (ptr != end) {
    switch (BYTE_TYPE(enc, ptr)) {
    CHECK_NAME_CASES(enc, ptr, end, nextTokPtr)
    case BT_SEMI:
      *nextTokPtr = ptr + MINBPC;
      return XML_TOK_ENTITY_REF;
    default:
      *nextTokPtr = ptr;
      return XML_TOK_INVALID;
    }
  }
  return XML_TOK_PARTIAL;
}

/* ptr points to character following first character of attribute name */

static
int PREFIX(scanAtts)(const ENCODING *enc, const char *ptr, const char *end,
		     const char **nextTokPtr)
{
  while (ptr != end) {
    switch (BYTE_TYPE(enc, ptr)) {
    CHECK_NAME_CASES(enc, ptr, end, nextTokPtr)
    case BT_S: case BT_CR: case BT_LF:
      for (;;) {
	int t;

	ptr += MINBPC;
	if (ptr == end)
	  return XML_TOK_PARTIAL;
	t = BYTE_TYPE(enc, ptr);
	if (t == BT_EQUALS)
	  break;
	switch (t) {
	case BT_S:
	case BT_LF:
	case BT_CR:
	  break;
	default:
	  *nextTokPtr = ptr;
	  return XML_TOK_INVALID;
	}
      }
    /* fall through */
    case BT_EQUALS:
      {
	int open;
	for (;;) {
	  
	  ptr += MINBPC;
	  if (ptr == end)
	    return XML_TOK_PARTIAL;
	  open = BYTE_TYPE(enc, ptr);
	  if (open == BT_QUOT || open == BT_APOS)
	    break;
	  switch (open) {
	  case BT_S:
	  case BT_LF:
	  case BT_CR:
	    break;
	  default:
	    *nextTokPtr = ptr;
	    return XML_TOK_INVALID;
	  }
	}
	ptr += MINBPC;
	/* in attribute value */
	for (;;) {
	  int t;
	  if (ptr == end)
	    return XML_TOK_PARTIAL;
	  t = BYTE_TYPE(enc, ptr);
	  if (t == open)
	    break;
	  switch (t) {
	  INVALID_CASES(ptr, nextTokPtr)
          MULTIBYTE_CASES(ptr, end, XML_TOK_PARTIAL_CHAR)
	  case BT_AMP:
	    {
	      int tok = PREFIX(scanRef)(enc, ptr + MINBPC, end, &ptr);
	      if (tok <= 0) {
		if (tok == XML_TOK_INVALID)
		  *nextTokPtr = ptr;
		return tok;
	      }
	      break;
	    }
	  case BT_LT:
	    *nextTokPtr = ptr;
	    return XML_TOK_INVALID;
	  default:
	    ptr += MINBPC;
	    break;
	  }
	}
	/* ptr points to closing quote */
	for (;;) {
	  ptr += MINBPC;
	  if (ptr == end)
	    return XML_TOK_PARTIAL;
	  switch (BYTE_TYPE(enc, ptr)) {
	  CHECK_NMSTRT_CASES(enc, ptr, end, nextTokPtr)
	  case BT_S: case BT_CR: case BT_LF:
	    continue;
	  case BT_GT:
	    *nextTokPtr = ptr + MINBPC;
	    return XML_TOK_START_TAG_WITH_ATTS;
	  case BT_SOL:
	    ptr += MINBPC;
	    if (ptr == end)
	      return XML_TOK_PARTIAL;
	    if (!CHAR_MATCHES(enc, ptr, '>')) {
	      *nextTokPtr = ptr;
	      return XML_TOK_INVALID;
	    }
	    *nextTokPtr = ptr + MINBPC;
	    return XML_TOK_EMPTY_ELEMENT_WITH_ATTS;
	  default:
	    *nextTokPtr = ptr;
	    return XML_TOK_INVALID;
	  }
	  break;
	}
	break;
      }
    default:
      *nextTokPtr = ptr;
      return XML_TOK_INVALID;
    }
  }
  return XML_TOK_PARTIAL;
}

/* ptr points to character following "<" */

static
int PREFIX(scanLt)(const ENCODING *enc, const char *ptr, const char *end,
		   const char **nextTokPtr)
{
  if (ptr == end)
    return XML_TOK_PARTIAL;
  switch (BYTE_TYPE(enc, ptr)) {
  CHECK_NMSTRT_CASES(enc, ptr, end, nextTokPtr)
  case BT_EXCL:
    if ((ptr += MINBPC) == end)
      return XML_TOK_PARTIAL;
    switch (BYTE_TYPE(enc, ptr)) {
    case BT_MINUS:
      return PREFIX(scanComment)(enc, ptr + MINBPC, end, nextTokPtr);
    case BT_LSQB:
      return PREFIX(scanCdataSection)(enc, ptr + MINBPC, end, nextTokPtr);
    }
    *nextTokPtr = ptr;
    return XML_TOK_INVALID;
  case BT_QUEST:
    return PREFIX(scanPi)(enc, ptr + MINBPC, end, nextTokPtr);
  case BT_SOL:
    return PREFIX(scanEndTag)(enc, ptr + MINBPC, end, nextTokPtr);
  default:
    *nextTokPtr = ptr;
    return XML_TOK_INVALID;
  }
  /* we have a start-tag */
  while (ptr != end) {
    switch (BYTE_TYPE(enc, ptr)) {
    CHECK_NAME_CASES(enc, ptr, end, nextTokPtr)
    case BT_S: case BT_CR: case BT_LF:
      {
        ptr += MINBPC;
	while (ptr != end) {
	  switch (BYTE_TYPE(enc, ptr)) {
	  CHECK_NMSTRT_CASES(enc, ptr, end, nextTokPtr)
	  case BT_GT:
	    goto gt;
	  case BT_SOL:
	    goto sol;
	  case BT_S: case BT_CR: case BT_LF:
	    ptr += MINBPC;
	    continue;
	  default:
	    *nextTokPtr = ptr;
	    return XML_TOK_INVALID;
	  }
	  return PREFIX(scanAtts)(enc, ptr, end, nextTokPtr);
	}
	return XML_TOK_PARTIAL;
      }
    case BT_GT:
    gt:
      *nextTokPtr = ptr + MINBPC;
      return XML_TOK_START_TAG_NO_ATTS;
    case BT_SOL:
    sol:
      ptr += MINBPC;
      if (ptr == end)
	return XML_TOK_PARTIAL;
      if (!CHAR_MATCHES(enc, ptr, '>')) {
	*nextTokPtr = ptr;
	return XML_TOK_INVALID;
      }
      *nextTokPtr = ptr + MINBPC;
      return XML_TOK_EMPTY_ELEMENT_NO_ATTS;
    default:
      *nextTokPtr = ptr;
      return XML_TOK_INVALID;
    }
  }
  return XML_TOK_PARTIAL;
}

static
int PREFIX(contentTok)(const ENCODING *enc, const char *ptr, const char *end,
		       const char **nextTokPtr)
{
  if (ptr == end)
    return XML_TOK_NONE;
#if MINBPC > 1
  {
    size_t n = end - ptr;
    if (n & (MINBPC - 1)) {
      n &= ~(MINBPC - 1);
      if (n == 0)
	return XML_TOK_PARTIAL;
      end = ptr + n;
    }
  }
#endif
  switch (BYTE_TYPE(enc, ptr)) {
  case BT_LT:
    return PREFIX(scanLt)(enc, ptr + MINBPC, end, nextTokPtr);
  case BT_AMP:
    return PREFIX(scanRef)(enc, ptr + MINBPC, end, nextTokPtr);
  case BT_RSQB:
    ptr += MINBPC;
    if (ptr == end)
      return XML_TOK_PARTIAL;
    if (!CHAR_MATCHES(enc, ptr, ']'))
      break;
    ptr += MINBPC;
    if (ptr == end)
      return XML_TOK_PARTIAL;
    if (!CHAR_MATCHES(enc, ptr, '>')) {
      ptr -= MINBPC;
      break;
    }
    *nextTokPtr = ptr;
    return XML_TOK_INVALID;
  INVALID_CASES(ptr, nextTokPtr)
  MULTIBYTE_CASES(ptr, end, XML_TOK_PARTIAL_CHAR)
  default:
    ptr += MINBPC;
    break;
  }
  while (ptr != end) {
    switch (BYTE_TYPE(enc, ptr)) {
    MULTIBYTE_CASES(ptr, end, (*nextTokPtr = ptr, XML_TOK_DATA_CHARS))
    case BT_RSQB:
      if (ptr + MINBPC != end) {
	 if (!CHAR_MATCHES(enc, ptr + MINBPC, ']')) {
	   ptr += MINBPC;
	   break;
	 }
	 if (ptr + 2*MINBPC != end) {
	   if (!CHAR_MATCHES(enc, ptr + 2*MINBPC, '>')) {
	     ptr += MINBPC;
	     break;
	   }
	   *nextTokPtr = ptr + 2*MINBPC;
	   return XML_TOK_INVALID;
	 }
      }
      /* fall through */
    case BT_AMP:
    case BT_LT:
    case BT_NONXML:
    case BT_MALFORM:
    case BT_TRAIL:
      *nextTokPtr = ptr;
      return XML_TOK_DATA_CHARS;
    default:
      ptr += MINBPC;
      break;
    }
  }
  *nextTokPtr = ptr;
  return XML_TOK_DATA_CHARS;
}

static
int PREFIX(prologTok)(const ENCODING *enc, const char *ptr, const char *end,
		      const char **nextTokPtr)
{
  if (ptr == end)
    return XML_TOK_NONE;
#if MINBPC > 1
  {
    size_t n = end - ptr;
    if (n & (MINBPC - 1)) {
      n &= ~(MINBPC - 1);
      if (n == 0)
	return XML_TOK_PARTIAL;
      end = ptr + n;
    }
  }
#endif
  switch (BYTE_TYPE(enc, ptr)) {
  INVALID_CASES(ptr, nextTokPtr)
  MULTIBYTE_CASES(ptr, end, XML_TOK_PARTIAL_CHAR)
  case BT_QUOT:
    {
      for (ptr += MINBPC; ptr != end; ptr += MINBPC) {
	if (BYTE_TYPE(enc, ptr) == BT_QUOT) {
	  *nextTokPtr = ptr + MINBPC;
	  return XML_TOK_LITERAL;
	}
      }
      return XML_TOK_PARTIAL;
    }
  case BT_APOS:
    {
      for (ptr += MINBPC; ptr != end; ptr += MINBPC) {
	if (BYTE_TYPE(enc, ptr) == BT_APOS) {
	  *nextTokPtr = ptr + MINBPC;
	  return XML_TOK_LITERAL;
	}
      }
      return XML_TOK_PARTIAL;
    }
  case BT_LT:
    {
      ptr += MINBPC;
      if (ptr == end)
	return XML_TOK_PARTIAL;
      switch (BYTE_TYPE(enc, ptr)) {
      case BT_EXCL:
	return PREFIX(scanDecl)(enc, ptr + MINBPC, end, nextTokPtr);
      case BT_QUEST:
	return PREFIX(scanPi)(enc, ptr + MINBPC, end, nextTokPtr);
      case BT_NMSTRT:
      case BT_HEX:
      case BT_NONASCII:
      case BT_LEAD2:
      case BT_LEAD3:
      case BT_LEAD4:
      case BT_LEAD5:
	return PREFIX(contentTok)(enc, ptr - MINBPC, end, nextTokPtr);
      }
      *nextTokPtr = ptr;
      return XML_TOK_INVALID;
    }
  case BT_S: case BT_CR: case BT_LF:
    for (;;) {
      ptr += MINBPC;
      if (ptr == end)
	break;
      switch (BYTE_TYPE(enc, ptr)) {
      case BT_S: case BT_CR: case BT_LF:
	break;
      default:
	*nextTokPtr = ptr;
	return XML_TOK_PROLOG_S;
      }
    }
    *nextTokPtr = ptr;
    return XML_TOK_PROLOG_S;
  default:
    ptr += MINBPC;
    break;
  }
  for (; ptr != end;) {
    switch (BYTE_TYPE(enc, ptr)) {
    case BT_LT:
    case BT_QUOT:
    case BT_APOS:
    case BT_NONXML:
    case BT_MALFORM:
    case BT_TRAIL:
    case BT_S: case BT_CR: case BT_LF:
      *nextTokPtr = ptr;
      return XML_TOK_PROLOG_CHARS;
    MULTIBYTE_CASES(ptr, end, (*nextTokPtr = ptr, XML_TOK_PROLOG_CHARS))
    default:
      ptr += MINBPC;
      break;
    }
  }
  *nextTokPtr = ptr;
  return XML_TOK_PROLOG_CHARS;
}

/* This must only be called for a well-formed start-tag or empty element tag.
Returns the number of attributes.  Pointers to the names of up to the first
attsMax attributes are stored in atts. */
static
int PREFIX(getAtts)(const ENCODING *enc, const char *ptr,
		    int attsMax, const char **atts)
{
  enum { other, inName, inValue } state = inName;
  int nAtts = 0;
  int open;

  for (ptr += MINBPC;; ptr += MINBPC) {
    switch (BYTE_TYPE(enc, ptr)) {
#define START_NAME \
      if (state == other) { \
	if (nAtts < attsMax) \
	  atts[nAtts] = ptr; \
	++nAtts; \
	state = inName; \
      }
#define LEAD_CASE(n) \
    case BT_LEAD ## n: START_NAME ptr += (n - MINBPC); break;
    LEAD_CASE(2) LEAD_CASE(3) LEAD_CASE(4) LEAD_CASE(5) LEAD_CASE(6)
#undef LEAD_CASE
    case BT_NONASCII:
    case BT_NMSTRT:
    case BT_HEX:
      START_NAME
      break;
#undef START_NAME
    case BT_QUOT:
      if (state == other) {
        state = inValue;
        open = BT_QUOT;
      }
      else if (open == BT_QUOT)
        state = other;
      break;
    case BT_APOS:
      if (state == other) {
        state = inValue;
        open = BT_APOS;
      }
      else if (open == BT_APOS)
        state = other;
      break;
    case BT_S: case BT_CR: case BT_LF:
      /* This case ensures that the first attribute name is counted
         Apart from that we could just change state on the quote. */
      if (state == inName)
        state = other;
      break;
    case BT_GT:
    case BT_SOL:
      if (state != inValue)
	return nAtts;
      break;
    default:
      break;
    }
  }
  /* not reached */
}

static
int PREFIX(sameName)(const ENCODING *enc, const char *ptr1, const char *ptr2)
{
  for (;;) {
    switch (BYTE_TYPE(enc, ptr1)) {
#define LEAD_CASE(n) \
    case BT_LEAD ## n: \
      if (*ptr1++ != *ptr2++) \
	return 0;
    LEAD_CASE(6) LEAD_CASE(5) LEAD_CASE(4) LEAD_CASE(3) LEAD_CASE(2)
#undef LEAD_CASE
      /* fall through */
      if (*ptr1++ != *ptr2++)
	return 0;
      break;
    case BT_NONASCII:
    case BT_NMSTRT:
    case BT_HEX:
    case BT_DIGIT:
    case BT_NAME:
    case BT_MINUS:
      if (*ptr2 != *ptr1)
	return 0;
      ptr1 += MINBPC;
      ptr2 += MINBPC;
      break;
    default:
      if (*ptr1 == *ptr2)
	return 1;
      switch (BYTE_TYPE(enc, ptr2)) {
      case BT_LEAD2:
      case BT_LEAD3:
      case BT_LEAD4:
      case BT_LEAD5:
      case BT_LEAD6:
      case BT_NONASCII:
      case BT_NMSTRT:
      case BT_HEX:
      case BT_DIGIT:
      case BT_NAME:
      case BT_MINUS:
	return 0;
      default:
	return 1;
      }
    }
  }
  /* not reached */
}

static
void PREFIX(updatePosition)(const ENCODING *enc,
			    const char *ptr,
			    const char *end,
			    POSITION *pos)
{
  if (pos->ignoreInitialLF) {
    if (ptr == end)
      return;
    if (CHAR_MATCHES(enc, ptr, '\n'))
      ptr += MINBPC;
    pos->ignoreInitialLF = 0;
  }
  while (ptr != end) {
    switch (BYTE_TYPE(enc, ptr)) {
    MULTIBYTE_CASES(ptr, end, ;/* hack! */)
    case BT_LF:
      pos->columnNumber = (unsigned)-1;
      pos->lineNumber++;
      ptr += MINBPC;
      break;
    case BT_CR:
      pos->lineNumber++;
      ptr += MINBPC;
      if (ptr == end) {
	pos->ignoreInitialLF = 1;
	pos->columnNumber = 0;
	return;
      }
      pos->columnNumber = (unsigned)-1;
      if (CHAR_MATCHES(enc, ptr, '\n'))
	ptr += MINBPC;
      break;
    default:
      ptr += MINBPC;
      break;
    }
    pos->columnNumber++;
  }
}

#undef DO_LEAD_CASE
#undef MULTIBYTE_CASES
#undef INVALID_CASES
#undef CHECK_NAME_CASE
#undef CHECK_NAME_CASES
#undef CHECK_NMSTRT_CASE
#undef CHECK_NMSTRT_CASES
