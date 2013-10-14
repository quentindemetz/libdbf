## Process this file with automake to produce Makefile.in

SUBDIRS = include src po $(DOCDIR)

spec = libdbf.spec

EXTRA_DIST = $(spec) $(spec).in autogen.sh libdbf.pc.in intltool-extract.in intltool-merge.in intltool-update.in man

#all-local: $(spec)

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = libdbf.pc

rpm: $(distdir).tar.gz
	rpm -ta $(distdir).tar.gz

