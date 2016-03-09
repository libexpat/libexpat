# Copyright (C) Sebastian Pipping <sebastian@pipping.org>
# Licensed under MIT/X License

DOCBOOK_TO_MAN = docbook2x-man
SGML_TO_XML = sgml2xml-isoent

.PHONY: all
all: xmlwf.1

xmlwf.1: XMLWF.1
	cp $< $@

XMLWF.1: xmlwf.xml
	which $(DOCBOOK_TO_MAN)
	$(DOCBOOK_TO_MAN) $< > $@

xmlwf.xml: xmlwf.sgml
	which $(SGML_TO_XML)
	$(SGML_TO_XML) $< > $@ || true
	grep -q '^<?xml' $@

.PHONY: clean
clean:
	$(RM) xmlwf.1 XMLWF.1 xmlwf.xml
