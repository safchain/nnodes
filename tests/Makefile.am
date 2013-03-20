SUBDIRS = fuzzing

INCLUDES = \
	@CHECK_CFLAGS@ \
	-I../src/includes \
	-I../src/libmm \
	-I../src/libhl \
	-I../src/libmisc \
	-I../src/libxml \
	-I../src/libnet \
	-I../src/libdns \
	-I../src/librdb \
	-I../src/libtopt \
	-I../src/libmon \
	-I../src/libnnodes
	
if HAVE_CHECK
TESTS                       = check_nnodes
else
TESTS                       =
endif
noinst_PROGRAMS             = $(TESTS)
check_nnodes_SOURCES      	= check_nnodes.c
check_nnodes_LDADD        	= \
	@CHECK_LIBS@ \
	../src/libhl/libhl.a \
	../src/libnet/libnet.a \
	../src/libdns/libdns.a \
	../src/librdb/librdb.a \
	../src/libxml/libxml.a \
	../src/libmisc/libmisc.a \
	../src/libmm/libmm.a \
	../src/libmon/libmon.a \
	../src/libtopt/libtopt.a \
	../src/libnnodes/libnnodes.a \
	../src/libhl/libhl.a \
	../src/libnet/libnet.a \
	../src/libdns/libdns.a \
	../src/librdb/librdb.a \
	../src/libxml/libxml.a \
	../src/libmisc/libmisc.a \
	../src/libmm/libmm.a \
	../src/libmon/libmon.a \
	../src/libtopt/libtopt.a \
	../src/libnnodes/libnnodes.a

check_nnodes_DEPENDENCIES = \
	../src/libhl/libhl.a \
	../src/libnet/libnet.a \
	../src/libdns/libdns.a \
	../src/librdb/librdb.a \
	../src/libxml/libxml.a \
	../src/libmisc/libmisc.a \
	../src/libmm/libmm.a \
	../src/libmon/libmon.a \
	../src/libtopt/libtopt.a \
	../src/libnnodes/libnnodes.a

CLEANFILES                  = check_nnodes.log
