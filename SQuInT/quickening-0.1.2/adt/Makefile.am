noinst_PROGRAMS = test_eventqueue test_set check_si check_typed_hash \
	check_string_bind bench-recursively check_radix check_socket_printf \
	check_base64 check_misc check_sashimi check_fmt
TESTS = check_si check_typed_hash check_string_bind check_radix check_socket_printf check_base64 check_misc check_sashimi check_fmt
EXTRA_DIST = README neils_adt.m4 tcpdump.m4 

test_eventqueue_SOURCES = test_eventqueue.c 
test_eventqueue_LDADD = libadt.la 

test_set_SOURCES = test_set.c 
test_set_LDADD = libadt.la

check_misc_SOURCES = check_misc.c
check_misc_LDADD = libadt.la
check_base64_SOURCES = check_base64.c 
check_base64_LDADD = libadt.la
check_si_SOURCES = check_si.c
check_si_LDADD = libadt.la
check_typed_hash_SOURCES = check_typed_hash.c
check_typed_hash_LDADD = libadt.la
check_string_bind_SOURCES = check_string_bind.c
check_string_bind_LDADD = libadt.la
check_radix_SOURCES = check_radix.c
check_radix_LDADD = libadt.la
check_socket_printf_SOURCES = check_socket_printf.c
check_socket_printf_LDADD = libadt.la
check_sashimi_SOURCES = check_sashimi.c 
check_sashimi_LDADD = libadt.la
check_fmt_SOURCES = check_fmt.c
check_fmt_LDADD = libadt.la

bench_recursively_SOURCES = bench-recursively.c
bench_recursively_LDADD = libadt.la

noinst_LTLIBRARIES = libadt.la libgadt.la

libadt_la_SOURCES = hashtable.c hashtable.h \
	typed_hashtable.h \
	base64.c base64.h \
	queue.c queue.h \
	typed_queue.h \
	timeseries.c timeseries.h \
	eventqueue.c eventqueue.h \
	sprinter.c sprinter.h \
	hashes.c hashes.h \
	alert.c alert.h \
	hprop.c hprop.h \
	buffer.c buffer.h \
	table.c table.h \
	hpropinterface.c hpropinterface.h \
	sorted_intlist.c sorted_intlist.h \
	sorted_stringlist.c sorted_stringlist.h \
	red_transitive.c red_transitive.h \
	link_matrix.c link_matrix.h \
	disjoint_set.c disjoint_set.h \
	string_bindings.c string_bindings.h \
	progress.c progress.h \
	postgresql.c postgresql.h \
	radix.c radix.h \
	distribution.c distribution.h \
	set.c set.h \
	join.c join.h \
	commando.c commando.h \
	recursively.c recursively.h \
	bloom.c bloom.h \
	sockaddr.c sockaddr.h \
	snprintf.c \
	socket_printf.c socket_printf.h \
	filetest.c filetest.h \
	globber.c globber.h \
	configaro.c configaro.h \
	lamefmt.c lamefmt.h \
	sashimi.c sashimi.h sashimitimer.c sashimitimer.h \
	log.c log.h \
	daemon.c daemon.h \
	nscommon.h 

# the optional getopt stuff gets used on FreeBSD, which has
# its own, non-gnu, getopt code.
libadt_la_LIBADD = @GETOPT_LIBTOOLOBJ@ @SCANDIR_LIBTOOLOBJ@
libadt_la_DEPENDENCIES = @GETOPT_LIBTOOLOBJ@ @SCANDIR_LIBTOOLOBJ@
EXTRA_libadt_la_SOURCES = getopt.c getopt.h getopt1.c scandir.c scandir.h

libgadt_la_SOURCES = graphing_ts.c graphing_ts.h

%.lint: %.c Makefile .lclintrc %.o
	$(LINT) +unixlib -exportlocal $(AM_CFLAGS) $(DEFS) $(DEFAULT_INCLUDES) $< | tee $@

%.E: %.c
	$(CC) -E $(INCLUDES) $(DEFS) $< > $@

INCLUDES = -I$(srcdir)/.. -I$(srcdir)
%.eps: %.dot
	dot -Tps $< > $@ || rm $@

