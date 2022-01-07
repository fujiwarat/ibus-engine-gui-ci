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
