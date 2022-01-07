/* -*- mode: C; c-basic-offset: 4; indent-tabs-mode: nil; -*- */
/* vim:set et sts=4: */
/* ibus-engine-gui-ci
 * Copyright (C) 2022 Takao Fujiwara <takao.fujiwara1@gmail.com>
 * Copyright (C) 2022 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA
 */

#ifndef __IBUS_ENGINE_CI_SIMPLE_H_
#define __IBUS_ENGINE_CI_SIMPLE_H_

#include <ibus.h>

/*
 * Type macros.
 */

/* define GOBJECT macros */
#define IBUS_TYPE_ENGINE_CI_SIMPLE            \
    (ibus_engine_ci_simple_get_type ())
#define IBUS_ENGINE_CI_SIMPLE(obj)            \
    (G_TYPE_CHECK_INSTANCE_CAST ((obj), IBUS_TYPE_ENGINE_CI_SIMPLE, \
     IBusEngineCISimple))
#define IBUS_ENGINE_CI_SIMPLE_CLASS(klass)    \
    (G_TYPE_CHECK_CLASS_CAST ((klass), IBUS_TYPE_ENGINE_CI_SIMPLE, \
     IBusEngineCISimpleClass))
#define IBUS_IS_ENGINE_CI_SIMPLE(obj)         \
    (G_TYPE_CHECK_INSTANCE_TYPE ((obj), IBUS_TYPE_ENGINE_CI_SIMPLE))
#define IBUS_IS_ENGINE_CI_SIMPLE_CLASS(klass) \
    (G_TYPE_CHECK_CLASS_TYPE ((klass), IBUS_TYPE_ENGINE_CI_SIMPLE))
#define IBUS_ENGINE_CI_SIMPLE_GET_CLASS(obj)  \
    (G_TYPE_INSTANCE_GET_CLASS ((obj), IBUS_TYPE_ENGINE_CI_SIMPLE, \
     IBusEngineCISimpleClass))

G_BEGIN_DECLS

typedef struct _IBusEngineCISimple IBusEngineCISimple;
typedef struct _IBusEngineCISimpleClass IBusEngineCISimpleClass;
typedef struct _IBusEngineCISimplePrivate IBusEngineCISimplePrivate;

/**
 * IBusEngineCISimple:
 *
 * IBusEngineCISimple properties.
 */
struct _IBusEngineCISimple {
    /*< private >*/
    IBusEngine parent;
};

struct _IBusEngineCISimpleClass {
    /*< private >*/
    IBusEngineClass parent;

    gpointer pdummy[4];
};

GType        ibus_engine_ci_simple_get_type (void);


G_END_DECLS
#endif
