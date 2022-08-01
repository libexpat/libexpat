/* None-dummy event handler functions for the Expat test suite.
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

#ifndef XML_HANDLERS_H
#  define XML_HANDLERS_H

/* Event handlers storing structured data */
#define STRUCT_START_TAG 0
#define STRUCT_END_TAG 1

extern void XMLCALL
start_element_event_handler2(void *userData, const XML_Char *name,
                             const XML_Char **attr);

extern void XMLCALL
end_element_event_handler2(void *userData, const XML_Char *name);

/* Event handlers storing character data */
extern void XMLCALL
start_element_event_handler(void *userData, const XML_Char *name,
                            const XML_Char **atts);

extern void XMLCALL
end_element_event_handler(void *userData, const XML_Char *name);

typedef struct attrInfo {
  const XML_Char *name;
  const XML_Char *value;
} AttrInfo;

typedef struct elementInfo {
  const XML_Char *name;
  int attr_count;
  const XML_Char *id_name;
  AttrInfo *attributes;
} ElementInfo;

/* This handler expects to be called with an ElementInfo pointer as its
 * user data, containing details of all the attributes and their values.
 * There is additional checking for the ID attribute, if present.
 */
extern void XMLCALL
counting_start_element_handler(void *userData, const XML_Char *name,
                               const XML_Char **atts);

/* Do-nothing handler for the text encoding named "unsupported-encoding" */
extern int XMLCALL
UnknownEncodingHandler(void *data, const XML_Char *encoding,
                       XML_Encoding *info);

/* Handler that errors for all not built-in text encodings */
extern int XMLCALL
UnrecognisedEncodingHandler(void *data, const XML_Char *encoding,
                            XML_Encoding *info);

/* As UnknownEncodingHandler but includes a "release" function pointer */
extern int XMLCALL
unknown_released_encoding_handler(void *data, const XML_Char *encoding,
                                  XML_Encoding *info);

/* External Entity Handlers
 *
 * This handler expects to be passed an ExtTest structure (see common.h) as
 * its user data.  If the "encoding" field is not NULL, it will set the
 * external entity parser's encoding to that name.  Then it parses the
 * text pointed to by the "parse_text" field and returns success or failure.
 */
extern int XMLCALL
external_entity_loader(XML_Parser parser, const XML_Char *context,
                       const XML_Char *base, const XML_Char *systemId,
                       const XML_Char *publicId);

typedef struct ext_faults {
  const char *parse_text;
  const char *fail_text;
  const XML_Char *encoding;
  enum XML_Error error;
} ExtFaults;

/* This handler expects to be passed an ExtFaults structure as its user
 * data.  If the "encoding" field is not NULL, it will set the external
 * entity parser's encoding to that name.  Then it parses the text pointed
 * to by the "parse_text" field.  The parse is expected to fail with the
 * error code in the "error" field.  If the parse succeeds, the test will be
 * failed with the message given in "fail_text".
 */
extern int XMLCALL
external_entity_faulter(XML_Parser parser, const XML_Char *context,
                        const XML_Char *base, const XML_Char *systemId,
                        const XML_Char *publicId);

/* This handler does nothing at all */
extern int XMLCALL
external_entity_null_loader(XML_Parser parser, const XML_Char *context,
                            const XML_Char *base, const XML_Char *systemId,
                            const XML_Char *publicId);

/* NotStandalone handlers */
extern int XMLCALL
reject_not_standalone_handler(void *userData);

extern int XMLCALL
accept_not_standalone_handler(void *userData);

/* Attribute List handlers */
typedef struct AttTest {
  const char *definition;
  const XML_Char *element_name;
  const XML_Char *attr_name;
  const XML_Char *attr_type;
  const XML_Char *default_value;
  int is_required;
} AttTest;

extern void XMLCALL
verify_attlist_decl_handler(void *userData, const XML_Char *element_name,
                            const XML_Char *attr_name,
                            const XML_Char *attr_type,
                            const XML_Char *default_value, int is_required);

/* Character data handlers
 *
 * This handler stops the (global) parser, leaving it resumable if
 * g_resumable is True, and removes itself as the character data handler.
 */
extern void
clearing_aborting_character_handler(void *userData, const XML_Char *s,
                                    int len);

/* This handler stops the global parser as above, then attempts further
 * variations of aborting depending on g_resumable and g_abortable.
 */
extern void
parser_stop_character_handler(void *userData, const XML_Char *s, int len);

/* Handlers that record invocation with a single character.  They expect
 * to be called with a CharData pointer as their user data.
 */
extern void XMLCALL
record_default_handler(void *userData, const XML_Char *s, int len);

extern void XMLCALL
record_cdata_handler(void *userData, const XML_Char *s, int len);

extern void XMLCALL
record_cdata_nodefault_handler(void *userData, const XML_Char *s, int len);

extern void XMLCALL
record_skip_handler(void *userData, const XML_Char *entityName,
                    int is_parameter_entity);

#endif /* XML_HANDLERS_H */

#ifdef __cplusplus
}
#endif
