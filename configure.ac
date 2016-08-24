

AC_INIT([main.c])

AC_PROG_CC
AC_LANG_C
AC_PROG_MAKE_SET
AC_PROG_INSTALL
AC_HEADER_STDC
AC_SYS_LARGEFILE


VERSION="1.0" 
AC_SUBST(VERSION) 

AC_CONFIG_SUBDIRS([libUseful-2.5])

AC_CHECK_HEADER(linux/rtc.h,AC_DEFINE(HAVE_RTC_H, 1))

AC_CHECK_LIB(c,stime,,)
cf_have_stime=$ac_cv_lib_c_stime
if test "$cf_have_stime"
then
AC_DEFINE([HAVE_STIME])
fi

dnl read Makefile.in and write Makefile 
AC_OUTPUT(Makefile)
