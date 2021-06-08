protoSOURCES =
DISTCLEANFILES =

ApplicationProcessNamingInfoMessage.stamp: ApplicationProcessNamingInfoMessage.proto
	rm -f ApplicationProcessNamingInfoMessage.tmp
	touch ApplicationProcessNamingInfoMessage.tmp
	$(PROTOC) -I$(builddir) -I$(srcdir)	\
		--cpp_out=$(builddir)		\
		$(srcdir)/ApplicationProcessNamingInfoMessage.proto
	mv -f ApplicationProcessNamingInfoMessage.tmp $@

ApplicationProcessNamingInfoMessage.pb.h ApplicationProcessNamingInfoMessage.pb.cc: ApplicationProcessNamingInfoMessage.stamp
	if test -f $@; then :; else \
	  trap 'rm -rf ApplicationProcessNamingInfoMessage.lock ApplicationProcessNamingInfoMessage.stamp' 1 2 13 15; \
	  if mkdir ApplicationProcessNamingInfoMessage.lock 2>/dev/null; then \
	    rm -f ApplicationProcessNamingInfoMessage.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) ApplicationProcessNamingInfoMessage.stamp; \
	    result=$$?; rm -rf ApplicationProcessNamingInfoMessage.lock; exit $$result; \
	  else \
	    while test -d ApplicationProcessNamingInfoMessage.lock; do sleep 1; done; \
	    test -f ApplicationProcessNamingInfoMessage.stamp; \
	  fi; \
	fi

DISTCLEANFILES +=                               \
        ApplicationProcessNamingInfoMessage.pb.h ApplicationProcessNamingInfoMessage.pb.cc ApplicationProcessNamingInfoMessage.stamp

MOSTLYCLEANFILES +=                             \
        ApplicationProcessNamingInfoMessage.tmp

EXTRA_DIST +=                                   \
        ApplicationProcessNamingInfoMessage.proto

protoSOURCES += ApplicationProcessNamingInfoMessage.pb.cc ApplicationProcessNamingInfoMessage.pb.h


sm-cbac.stamp: sm-cbac.proto
	rm -f sm-cbac.tmp
	touch sm-cbac.tmp
	$(PROTOC) -I$(builddir) -I$(srcdir)	\
		--cpp_out=$(builddir)		\
		$(srcdir)/sm-cbac.proto
	mv -f sm-cbac.tmp $@

sm-cbac.pb.h sm-cbac.pb.cc: sm-cbac.stamp
	if test -f $@; then :; else \
	  trap 'rm -rf sm-cbac.lock sm-cbac.stamp' 1 2 13 15; \
	  if mkdir sm-cbac.lock 2>/dev/null; then \
	    rm -f sm-cbac.stamp; \
	    $(MAKE) $(AM_MAKEFLAGS) sm-cbac.stamp; \
	    result=$$?; rm -rf sm-cbac.lock; exit $$result; \
	  else \
	    while test -d sm-cbac.lock; do sleep 1; done; \
	    test -f sm-cbac.stamp; \
	  fi; \
	fi

DISTCLEANFILES +=                               \
        sm-cbac.pb.h sm-cbac.pb.cc sm-cbac.stamp

MOSTLYCLEANFILES +=                             \
        sm-cbac.tmp

EXTRA_DIST +=                                   \
        sm-cbac.proto

protoSOURCES += sm-cbac.pb.cc sm-cbac.pb.h


