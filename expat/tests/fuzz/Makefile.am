
bin_PROGRAMS = parse_UTF_16_fuzzer parse_UTF_8_fuzzer parse_ISO_8859_1_fuzzer parse_US_ASCII_fuzzer parse_UTF_16BE_fuzzer parse_UTF_16LE_fuzzer

parse_UTF_16_fuzzer_SOURCES = parse_fuzzer.cc
parse_UTF_16_fuzzer_CPPFLAGS = -std=c++11 -I../../lib -DENCODING_UTF_16
parse_UTF_16_fuzzer_LDADD = ../../lib/libexpat.la

parse_UTF_8_fuzzer_SOURCES = parse_fuzzer.cc
parse_UTF_8_fuzzer_CPPFLAGS = -std=c++11 -I../../lib -DENCODING_UTF_8
parse_UTF_8_fuzzer_LDADD = ../../lib/libexpat.la

parse_ISO_8859_1_fuzzer_SOURCES = parse_fuzzer.cc
parse_ISO_8859_1_fuzzer_CPPFLAGS = -std=c++11 -I../../lib -DENCODING_ISO_8859_1
parse_ISO_8859_1_fuzzer_LDADD = ../../lib/libexpat.la

parse_US_ASCII_fuzzer_SOURCES = parse_fuzzer.cc
parse_US_ASCII_fuzzer_CPPFLAGS = -std=c++11 -I../../lib -DENCODING_US_ASCII
parse_US_ASCII_fuzzer_LDADD = ../../lib/libexpat.la

parse_UTF_16BE_fuzzer_SOURCES = parse_fuzzer.cc
parse_UTF_16BE_fuzzer_CPPFLAGS = -std=c++11 -I../../lib -DENCODING_UTF_16BE
parse_UTF_16BE_fuzzer_LDADD = ../../lib/libexpat.la

parse_UTF_16LE_fuzzer_SOURCES = parse_fuzzer.cc
parse_UTF_16LE_fuzzer_CPPFLAGS = -std=c++11 -I../../lib -DENCODING_UTF_16LE
parse_UTF_16LE_fuzzer_LDADD = ../../lib/libexpat.la

if HAS_SANITIZEFUZZER
    parse_UTF_16_fuzzer_CXXFLAGS = -fsanitize=fuzzer
    parse_UTF_16_fuzzer_LDFLAGS = -fsanitize=fuzzer
    parse_UTF_8_fuzzer_CXXFLAGS = -fsanitize=fuzzer
    parse_UTF_8_fuzzer_LDFLAGS = -fsanitize=fuzzer
    parse_ISO_8859_1_fuzzer_CXXFLAGS = -fsanitize=fuzzer
    parse_ISO_8859_1_fuzzer_LDFLAGS = -fsanitize=fuzzer
    parse_US_ASCII_fuzzer_CXXFLAGS = -fsanitize=fuzzer
    parse_US_ASCII_fuzzer_LDFLAGS = -fsanitize=fuzzer
    parse_UTF_16BE_fuzzer_CXXFLAGS = -fsanitize=fuzzer
    parse_UTF_16BE_fuzzer_LDFLAGS = -fsanitize=fuzzer
    parse_UTF_16LE_fuzzer_CXXFLAGS = -fsanitize=fuzzer
    parse_UTF_16LE_fuzzer_LDFLAGS = -fsanitize=fuzzer
endif
