#
# Makefile.am for libstorage/testsuite/Utils
#

AM_CPPFLAGS = -I$(top_srcdir)

LDADD = ../../storage/libstorage-ng.la -lboost_unit_test_framework

check_PROGRAMS = enum.test

AM_DEFAULT_SOURCE_EXT = .cc

TESTS = $(check_PROGRAMS)

AM_TESTS_ENVIRONMENT = BOOST_TEST_CATCH_SYSTEM_ERRORS=no

