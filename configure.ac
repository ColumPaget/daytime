

AC_INIT
AC_CONFIG_SRCDIR([main.c])

AC_PROG_CC
AC_LANG([C])
AC_PROG_MAKE_SET
AC_PROG_INSTALL

AC_SYS_LARGEFILE

AC_ARG_ENABLE(fortify, [  --enable-fortify            use 'FORTIFY_SOURCE=3' to harden source code], cf_use_fortify=$enableval, cf_use_fortify=no )
AC_ARG_ENABLE(stack-protect, [  --enable-stack-protect      use '-fstack-protector-strong' to harden source code], cf_use_stackprotect=$enableval, cf_use_stackprotect=no )
AC_ARG_ENABLE(harden, [  --enable-harden             use 'FORTIFY_SOURCE=3' and stack-protect to harden source code], cf_use_harden=$enableval, cf_use_harden=no )



if test "$cf_use_harden" = "yes"
then
cf_use_fortify="yes"
cf_use_stackprotect="yes"
fi


if test "$cf_use_fortify" = "yes"
then
CFLAGS="$CFLAGS -O2 -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=3"
fi

if test "$cf_use_stackprotect" = "yes"
then
CFLAGS="$CFLAGS -fstack-protector-strong"
fi


AC_CHECK_HEADER(linux/rtc.h,AC_DEFINE(HAVE_RTC_H, 1))

AC_CHECK_LIB(c,stime,,)
cf_have_stime=$ac_cv_lib_c_stime
if test "$cf_have_stime"
then
AC_DEFINE([HAVE_STIME])
fi



AC_ARG_ENABLE(ssl, [  --enable-ssl                use Secure Sockets Layer], cf_use_ssl=$enableval, cf_use_ssl=yes )

if test "$cf_use_ssl" = "yes"
then
AC_CHECK_HEADER([openssl/crypto.h])
AC_CHECK_HEADER([openssl/ssl.h])

AC_CHECK_LIB(crypto,EVP_CIPHER_CTX_init,,)
cf_have_libcrypto=$ac_cv_lib_crypto_EVP_CIPHER_CTX_init

dnl New versions of openssl seem to lack EVP_CIPHER_CTX_init, so let's look for another function. I've no idea what's going on with openssl, this is why I use libressl instead.
if test "$cf_have_libcrypto" != "yes"
then
	AC_CHECK_LIB(crypto,EVP_CIPHER_CTX_new,,)
	cf_have_libcrypto=$ac_cv_lib_crypto_EVP_CIPHER_CTX_new
fi

AC_CHECK_LIB(ssl,SSL_library_init,,)
cf_have_libssl=$ac_cv_lib_ssl_SSL_library_init

dnl New versions of openssl seem to lack SSL_library_init, so let's look for another function. I've no idea what's going on with openssl, this is why I use libressl instead.
if test "$cf_have_libssl" != "yes"
then
	AC_CHECK_LIB(ssl,OPENSSL_init_ssl,,)
	cf_have_libssl=$ac_cv_lib_ssl_OPENSSL_init_ssl
fi

if test "$cf_have_libssl" = "yes"
then
cf_use_zlib=yes
fi


fi


AC_ARG_ENABLE(zlib, [  --enable-zlib               use ZLib Compression], cf_use_zlib=$enableval, cf_use_zlib=yes)

if test "$cf_use_zlib" = "yes" -o "$cf_use_ssl" = "yes"
then
AC_CHECK_LIB(z,deflate,,)
cf_have_libz=$ac_cv_lib_z_deflate
fi



AC_ARG_WITH(libuseful, [  --with-libuseful              use system-wide libUseful (default=auto) ], cf_system_libuseful=$withval )

dnl These libraries are required by libuseful. We only really need them if static linking, but
dnl let's be lazy and just include them all anyway
AC_CHECK_LIB(z,deflate,,)
AC_CHECK_LIB(crypto,EVP_CIPHER_CTX_init,,)
AC_CHECK_LIB(ssl,SSL_library_init,,)
AC_CHECK_LIB(cap,cap_init,,)

if test "$cf_system_libuseful" != "no"
then
        AC_CHECK_LIB([Useful-5],GetToken,[cf_have_libUseful5=yes],[cf_have_libUseful5=no])
        if test "$cf_have_libUseful5" = "yes"
        then
        AC_CHECK_HEADERS([libUseful-5/libUseful.h])
        else
        AC_CHECK_LIB([Useful-4],GetToken,[cf_have_libUseful4=yes],[cf_have_libUseful4=no])
        AC_CHECK_HEADERS([libUseful-4/libUseful.h])
        fi
fi

if test "$cf_have_libUseful5" = "yes"
then
  echo "installed libUseful-5 found, using that instead of bundled libUseful."
  echo "if you want to use bundled libUseful, force it with --with-libuseful=no"
  AC_SUBST(LIBUSEFUL_BUNDLED,"")
  dnl Default behavior of AC_CHECK_LIB is to add the library to the LIBS variable
  dnl previous AC_CHECK_LIB don't do this, because an action is supplied for them
  AC_CHECK_LIB([Useful-5],GetToken)
elif test "$cf_have_libUseful4" = "yes"
then
  echo "installed libUseful-4 found, using that instead of bundled libUseful."
  echo "if you want to use bundled libUseful, force it with --with-libuseful=no"
  AC_SUBST(LIBUSEFUL_BUNDLED,"")
  dnl Default behavior of AC_CHECK_LIB is to add the library to the LIBS variable
  dnl previous AC_CHECK_LIB don't do this, because an action is supplied for them
  AC_CHECK_LIB([Useful-4],GetToken)
else
  AC_SUBST(LIBUSEFUL_BUNDLED,"libUseful/libUseful.a")
  AC_CONFIG_SUBDIRS([libUseful])
fi



dnl read Makefile.in and write Makefile 
AC_CONFIG_FILES([Makefile])
AC_OUTPUT
