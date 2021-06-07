#
# Makefile.am
#
# Written by: Francesco Salvestrini <f DOT salvestrini AT nextworks DOT it>
#

EXTRA_DIST   =
CLEANFILES   =

SUBDIRS= ma proto

#
#Addons
#
noinst_LTLIBRARIES = libaddons.la
libaddons_la_SOURCES  =				\
	console.cc				\
	scripting.cc			\
	mobility-manager.cc	    \
	ipcm-api.cc

libaddons_la_CPPFLAGS =				\
	$(CPPFLAGS_EXTRA)			\
	$(LIBRINA_CFLAGS)			\
	-I$(srcdir)/../../			\
	-I$(srcdir)/../../common

libaddons_la_CFLAGS = $(LIBPROTOBUF_CFLAGS) $(LIBGRPCPP_CFLAGS)

libaddons_la_LIBADD =				\
	ma/libaddons_ma.la \
	$(builddir)/proto/libipcm_api_proto.la \
	$(LIBGRPCPP_LIBS) \
	-lgrpc++_reflection
