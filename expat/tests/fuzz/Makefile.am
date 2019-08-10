
bin_PROGRAMS = parse-utf-16-fuzzer parse-utf-8-fuzzer parse-iso-8859-1-fuzzer parse-us-ascii-fuzzer parse-utf-16be-fuzzer parse-utf-16le-fuzzer

parse_utf_16_fuzzer_SOURCES = parse_fuzzer.cc
parse_utf_16_fuzzer_CPPFLAGS = -std=c++11 -I../../lib -DENCODING_UTF_16
parse_utf_16_fuzzer_LDADD = ../../lib/libexpat.la

parse_utf_8_fuzzer_SOURCES = parse_fuzzer.cc
parse_utf_8_fuzzer_CPPFLAGS = -std=c++11 -I../../lib -DENCODING_UTF_8
parse_utf_8_fuzzer_LDADD = ../../lib/libexpat.la

parse_iso_8859_1_fuzzer_SOURCES = parse_fuzzer.cc
parse_iso_8859_1_fuzzer_CPPFLAGS = -std=c++11 -I../../lib -DENCODING_ISO_8859_1
parse_iso_8859_1_fuzzer_LDADD = ../../lib/libexpat.la

parse_us_ascii_fuzzer_SOURCES = parse_fuzzer.cc
parse_us_ascii_fuzzer_CPPFLAGS = -std=c++11 -I../../lib -DENCODING_US_ASCII
parse_us_ascii_fuzzer_LDADD = ../../lib/libexpat.la

parse_utf_16be_fuzzer_SOURCES = parse_fuzzer.cc
parse_utf_16be_fuzzer_CPPFLAGS = -std=c++11 -I../../lib -DENCODING_UTF_16BE
parse_utf_16be_fuzzer_LDADD = ../../lib/libexpat.la

parse_utf_16le_fuzzer_SOURCES = parse_fuzzer.cc
parse_utf_16le_fuzzer_CPPFLAGS = -std=c++11 -I../../lib -DENCODING_UTF_16LE
parse_utf_16le_fuzzer_LDADD = ../../lib/libexpat.la

if HAS_SANITIZEFUZZER
    parse_utf_16_fuzzer_CXXFLAGS = -fsanitize=fuzzer
    parse_utf_16_fuzzer_LDFLAGS = -fsanitize=fuzzer
    parse_utf_8_fuzzer_CXXFLAGS = -fsanitize=fuzzer
    parse_utf_8_fuzzer_LDFLAGS = -fsanitize=fuzzer
    parse_iso_8859_1_fuzzer_CXXFLAGS = -fsanitize=fuzzer
    parse_iso_8859_1_fuzzer_LDFLAGS = -fsanitize=fuzzer
    parse_us_ascii_fuzzer_CXXFLAGS = -fsanitize=fuzzer
    parse_us_ascii_fuzzer_LDFLAGS = -fsanitize=fuzzer
    parse_utf_16be_fuzzer_CXXFLAGS = -fsanitize=fuzzer
    parse_utf_16be_fuzzer_LDFLAGS = -fsanitize=fuzzer
    parse_utf_16le_fuzzer_CXXFLAGS = -fsanitize=fuzzer
    parse_utf_16le_fuzzer_LDFLAGS = -fsanitize=fuzzer
endif
