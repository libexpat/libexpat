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


static Suite *
make_basic_suite(void)
{
    Suite *s = suite_create("basic");
    TCase *tc_nulls = tcase_create("null characters");
    TCase *tc_xmldecl = tcase_create("XML declaration");

    suite_add_tcase(s, tc_nulls);
    tcase_add_checked_fixture(tc_nulls, basic_setup, basic_teardown);
    tcase_add_test(tc_nulls, test_nul_byte);
    tcase_add_test(tc_nulls, test_u0000_char);

    suite_add_tcase(s, tc_xmldecl);
    tcase_add_checked_fixture(tc_xmldecl, basic_setup, basic_teardown);
    tcase_add_test(tc_xmldecl, test_xmldecl_misplaced);

    return s;
}


int
main(int argc, char *argv[])
{
    int nf;
    int verbosity = CK_NORMAL;
    Suite *s = make_basic_suite();
    SRunner *sr = srunner_create(s);

    if (argc >= 2) {
        char *opt = argv[1];
        if (strcmp(opt, "-v") == 0 || strcmp(opt, "--verbose") == 0)
            verbosity = CK_VERBOSE;
        else if (strcmp(opt, "-q") == 0 || strcmp(opt, "--quiet") == 0)
            verbosity = CK_SILENT;
    }
    srunner_run_all(sr, verbosity);
    nf = srunner_ntests_failed(sr);
    srunner_free(sr);
    suite_free(s);

    return (nf == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
