/* Copyright (c) 1998, 1999 Thai Open Source Software Center Ltd
See the file copying.txt for copying permission. */
#ifdef __cplusplus
extern "C" {
#endif

int XML_URLInit();
void XML_URLUninit();
int XML_ProcessURL(XML_Parser parser,
		   const XML_Char *url,
		   unsigned flags);

#ifdef __cplusplus
}
#endif
