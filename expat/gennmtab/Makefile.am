noinst_PROGRAMS = gennmtab
gennmtab_SOURCES = gennmtab.c

gennmtab$(BUILD_EXEEXT): $(gennmtab_OBJECTS)
	$(CC_FOR_BUILD) $(CFLAGS_FOR_BUILD) $(LDFLAGS_FOR_BUILD) -o $@ $^

$(gennmtab_OBJECTS): CC=$(CC_FOR_BUILD)
$(gennmtab_OBJECTS): CFLAGS=$(CFLAGS_FOR_BUILD)
$(gennmtab_OBJECTS): CPPFLAGS=$(CPPFLAGS_FOR_BUILD)
