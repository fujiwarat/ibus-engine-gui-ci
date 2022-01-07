# vim:set noet ts=4:
#
# ibus-engine-gui-ci - GUI CI for IBus engine
#
# Copyright (C) 2021 Takao Fujiwara <takao.fujiwara1@gmail.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

SUBDIRS = \
    cases \
    $(NULL)

TESTS = \
    ibus-engine-gui-ci \
    $(NULL)

noinst_PROGRAMS = $(TESTS)

TESTS_ENVIRONMENT = \
    top_builddir=$(top_builddir) \
    top_srcdir=$(top_srcdir) \
    builddir=$(builddir) \
    srcdir=$(srcdir) \
    LD_LIBRARY_PATH="$(top_builddir)/src/.libs:$(top_builddir)/src" \
    DISABLE_GUI_TESTS="$(DISABLE_GUI_TESTS)" \
    $(NULL)

LOG_COMPILER = $(srcdir)/runtest
test_metas_in = meta.test.in

test_execs = ibus-engine-gui-ci
test_metas = $(addsuffix .test, $(test_execs))
test_sources_DATA = $(test_metas)
test_sourcesdir = $(datadir)/installed-tests/@ENGINE_NAME@
test_execs_PROGRAMS = $(TESTS)
test_execsdir = $(libexecdir)/installed-tests/@ENGINE_NAME@

# _SOURCES does not support substitution @_SOURCES@
include Makefile-sources

ibus_engine_gui_ci_SOURCES = \
    $(ENGINE_SOURCES) \
    ciconfig.c \
    ciconfig.h \
    guitest.c \
    guitest.h \
    common.h \
    $(NULL)
ibus_engine_gui_ci_CFLAGS = \
    @IBUS_CFLAGS@ \
    @JSON_GLIB_CFLAGS@ \
    @ENGINE_CFLAGS@ \
    $(NULL)
ibus_engine_gui_ci_LDADD = \
    @IBUS_LIBS@ \
    @JSON_GLIB_LIBS@ \
    @ENGINE_LIBS@ \
    $(NULL)

EXTRA_DIST = \
    $(test_metas_in) \
    enginetmpl.c \
    enginetmpl.h \
    keystrtoval.h \
    keystrtoval.py \
    runtest \
    $(NULL)
CLEANFILES = \
    $(ENGINE_SOURCES) \
    $(test_metas) \
    engine.c \
    $(NULL)
MAINTAINERCLEANFILES = \
    keystrtoval.h \
    $(NULL)

$(test_metas): $(test_metas_in) $(test_programs)
	f=`echo $@ | sed -e 's/\.test//'`; \
	TEST_EXEC=$(test_execsdir)/$$f; \
	sed -e "s|@TEST_EXEC[@]|$$TEST_EXEC|g" $(test_metas_in) > $@.tmp; \
	mv $@.tmp $@; \
	$(NULL)

@ENGINE_SRCFILES@: guitest.h
	if test "x$(ENGINE_SRCFILESDIR)" = "x$(srcdir)" ; then \
	    cp -p enginetmpl.c engine.c; \
	    cp -p enginetmpl.h engine.h; \
	elif test -f  @ENGINE_SRCFILESDIR@/$@ ; then \
	    cp -p @ENGINE_SRCFILESDIR@/$@ .; \
	else \
	    abrt; \
	fi;
	$(NULL)

keystrtoval.h: keystrtoval.py
	python3 keystrtoval.py > $@.tmp
	mv $@.tmp $@
	$(NULL)

