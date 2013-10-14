libdbf.3: libdbf.sgml
	docbook2man $<
	mv LIBDBF.3 libdbf.3

%.3: %.sgml
	docbook-to-man $< > $@

%.html: %.sgml
	docbook2html $<
	mv index.html $@

distclean-local: clean-local

clean-local:
	rm -f ${man_MANS}

SGML_FILES = 

EXTRA_DIST = ${SGML_FILES} libdbf.sgml

man_MANS = $(SGML_FILES:.sgml=.3) libdbf.3

html-local: $(SGML_FILES:.sgml=.html)

.PHONY: html
