#include <assert.h>
#include <check.h>
#include <stdlib.h>
#include <stdio.h>

#include "expat.h"


static XML_Parser parser;


static void
basic_setup(void)
{
    parser = XML_ParserCreate("us-ascii");
    if (parser == NULL)
        fail("Parser not created.");
}

static void
basic_teardown(void)
{
    if (parser != NULL)
        XML_ParserFree(parser);
}

/* Generate a failure using the parser state to create an error message;
 * this should be used when the parser reports and error we weren't
 * expecting.
 */
static void
xml_failure(void)
{
    char buffer[256];
    sprintf(buffer, "%s (line %d, offset %d)",
            XML_ErrorString(XML_GetErrorCode(parser)),
            XML_GetCurrentLineNumber(parser),
            XML_GetCurrentColumnNumber(parser));
    fail(buffer);
}

START_TEST(test_nul_byte)
{
    char text[] = "<doc>\0</doc>";

    /* test that a NUL byte (in US-ASCII data) is an error */
    if (XML_Parse(parser, text, sizeof(text) - 1, 1))
        fail("Parser did not report error on NUL-byte.");
    if (XML_GetErrorCode(parser) != XML_ERROR_INVALID_TOKEN)
        xml_failure();
}
END_TEST


START_TEST(test_u0000_char)
{
    char *text = "<doc>&#0;</doc>";

    /* test that a NUL byte (in US-ASCII data) is an error */
    if (XML_Parse(parser, text, strlen(text), 1))
        fail("Parser did not report error on NUL-byte.");
    if (XML_GetErrorCode(parser) != XML_ERROR_BAD_CHAR_REF)
        xml_failure();
}
END_TEST


START_TEST(test_xmldecl_misplaced)
{
    char *text =
        "\n"
        "<?xml version='1.0'?>\n"
        "<a>&eee;</a>";

    if (!XML_Parse(parser, text, strlen(text), 1)) {
        if (XML_GetErrorCode(parser) != XML_ERROR_MISPLACED_XML_PI)
            xml_failure();
    }
    else {
        fail("expected XML_ERROR_MISPLACED_XML_PI with misplaced XML decl");
    }
}
END_TEST

START_TEST(test_bom_utf8)
{
    /* This test is really just making sure we don't core on a UTF-8 BOM. */
    char *text = "\357\273\277<e/>";

    if (!XML_Parse(parser, text, strlen(text), 1))
        xml_failure();
}
END_TEST

START_TEST(test_bom_utf16_be)
{
    char text[] = "\376\377\0<\0e\0/\0>";

    if (!XML_Parse(parser, text, sizeof(text) - 1, 1))
        xml_failure();
}
END_TEST

START_TEST(test_bom_utf16_le)
{
    char text[] = "\377\376<\0e\0/\0>\0";

    if (!XML_Parse(parser, text, sizeof(text) - 1, 1))
        xml_failure();
}
END_TEST


/* Helpers used by the following test; this checks any "attr" and "refs"
 * attributes to make sure whitespace has been normalized.
 */

/* Return true if whitespace has been normalized in a string, using
 * the rules for attribute value normalization.  The 'is_cdata' flag
 * is needed since CDATA attributes don't need to have multiple
 * whitespace characters collapsed to a single space, while other
 * attribute data types do.  (Section 3.3.3 of the recommendation.)
 */
static int
is_whitespace_normalized(const XML_Char *s, int is_cdata)
{
    int blanks = 0;
    int at_start = 1;
    while (*s) {
        if (*s == ' ')
            ++blanks;
        else if (*s == '\t' || *s == '\n' || *s == '\r')
            return 0;
        else {
            if (at_start) {
                at_start = 0;
                if (blanks && !is_cdata)
                    /* illegal leading blanks */
                    return 0;
            }
            else if (blanks > 1 && !is_cdata)
                return 0;
            blanks = 0;
        }
        ++s;
    }
    if (blanks && !is_cdata)
        return 0;
    return 1;
}

/* Check the attribute whitespace checker: */
static void
testhelper_is_whitespace_normalized(void)
{
    assert(is_whitespace_normalized("abc", 0));
    assert(is_whitespace_normalized("abc", 1));
    assert(is_whitespace_normalized("abc def ghi", 0));
    assert(is_whitespace_normalized("abc def ghi", 1));
    assert(!is_whitespace_normalized(" abc def ghi", 0));
    assert(is_whitespace_normalized(" abc def ghi", 1));
    assert(!is_whitespace_normalized("abc  def ghi", 0));
    assert(is_whitespace_normalized("abc  def ghi", 1));
    assert(!is_whitespace_normalized("abc def ghi ", 0));
    assert(is_whitespace_normalized("abc def ghi ", 1));
    assert(!is_whitespace_normalized(" ", 0));
    assert(is_whitespace_normalized(" ", 1));
    assert(!is_whitespace_normalized("\t", 0));
    assert(!is_whitespace_normalized("\t", 1));
    assert(!is_whitespace_normalized("\n", 0));
    assert(!is_whitespace_normalized("\n", 1));
    assert(!is_whitespace_normalized("\r", 0));
    assert(!is_whitespace_normalized("\r", 1));
    assert(!is_whitespace_normalized("abc\t def", 1));
}

