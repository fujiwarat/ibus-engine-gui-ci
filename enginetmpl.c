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
#include "engine.h"

#define IBUS_ENGINE_CI_SIMPLE_GET_PRIVATE(o)  \
   ((IBusEngineCISimplePrivate *)ibus_engine_ci_simple_get_instance_private (o))

/* IBusEngineCISimplePrivate */
struct _IBusEngineCISimplePrivate {
    GString *preedit;
};


/* functions prototype */
static gboolean  ibus_engine_ci_simple_process_key_event
                                             (IBusEngine         *engine,
                                              guint               keyval,
                                              guint               keycode,
                                              guint               state);


G_DEFINE_TYPE_WITH_PRIVATE (IBusEngineCISimple,
                            ibus_engine_ci_simple,
                            IBUS_TYPE_ENGINE)


static void
ibus_engine_ci_simple_class_init (IBusEngineCISimpleClass *class)
{
    IBusEngineClass *engine_class = IBUS_ENGINE_CLASS (class);
    engine_class->process_key_event = ibus_engine_ci_simple_process_key_event;

    /* install signals */
}


static void
ibus_engine_ci_simple_init (IBusEngineCISimple *self)
{
    IBusEngineCISimplePrivate *priv;
    priv = IBUS_ENGINE_CI_SIMPLE_GET_PRIVATE (self);
    g_assert (priv);
    priv->preedit = g_string_new ("");
}


static gboolean
ibus_engine_ci_simple_process_key_event (IBusEngine *engine,
                                         guint       keyval,
                                         guint       keycode,
                                         guint       state)
{
    IBusEngineCISimple *self;
    IBusEngineCISimplePrivate *priv;
    GString *preedit;
    gunichar ch = ibus_keyval_to_unicode (keyval);
    guint len;
    IBusText *text;

    g_return_val_if_fail (IBUS_IS_ENGINE_CI_SIMPLE (engine), FALSE);
    if (state & IBUS_RELEASE_MASK)
        return FALSE;
    self = IBUS_ENGINE_CI_SIMPLE (engine);
    priv = IBUS_ENGINE_CI_SIMPLE_GET_PRIVATE (self);
    g_assert (priv);
    g_assert (priv->preedit);
    preedit = priv->preedit;
    switch (keyval) {
    case IBUS_KEY_Return:
        ibus_engine_hide_preedit_text ((IBusEngine *)self);
        ibus_engine_commit_text ((IBusEngine *)self,
                                 ibus_text_new_from_string (preedit->str));
        g_string_set_size (preedit, 0);
        return TRUE;
    case IBUS_KEY_Escape:
        ibus_engine_hide_preedit_text ((IBusEngine *)self);
        g_string_set_size (preedit, 0);
        return TRUE;
    default:
        if (!g_unichar_isprint (ch))
            return FALSE;
        else
            g_string_append_unichar (preedit, ch);
    }
    len = (guint)g_utf8_strlen (preedit->str, -1);
    text = ibus_text_new_from_string (preedit->str);
    ibus_text_append_attribute (text,
                                IBUS_ATTR_TYPE_UNDERLINE,
                                IBUS_ATTR_UNDERLINE_SINGLE,
                                0, len);
    ibus_engine_update_preedit_text ((IBusEngine *)self, text, len, TRUE);
    return TRUE;
}

