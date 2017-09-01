#
# Makefile.am for libstorage/testsuite
#

SUBDIRS = helpers . Utils SystemInfo probe dependencies sorting freeinfo 	\
	Devices partitions performance commented-config-file CompoundAction

AM_CPPFLAGS = -I$(top_srcdir)

LDADD = ../storage/libstorage-ng.la -lboost_unit_test_framework

check_PROGRAMS =								\
	copy.test

AM_DEFAULT_SOURCE_EXT = .cc

TESTS = $(check_PROGRAMS)

EXTRA_DIST = probe.xml

