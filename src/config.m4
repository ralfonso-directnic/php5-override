dnl
$
Id$
dnl config.m4 for extension override

PHP_ARG_ENABLE(override, for override support,
[  --enable-override            Include counter support])

dnl Check whether the extension is enabled at all
if test "$PHP_OVERRIDE" != "no"; then
  dnl Finally, tell the build system about the extension and what files are needed
  PHP_NEW_EXTENSION(override,override.c, $ext_shared)
  PHP_SUBST(COUNTER_SHARED_LIBADD)
fi
