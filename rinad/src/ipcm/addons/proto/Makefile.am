#
# Makefile.am
#
#

%.pb.h %.pb.cc: %.proto
	protoc --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN) --grpc_out . --proto_path=. --cpp_out=. $<

GENERATED_FILES = \
	ipcm_api.pb.cc \
	ipcm_api.pb.h \
	ipcm_api.grpc.pb.cc \
	ipcm_api.grpc.pb.h 

dist_noinst_DATA = ipcm_api.proto

noinst_LTLIBRARIES = \
	libipcm_api_proto.la

nodist_libipcm_api_proto_la_SOURCES = $(GENERATED_FILES)

MOSTLYCLEANFILES = $(GENERATED_FILES)

AM_CPPFLAGS = -I$(builddir)
