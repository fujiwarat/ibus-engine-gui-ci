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

#ifndef __IBUS_ENGINE_GUI_CI_CONFIG_H_
#define __IBUS_ENGINE_GUI_CI_CONFIG_H_

#include <glib-object.h>

#include "common.h"

G_BEGIN_DECLS

typedef struct _IBusEngineCIConfig IBusEngineCIConfig;

#define IBUS_TYPE_ENGINE_CI_CONFIG (ibus_engine_ci_config_get_type ())
#define IBUS_ENGINE_CI_CONFIG(o) (G_TYPE_CHECK_INSTANCE_CAST ((o), IBUS_TYPE_ENGINE_CI_CONFIG, IBusEngineCIConfig))
#define IBUS_IS_ENGINE_CI_CONFIG(o) (G_TYPE_CHECK_INSTANCE_TYPE ((o), IBUS_TYPE_ENGINE_CI_CONFIG))

GType               ibus_engine_ci_config_get_type   (void);
IBusEngineCIConfig *ibus_engine_ci_config_new_from_file
                                                     (const char      *filename,
                                                         GError      **error);
IBusCIKeySequence * ibus_engine_ci_config_get_init   (IBusEngineCIConfig *self);
IBusCITest *        ibus_engine_ci_config_get_tests  (IBusEngineCIConfig *self);

G_DEFINE_AUTOPTR_CLEANUP_FUNC (IBusEngineCIConfig, g_object_unref);

G_END_DECLS
#endif
