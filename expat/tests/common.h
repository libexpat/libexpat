/* Commonly used functions and macros for the Expat test suite
                            __  __            _
                         ___\ \/ /_ __   __ _| |_
                        / _ \\  /| '_ \ / _` | __|
                       |  __//  \| |_) | (_| | |_
                        \___/_/\_\ .__/ \__,_|\__|
                                 |_| XML parser

   Copyright (c) 2001-2006 Fred L. Drake, Jr. <fdrake@users.sourceforge.net>
   Copyright (c) 2003      Greg Stein <gstein@users.sourceforge.net>
   Copyright (c) 2005-2007 Steven Solie <steven@solie.ca>
   Copyright (c) 2005-2012 Karl Waclawek <karl@waclawek.net>
   Copyright (c) 2016-2022 Sebastian Pipping <sebastian@pipping.org>
   Copyright (c) 2017-2018 Rhodri James <rhodri@wildebeest.org.uk>
   Copyright (c) 2017      Joe Orton <jorton@redhat.com>
   Copyright (c) 2017      José Gutiérrez de la Concha <jose@zeroc.com>
   Copyright (c) 2018      Marco Maggi <marco.maggi-ipsu@poste.it>
   Copyright (c) 2019      David Loffredo <loffredo@steptools.com>
   Copyright (c) 2020      Tim Gates <tim.gates@iress.com>
   Copyright (c) 2021      Dong-hee Na <donghee.na@python.org>
   Licensed under the MIT license:

   Permission is  hereby granted,  free of charge,  to any  person obtaining
   a  copy  of  this  software   and  associated  documentation  files  (the
   "Software"),  to  deal in  the  Software  without restriction,  including
   without  limitation the  rights  to use,  copy,  modify, merge,  publish,
   distribute, sublicense, and/or sell copies of the Software, and to permit
   persons  to whom  the Software  is  furnished to  do so,  subject to  the
   following conditions:

   The above copyright  notice and this permission notice  shall be included
   in all copies or substantial portions of the Software.

   THE  SOFTWARE  IS  PROVIDED  "AS  IS",  WITHOUT  WARRANTY  OF  ANY  KIND,
   EXPRESS  OR IMPLIED,  INCLUDING  BUT  NOT LIMITED  TO  THE WARRANTIES  OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN
   NO EVENT SHALL THE AUTHORS OR  COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
   DAMAGES OR  OTHER LIABILITY, WHETHER  IN AN  ACTION OF CONTRACT,  TORT OR
   OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
   USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#ifdef __cplusplus
extern "C" {
#endif

#ifndef XML_COMMON_H
#  define XML_COMMON_H

#ifdef XML_LARGE_SIZE
#  define XML_FMT_INT_MOD "ll"
#else
#  define XML_FMT_INT_MOD "l"
#endif

#ifdef XML_UNICODE_WCHAR_T
#  define XML_FMT_CHAR "lc"
#  define XML_FMT_STR "ls"
#  include <wchar.h>
#  define xcstrlen(s) wcslen(s)
#  define xcstrcmp(s, t) wcscmp((s), (t))
#  define xcstrncmp(s, t, n) wcsncmp((s), (t), (n))
#  define XCS(s) _XCS(s)
#  define _XCS(s) L##s
#else
#  ifdef XML_UNICODE
#    error "No support for UTF-16 character without wchar_t in tests"
#  else
#    define XML_FMT_CHAR "c"
#    define XML_FMT_STR "s"
#    define xcstrlen(s) strlen(s)
#    define xcstrcmp(s, t) strcmp((s), (t))
#    define xcstrncmp(s, t, n) strncmp((s), (t), (n))
#    define XCS(s) s
#  endif /* XML_UNICODE */
#endif   /* XML_UNICODE_WCHAR_T */


extern const char *long_character_data_text;

extern XML_Parser g_parser;

extern XML_Bool g_resumable;


/* Support structure for a generic external entity handler.  The
 * handler function external_entity_optioner() expects its userdata to
 * be an array of these structures, terminated by a structure with all
 * NULL pointers.
 */
