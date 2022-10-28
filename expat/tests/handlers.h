/* XML handler functions for the Expat test suite
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
   Copyright (c) 2017-2022 Rhodri James <rhodri@wildebeest.org.uk>
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

#  include "expat.h"

/* Variable holding the expected handler userData */
extern void *g_handler_data;
/* Count of the number of times the comment handler has been invoked */
extern int g_comment_count;
/* Count of the number of skipped entities */
extern int g_skip_count;
/* Count of the number of times the XML declaration handler is invoked */
extern int g_xdecl_count;

/* Start/End Element Handlers */

extern void XMLCALL start_element_event_handler(void *userData,
                                                const XML_Char *name,
                                                const XML_Char **atts);

extern void XMLCALL end_element_event_handler(void *userData,
                                              const XML_Char *name);

#  define STRUCT_START_TAG 0
#  define STRUCT_END_TAG 1

extern void XMLCALL start_element_event_handler2(void *userData,
                                                 const XML_Char *name,
                                                 const XML_Char **attr);

extern void XMLCALL end_element_event_handler2(void *userData,
                                               const XML_Char *name);

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

extern void XMLCALL counting_start_element_handler(void *userData,
                                                   const XML_Char *name,
                                                   const XML_Char **atts);

/* Text encoding handlers */

extern int XMLCALL UnknownEncodingHandler(void *data, const XML_Char *encoding,
                                          XML_Encoding *info);

extern int XMLCALL UnrecognisedEncodingHandler(void *data,
                                               const XML_Char *encoding,
                                               XML_Encoding *info);

extern int XMLCALL unknown_released_encoding_handler(void *data,
                                                     const XML_Char *encoding,
                                                     XML_Encoding *info);

/* External Entity Handlers */

typedef struct ExtOption {
  const XML_Char *system_id;
  const char *parse_text;
} ExtOption;

extern int XMLCALL external_entity_optioner(XML_Parser parser,
                                            const XML_Char *context,
                                            const XML_Char *base,
                                            const XML_Char *systemId,
                                            const XML_Char *publicId);

extern int XMLCALL external_entity_loader(XML_Parser parser,
                                          const XML_Char *context,
                                          const XML_Char *base,
                                          const XML_Char *systemId,
                                          const XML_Char *publicId);

typedef struct ext_faults {
  const char *parse_text;
  const char *fail_text;
  const XML_Char *encoding;
  enum XML_Error error;
} ExtFaults;

extern int XMLCALL external_entity_faulter(XML_Parser parser,
                                           const XML_Char *context,
                                           const XML_Char *base,
                                           const XML_Char *systemId,
                                           const XML_Char *publicId);
extern int XMLCALL external_entity_null_loader(XML_Parser parser,
                                               const XML_Char *context,
                                               const XML_Char *base,
                                               const XML_Char *systemId,
                                               const XML_Char *publicId);

extern int XMLCALL external_entity_resetter(XML_Parser parser,
                                            const XML_Char *context,
                                            const XML_Char *base,
                                            const XML_Char *systemId,
                                            const XML_Char *publicId);

extern int XMLCALL external_entity_suspender(XML_Parser parser,
                                             const XML_Char *context,
                                             const XML_Char *base,
                                             const XML_Char *systemId,
                                             const XML_Char *publicId);

extern int XMLCALL external_entity_suspend_xmldecl(XML_Parser parser,
                                                   const XML_Char *context,
                                                   const XML_Char *base,
                                                   const XML_Char *systemId,
                                                   const XML_Char *publicId);

extern int XMLCALL external_entity_suspending_faulter(XML_Parser parser,
                                                      const XML_Char *context,
                                                      const XML_Char *base,
                                                      const XML_Char *systemId,
                                                      const XML_Char *publicId);

extern int XMLCALL external_entity_cr_catcher(XML_Parser parser,
                                              const XML_Char *context,
                                              const XML_Char *base,
                                              const XML_Char *systemId,
                                              const XML_Char *publicId);

extern int XMLCALL external_entity_bad_cr_catcher(XML_Parser parser,
                                                  const XML_Char *context,
                                                  const XML_Char *base,
                                                  const XML_Char *systemId,
                                                  const XML_Char *publicId);

extern int XMLCALL external_entity_rsqb_catcher(XML_Parser parser,
                                                const XML_Char *context,
                                                const XML_Char *base,
                                                const XML_Char *systemId,
                                                const XML_Char *publicId);

