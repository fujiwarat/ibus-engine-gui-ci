# vim:set et ts=4:
#
# ibus-engine-gui-ci - GUI CI for IBus engine
#
# Copyright (C) 2021-2022 Takao Fujiwara <takao.fujiwara1@gmail.com>
# Copyright (C) 2021-2022 Red Hat, Inc.
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
# USA

dnl - if not 1, append datestamp to the version number.
m4_define([ibus_ci_released], [0])
m4_define([ibus_ci_major_version], [1])
m4_define([ibus_ci_minor_version], [0])
m4_define([ibus_ci_micro_version], [0])
m4_define(ibus_maybe_datestamp,
    m4_esyscmd([if test x]ibus_ci_released[ != x1; then date +.%Y%m%d | tr -d '\n\r'; fi]))

m4_define([ibus_ci_version],
    ibus_ci_major_version.ibus_ci_minor_version.ibus_ci_micro_version[]ibus_maybe_datestamp)
m4_define([ibus_required_version], [1.5.0])

AC_INIT([ibus-engine-gui-ci], [ibus_ci_version], [https://github.com/fujiwarat/ibus-engine-gui-ci/issues], [ibus-engine-gui-ci])
AM_INIT_AUTOMAKE([1.10])
AC_GNU_SOURCE

AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIR([m4])

dnl - define PACKAGE_VERSION_* variables
AM_SANITY_CHECK
AM_MAINTAINER_MODE(enable)
AC_DISABLE_STATIC
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_CXX
AC_HEADER_STDC
LT_INIT

PKG_CHECK_MODULES(IBUS, [
    ibus-1.0 >= ibus_required_version
])

PKG_CHECK_MODULES(JSON_GLIB, [
    json-glib-1.0 >= ibus_required_version
])

dnl - check engine properties
AC_ARG_WITH(engine-name,
           [AC_HELP_STRING([--with-engine-name=NAME],
                           [engine name])],
           [ENGINE_NAME="$withval"],
           [ENGINE_NAME="ibus-engine"])
AC_DEFINE_UNQUOTED(IBUS_ENGINE_CI_NAME, "$ENGINE_NAME", [engine name])

AC_ARG_WITH(compose-name,
           [AC_HELP_STRING([--with-compose-name=NAME],
                           [compose name])],
           [COMPOSE_NAME="$withval"],
           [COMPOSE_NAME="org.freedesktop.IBus.CI"])
AC_DEFINE_UNQUOTED(IBUS_COMPOSE_CI_NAME, "$COMPOSE_NAME", [compose name])

AC_ARG_WITH(engine-srcfiles,
           [AC_HELP_STRING([--with-engine-srcfiles=SRCFILE],
                           [engine source files])],
           [ENGINE_SRCFILES="$withval"],
           [ENGINE_SRCFILES='engine.c engine.h'])
if test "x${ENGINE_SRCFILES}" != "xengine.c engine.h" ; then
    echo "ENGINE_SOURCES = ${ENGINE_SRCFILES}" > Makefile-sources.tmp
    mv Makefile-sources.tmp Makefile-sources
fi
AC_SUBST(ENGINE_SRCFILES)

AC_ARG_WITH(engine-srcfilesdir,
           [AC_HELP_STRING([--with-engine-srcfilesdir=SRCFILEDIR],
                           [directory path of engine source files])],
           [ENGINE_SRCFILESDIR="$withval"],
           [ENGINE_SRCFILESDIR='$(srcdir)'])
AC_SUBST(ENGINE_SRCFILESDIR)

if test "${ENGINE_SRCFILESDIR}" = '$(srcdir)' ; then
    PKG_CHECK_MODULES(ENGINE, [
        gtk+-3.0
    ])
fi

AC_ARG_WITH(engine-cflags,
           [AC_HELP_STRING([--with-engine-cflags=CFLAGS],
                           [engine CFLAGS])],
           [ENGINE_CFLAGS="$withval"],
           [ENGINE_CFLAGS="$ENGINE_CFLAGS"])
AC_SUBST(ENGINE_CFLAGS)

AC_ARG_WITH(engine-libs,
           [AC_HELP_STRING([--with-engine-libs=LIBS],
                           [engine LIBS])],
           [ENGINE_LIBS="$withval"],
           [ENGINE_LIBS="$ENGINE_LIBS"])
AC_SUBST(ENGINE_LIBS)

AC_ARG_WITH(engine-type,
           [AC_HELP_STRING([--with-engine-type=TYPE],
                           [engine GType. default is ibus_engine_get_type()])],
           [ENGINE_TYPE="$withval"],
           [ENGINE_TYPE='ibus_engine_ci_simple_get_type()'])
AC_DEFINE_UNQUOTED(IBUS_TYPE_ENGINE_CI, $ENGINE_TYPE,
		   [an engine GType in external ENGINE_SRCFILES])

AC_ARG_WITH(engine-init,
           [AC_HELP_STRING([--with-engine-init=FUNC(bus)],
                           [external engine init FUNC(bus). default is (bus)])],
           [ENGINE_INIT="$withval"],
           [ENGINE_INIT='(bus)'])
AC_DEFINE_UNQUOTED(ibus_engine_ci_init(bus), $ENGINE_INIT,
                   [an engine init function in external ENGINE_SRCFILES])

dnl - OUTPUT files
dnl - OUTPUT files
AC_CONFIG_FILES([
Makefile
cases/Makefile
ibus-engine-gui-ci.spec
])

AC_OUTPUT
echo "
                ibus-engine-gui-ci version ibus_ci_version

ENGINE_NAME                 \"$ENGINE_NAME\"
COMPOSE_NAME                \"$COMPOSE_NAME\"
ENGINE_SRCFILES             $ENGINE_SRCFILES
ENGINE_SRCFILESDIR          $ENGINE_SRCFILESDIR
ENGINE_TYPE                 $ENGINE_TYPE
ENGINE_INIT                 $ENGINE_INIT
ENGINE_CFLAGS               $ENGINE_CFLAGS
ENGINE_LIBS                 $ENGINE_LIBS
IBUS_CFLAGS                 $IBUS_CFLAGS
IBUS_LIBS                   $IBUS_LIBS
JSON_GLIB_CFLAGS            $JSON_GLIB_CFLAGS
JSON_GLIB_LIBS              $JSON_GLIB_LIBS
"
