#include <check.h>
#include <stdlib.h>

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
    if (parser != NULL) {
        XML_ParserFree(parser);
    }
}


START_TEST(test_nul_byte)
{
    char *text = "<doc>\0</doc>";

    /* test that a NUL byte (in US-ASCII data) is an error */
    if (XML_Parse(parser, text, 12, 1))
        fail("Parser did not report error on NUL-byte.");
    fail_unless(XML_GetErrorCode(parser) == XML_ERROR_INVALID_TOKEN,
                "Got wrong error code for NUL-byte in US-ASCII encoding.");
}
END_TEST


START_TEST(test_u0000_char)
{
    char *text = "<doc>&#0;</doc>";

    /* test that a NUL byte (in US-ASCII data) is an error */
    if (XML_Parse(parser, text, strlen(text), 1))
        fail("Parser did not report error on NUL-byte.");
    fail_unless(XML_GetErrorCode(parser) == XML_ERROR_BAD_CHAR_REF,
                "Got wrong error code for &#0;.");
}
END_TEST


START_TEST(test_xmldecl_misplaced)
{
    char *text =
        "\n"
        "<?xml version='1.0'?>\n"
        "<a>&eee;</a>";

    if (!XML_Parse(parser, text, strlen(text), 1)) {
        fail_unless(XML_GetErrorCode(parser) == XML_ERROR_MISPLACED_XML_PI,
                    "wrong error when XML declaration is misplaced");
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
        fail("false error reported for UTF-8 BOM");
}
END_TEST

START_TEST(test_bom_utf16_be)
{
    char text[] = "\376\377\0<\0e\0/\0>";

    if (!XML_Parse(parser, text, sizeof(text) - 1, 1))
        fail("false error reported for UTF-16-BE BOM");
}
END_TEST

START_TEST(test_bom_utf16_le)
{
    char text[] = "\377\376<\0e\0/\0>\0";

    if (!XML_Parse(parser, text, sizeof(text) - 1, 1))
        fail("false error reported for UTF-16-LE BOM");
}
END_TEST

static Suite *
make_basic_suite(void)
{
    Suite *s = suite_create("basic");
    TCase *tc_chars = tcase_create("character tests");
    TCase *tc_xmldecl = tcase_create("XML declaration");

    suite_add_tcase(s, tc_chars);
    tcase_add_checked_fixture(tc_chars, basic_setup, basic_teardown);
    tcase_add_test(tc_chars, test_nul_byte);
    tcase_add_test(tc_chars, test_u0000_char);
    tcase_add_test(tc_chars, test_bom_utf8);
    tcase_add_test(tc_chars, test_bom_utf16_be);
    tcase_add_test(tc_chars, test_bom_utf16_le);

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