typedef struct ExtOption {
  const XML_Char *system_id;
  const char *parse_text;
} ExtOption;


/* A variant of tcase_add_test() that only adds the test to the test case
 * if the compile-time symbol XML_DTD is defined.
 */
extern void tcase_add_test__ifdef_xml_dtd(TCase *tc, tcase_test_function test);

extern void basic_setup(void);
extern void basic_teardown(void);

/* Generate a failure using the parser state to create an error message;
   this should be used when the parser reports an error we weren't
   expecting.
*/
extern void _xml_failure(XML_Parser parser, const char *file, int line);

#define xml_failure(parser) _xml_failure((parser), __FILE__, __LINE__)

/* Feed the given text to the parser one byte at a time
 */
extern enum XML_Status
_XML_Parse_SINGLE_BYTES(XML_Parser parser, const char *s, int len,
                        int isFinal);

/* Parse the text, and indicate failure if the given error is not returned
 */
extern void
_expect_failure(const char *text, enum XML_Error errorCode,
                const char *errorMessage, const char *file, int lineno);

#define expect_failure(text, errorCode, errorMessage)                   \
  _expect_failure((text), (errorCode), (errorMessage), __FILE__, __LINE__)

/* Generic simple external entity handler.  Expects an array of
 * ExtOption structures as its userdata, terminated by an entry with
 * NULL pointers.  It will match the systemId it is invoked with to an
 * entry in the userdata, and parse the corresponding parse_text.
 */
extern int XMLCALL
external_entity_optioner(XML_Parser parser, const XML_Char *context,
                         const XML_Char *base, const XML_Char *systemId,
                         const XML_Char *publicId);

/*
 * Parameter entity evaluation support.
 */
#define ENTITY_MATCH_FAIL (-1)
#define ENTITY_MATCH_NOT_FOUND (0)
#define ENTITY_MATCH_SUCCESS (1)

extern void
param_entity_match_init(const XML_Char *name, const XML_Char *value);

extern void XMLCALL
param_entity_match_handler(void *userData, const XML_Char *entityName,
                           int is_parameter_entity, const XML_Char *value,
                           int value_length, const XML_Char *base,
                           const XML_Char *systemId, const XML_Char *publicId,
                           const XML_Char *notationName);

extern int
get_param_entity_match_flag(void);

/*
 * Support functions for handlers to collect up character and attribute
 * data.
 */
extern void XMLCALL
accumulate_characters(void *userData, const XML_Char *s, int len);

extern void XMLCALL
accumulate_attribute(void *userData, const XML_Char *name,
                     const XML_Char **atts);

extern void
_run_character_check(const char *text, const XML_Char *expected,
                     const char *file, int line);

#define run_character_check(text, expected)                                    \
  _run_character_check(text, expected, __FILE__, __LINE__)

extern void
_run_attribute_check(const char *text, const XML_Char *expected,
                     const char *file, int line);

#define run_attribute_check(text, expected)                                    \
  _run_attribute_check(text, expected, __FILE__, __LINE__)

typedef struct ExtTest {
  const char *parse_text;
  const XML_Char *encoding;
  CharData *storage;
} ExtTest;

extern void XMLCALL
ext_accumulate_characters(void *userData, const XML_Char *s, int len);

extern void
_run_ext_character_check(const char *text, ExtTest *test_data,
                         const XML_Char *expected, const char *file, int line);

#define run_ext_character_check(text, test_data, expected)                     \
  _run_ext_character_check(text, test_data, expected, __FILE__, __LINE__)

/* Return true if whitespace has been normalized in a string, using
   the rules for attribute value normalization.  The 'is_cdata' flag
   is needed since CDATA attributes don't need to have multiple
   whitespace characters collapsed to a single space, while other
   attribute data types do.  (Section 3.3.3 of the recommendation.)
*/
extern int
is_whitespace_normalized(const XML_Char *s, int is_cdata);


#endif /* XML_COMMON_H */

#ifdef __cplusplus
}
#endif