extern int XMLCALL external_entity_good_cdata_ascii(XML_Parser parser,
                                                    const XML_Char *context,
                                                    const XML_Char *base,
                                                    const XML_Char *systemId,
                                                    const XML_Char *publicId);

/* Entity declaration handlers */

extern void XMLCALL entity_suspending_decl_handler(void *userData,
                                                   const XML_Char *name,
                                                   XML_Content *model);

extern void XMLCALL entity_suspending_xdecl_handler(void *userData,
                                                    const XML_Char *version,
                                                    const XML_Char *encoding,
                                                    int standalone);

extern int XMLCALL external_entity_param_checker(XML_Parser parser,
                                                 const XML_Char *context,
                                                 const XML_Char *base,
                                                 const XML_Char *systemId,
                                                 const XML_Char *publicId);

extern int XMLCALL external_entity_ref_param_checker(XML_Parser parameter,
                                                     const XML_Char *context,
                                                     const XML_Char *base,
                                                     const XML_Char *systemId,
                                                     const XML_Char *publicId);

extern int XMLCALL external_entity_param(XML_Parser parser,
                                         const XML_Char *context,
                                         const XML_Char *base,
                                         const XML_Char *systemId,
                                         const XML_Char *publicId);

extern int XMLCALL external_entity_load_ignore(XML_Parser parser,
                                               const XML_Char *context,
                                               const XML_Char *base,
                                               const XML_Char *systemId,
                                               const XML_Char *publicId);

/* NotStandalone handlers */

extern int XMLCALL reject_not_standalone_handler(void *userData);

extern int XMLCALL accept_not_standalone_handler(void *userData);

/* Attribute List handlers */

typedef struct AttTest {
  const char *definition;
  const XML_Char *element_name;
  const XML_Char *attr_name;
  const XML_Char *attr_type;
  const XML_Char *default_value;
  int is_required;
} AttTest;

extern void XMLCALL verify_attlist_decl_handler(
    void *userData, const XML_Char *element_name, const XML_Char *attr_name,
    const XML_Char *attr_type, const XML_Char *default_value, int is_required);

/* Character data handlers */

extern void XMLCALL clearing_aborting_character_handler(void *userData,
                                                        const XML_Char *s,
                                                        int len);

extern void XMLCALL parser_stop_character_handler(void *userData,
                                                  const XML_Char *s, int len);

extern void XMLCALL cr_cdata_handler(void *userData, const XML_Char *s,
                                     int len);

extern void XMLCALL rsqb_handler(void *userData, const XML_Char *s, int len);

typedef struct ByteTestData {
  int start_element_len;
  int cdata_len;
  int total_string_len;
} ByteTestData;

extern void XMLCALL byte_character_handler(void *userData, const XML_Char *s,
                                           int len);

/* Handlers that record their invocation by single characters */

extern void XMLCALL record_default_handler(void *userData, const XML_Char *s,
                                           int len);

extern void XMLCALL record_cdata_handler(void *userData, const XML_Char *s,
                                         int len);

extern void XMLCALL record_cdata_nodefault_handler(void *userData,
                                                   const XML_Char *s, int len);

extern void XMLCALL record_skip_handler(void *userData,
                                        const XML_Char *entityName,
                                        int is_parameter_entity);

/* Entity Declaration Handlers */
#  define ENTITY_MATCH_FAIL (-1)
#  define ENTITY_MATCH_NOT_FOUND (0)
#  define ENTITY_MATCH_SUCCESS (1)

extern void XMLCALL param_entity_match_handler(
    void *userData, const XML_Char *entityName, int is_parameter_entity,
    const XML_Char *value, int value_length, const XML_Char *base,
    const XML_Char *systemId, const XML_Char *publicId,
    const XML_Char *notationName);

extern void param_entity_match_init(const XML_Char *name,
                                    const XML_Char *value);

extern int get_param_entity_match_flag(void);

/* Misc handlers */

extern void XMLCALL xml_decl_handler(void *userData, const XML_Char *version,
                                     const XML_Char *encoding, int standalone);

extern void XMLCALL param_check_skip_handler(void *userData,
                                             const XML_Char *entityName,
                                             int is_parameter_entity);

extern void XMLCALL data_check_comment_handler(void *userData,
                                               const XML_Char *data);

#endif /* XML_HANDLERS_H */

#ifdef __cplusplus
}
#endif
