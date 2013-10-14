## Process this file with automake to produce Makefile.in

INCLUDES = -I@srcdir@/../include -I./include -DPACKAGE_LOCALE_DIR=\""$(datadir)/locale"\"

noinst_HEADERS = \
	dbf.h \
	endian.h 

lib_LTLIBRARIES = libdbf.la

libdbf_la_LDFLAGS = -version-info @LIBDBF_VERSION_INFO@

libdbf_la_SOURCES = \
	dbf.c \
	endian.c

libdbf_la_LIBADD =

BUILD_LIBS = -lm