static void
check_attr_contains_normalized_whitespace(void *userdata,
                                          const XML_Char *name,
                                          const XML_Char **atts)
{
    int i;
    for (i = 0; atts[i] != NULL; i += 2) {
        const XML_Char *attrname = atts[i];
        const XML_Char *value = atts[i + 1];
        if (strcmp("attr", attrname) == 0
            || strcmp("ents", attrname) == 0
            || strcmp("refs", attrname) == 0) {
            if (!is_whitespace_normalized(value, 0)) {
                char buffer[256];
                sprintf(buffer, "attribute value not normalized: %s='%s'",
                        attrname, value);
                fail(buffer);
            }
        }
    }
}

START_TEST(test_attr_whitespace_normalization)
{
    char *text =
        "<!DOCTYPE doc [\n"
        "  <!ATTLIST doc\n"
        "            attr NMTOKENS #REQUIRED\n"
        "            ents ENTITIES #REQUIRED\n"
        "            refs IDREFS   #REQUIRED>\n"
        "]>\n"
        "<doc attr='    a  b c\t\td\te\t' refs=' id-1   \t  id-2\t\t'  \n"
        "     ents=' ent-1   \t\r\n"
        "            ent-2  ' >\n"
        "  <e id='id-1'/>\n"
        "  <e id='id-2'/>\n"
        "</doc>";

    XML_SetStartElementHandler(parser,
                               check_attr_contains_normalized_whitespace);
    if (!XML_Parse(parser, text, strlen(text), 1))
        xml_failure();
}
END_TEST


static Suite *
make_basic_suite(void)
{
    Suite *s = suite_create("basic");
    TCase *tc_chars = tcase_create("character tests");
    TCase *tc_attrs = tcase_create("attributes");
    TCase *tc_xmldecl = tcase_create("XML declaration");

    suite_add_tcase(s, tc_chars);
    tcase_add_checked_fixture(tc_chars, basic_setup, basic_teardown);
    tcase_add_test(tc_chars, test_nul_byte);
    tcase_add_test(tc_chars, test_u0000_char);
    tcase_add_test(tc_chars, test_bom_utf8);
    tcase_add_test(tc_chars, test_bom_utf16_be);
    tcase_add_test(tc_chars, test_bom_utf16_le);

    suite_add_tcase(s, tc_attrs);
    tcase_add_checked_fixture(tc_attrs, basic_setup, basic_teardown);
    tcase_add_test(tc_attrs, test_attr_whitespace_normalization);

    suite_add_tcase(s, tc_xmldecl);
    tcase_add_checked_fixture(tc_xmldecl, basic_setup, basic_teardown);
    tcase_add_test(tc_xmldecl, test_xmldecl_misplaced);

    return s;
}


int
main(int argc, char *argv[])
{
    int i, nf;
    int forking = 0, forking_set = 0;
    int verbosity = CK_NORMAL;
    Suite *s = make_basic_suite();
    SRunner *sr = srunner_create(s);

    /* run the tests for internal helper functions */
    testhelper_is_whitespace_normalized();

    for (i = 1; i < argc; ++i) {
        char *opt = argv[i];
        if (strcmp(opt, "-v") == 0 || strcmp(opt, "--verbose") == 0)
            verbosity = CK_VERBOSE;
        else if (strcmp(opt, "-q") == 0 || strcmp(opt, "--quiet") == 0)
            verbosity = CK_SILENT;
        else if (strcmp(opt, "-f") == 0 || strcmp(opt, "--fork") == 0) {
            forking = 1;
            forking_set = 1;
        }
        else if (strcmp(opt, "-n") == 0 || strcmp(opt, "--no-fork") == 0) {
            forking = 0;
            forking_set = 1;
        }
    }
    if (forking_set)
        srunner_set_fork_status(sr, forking ? CK_FORK : CK_NOFORK);
    srunner_run_all(sr, verbosity);
    nf = srunner_ntests_failed(sr);
    srunner_free(sr);
    suite_free(s);

    return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
