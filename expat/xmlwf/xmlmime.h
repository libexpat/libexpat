/* Copyright (c) 1998, 1999 Thai Open Source Software Center Ltd
See the file copying.txt for copying permission. */

#ifdef __cplusplus
extern "C" {
#endif


#define CHARSET_MAX 41

buf contains the body of the header field (the part after "Content-Type:").
charset gets the charset to use.  It must be at least CHARSET_MAX chars long.
charset will be empty if the default charset should be used. */

void getXMLCharset(const char *buf, char *charset);

#ifdef __cplusplus
}
#endif
