#
# Makefile.am for libstorage/testsuite/SystemInfo
#

AM_CPPFLAGS = -I$(top_srcdir)

LDADD = ../../storage/libstorage-ng.la -lboost_unit_test_framework

check_PROGRAMS =								\
	blkid.test

AM_DEFAULT_SOURCE_EXT = .cc

TESTS = $(check_PROGRAMS)

