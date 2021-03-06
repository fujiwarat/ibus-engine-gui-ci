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

#include <ibus.h>
#include <json-glib/json-glib.h>

#include "ciconfig.h"
#include "common.h"
#include "keystrtoval.h"

enum {
    PROP_0,
    PROP_COMPONENT,
    PROP_ENGINE,
    PROP_INIT,
    PROP_TESTS,
    LAST_PROP
};

struct _IBusEngineCIConfig {
    GObject parent;
    IBusComponent *component;
    IBusEngineDesc *engine;
    IBusCIKeySequence *init;
    IBusCITest *tests;
};

typedef struct {
    GObjectClass parent_class;
} IBusEngineCIConfigClass;

G_DEFINE_TYPE (IBusEngineCIConfig,
               ibus_engine_ci_config,
               G_TYPE_OBJECT)

static void ibus_enigne_ci_config_finalize       (GObject           *object);
static void ibus_engine_ci_config_get_property   (GObject           *object,
                                                  uint               prop_id,
                                                  GValue             *value,
                                                  GParamSpec         *pspec);
static void ibus_engine_ci_config_set_property   (GObject            *object,
                                                  uint                prop_id,
                                                  const GValue       *value,
                                                  GParamSpec         *pspec);


static void
ibus_engine_ci_config_class_init (IBusEngineCIConfigClass *klass)
{
    GObjectClass *object_class = G_OBJECT_CLASS (klass);

    object_class->finalize = ibus_enigne_ci_config_finalize;
    object_class->get_property = ibus_engine_ci_config_get_property;
    object_class->set_property = ibus_engine_ci_config_set_property;

    g_object_class_install_property (object_class,
                                     PROP_COMPONENT,
                                     g_param_spec_pointer ("component",
                                                           "",
                                                           "",
                                                           G_PARAM_READWRITE));

    g_object_class_install_property (object_class,
                                     PROP_ENGINE,
                                     g_param_spec_pointer ("engine",
                                                           "",
                                                           "",
                                                           G_PARAM_READWRITE));

    g_object_class_install_property (object_class,
                                     PROP_INIT,
                                     g_param_spec_pointer ("init",
                                                           "",
                                                           "",
                                                           G_PARAM_READWRITE));

    g_object_class_install_property (object_class,
                                     PROP_TESTS,
                                     g_param_spec_pointer ("tests",
                                                           "",
                                                           "",
                                                           G_PARAM_READWRITE));
}


static void
ibus_engine_ci_config_init (IBusEngineCIConfig *self)
{
}

static void
ibus_enigne_ci_config_finalize (GObject *object)
{
    G_OBJECT_CLASS (ibus_engine_ci_config_parent_class)->finalize (object);
}


static void
ibus_engine_ci_config_get_property (GObject    *object,
                                    uint        prop_id,
                                    GValue     *value,
                                    GParamSpec *pspec)
{
    IBusEngineCIConfig *self = IBUS_ENGINE_CI_CONFIG (object);
    switch (prop_id) {
    case PROP_COMPONENT:
        if (self->component)
            g_value_set_pointer (value, g_object_ref (self->component));
        break;
    case PROP_ENGINE:
        if (self->engine)
            g_value_set_pointer (value, g_object_ref (self->engine));
        break;
    case PROP_INIT:
        g_value_set_pointer (value, self->init);
        break;
    case PROP_TESTS:
        g_value_set_pointer (value, self->tests);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static void
ibus_engine_ci_config_set_property (GObject       *object,
                                    uint           prop_id,
                                    const GValue  *value,
                                    GParamSpec    *pspec)
{
    IBusEngineCIConfig *self = IBUS_ENGINE_CI_CONFIG (object);
    switch (prop_id) {
    case PROP_COMPONENT: {
            IBusComponent *component = g_value_get_pointer (value);
            if (IBUS_IS_COMPONENT (component))
                self->component = g_object_ref_sink (component);
        }
        break;
    case PROP_ENGINE: {
            IBusEngineDesc *engine = g_value_get_pointer (value);
            if (IBUS_IS_ENGINE_DESC (engine))
                self->engine = g_object_ref_sink (engine);
        }
        break;
    case PROP_INIT:
        self->init = g_value_get_pointer (value);
        break;
    case PROP_TESTS:
        self->tests = g_value_get_pointer (value);
        break;
    default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}


static guint32
number_str_to_val (const char *key)
{
    guint32 retval;

    g_return_val_if_fail (key, 0);
    if (strlen (key) > 3 && key[0] == '0' && (key[1] == 'x' || key[1] == 'X'))
        return g_ascii_strtoll (key + 2, NULL, 16);
    if (strlen (key) > 3 && key[0] == 'U' && key[1] == '+')
        return g_ascii_strtoll (key + 2, NULL, 16);
    retval = g_ascii_strtoll (key, NULL, 10);
    if (retval == 0 && (*key) != 0 && (*key) != '0')
        g_warning ("Not defined your digit %s", key);
    return retval;
}


static guint32
keyval_str_to_val (const char *key)
{
    int i;

    g_return_val_if_fail (key, 0);
    if (g_str_has_prefix (key, "IBUS_KEY_")) {
        for (i = 0; IBusKeyvalStroToVal[i].key; i++) {
            if (!g_strcmp0 (key, IBusKeyvalStroToVal[i].key))
                return (guint32)IBusKeyvalStroToVal[i].val;
        }
        g_warning ("Not defined your keyval %s", key);
        return 0;
    }
    return number_str_to_val (key);
}


static guint32
state_str_to_val (const char *key)
{
    guint32 retval = 0;
    g_autofree gchar **states_str = NULL;
    int i, j;

    g_return_val_if_fail (key, 0);
    states_str = g_strsplit (key, "|", -1);
    for (i = 0; states_str[i]; i++) {
        const char *state = states_str[i];
        gboolean match_state = FALSE;
        if (g_str_has_prefix (state, "IBUS_") &&
            g_str_has_suffix (state, "_MASK")) {
            for (j = 0; IBusStateStrToVal[j].key; j++) {
                if (!g_strcmp0 (state, IBusStateStrToVal[j].key)) {
                    retval |= IBusStateStrToVal[j].val;
                    match_state = TRUE;
                    break;
                }
            }
            if (match_state)
                continue;
            g_warning ("Not defined your state %s", state);
            continue;
        }
        retval |= number_str_to_val (state);
    }
    return retval;
}


/**
 * ibus_component_new_from_json_reader:
 *
 * Returns: %NULL or #IBusComponent. If %NULL, @errors is assigned.
 */
static IBusComponent *
ibus_component_new_from_json_reader (JsonReader *reader,
                                     GError    **error)
{
    g_autofree gchar **keys = NULL;
    const GError *parse_error = NULL;
    int i;
    const char *key;
    const char *name = NULL;
    const char *description = "";
    const char *version = "";
    const char *author = "";
    const char *license = "";
    const char *homepage = "";
    const char *textdomain = "";
    const char *command_line = "";

#define GET_COMPONENT_VALUE(Key) \
        if (!g_strcmp0 (key, #Key)) \
            Key = json_reader_get_string_value (reader);

    if (!json_reader_count_members (reader)) {
        return ibus_component_new_varargs (
                "name", "org.freedesktop.IBus.Engine.CI",
                "description", "Engine CI Component",
                "version", "1.5.13",
                "author", "Takao Fujiwara &lt;takao.fujiwara1@gmail.com&gt;",
                "license", "GPL",
                "homepage", "https://github.com/ibus/ibus/wiki",
                "textdomain", "ibus-engine-gui-ci",
                "command-line", "",
                NULL);
    }
    if ((parse_error = json_reader_get_error (reader)))
        goto parse_component_error;
    keys = json_reader_list_members (reader);
    if ((parse_error = json_reader_get_error (reader)))
        goto parse_component_error;
    g_assert (keys);
    for (i = 0; keys[i]; i++) {
        key = keys[i];
        json_reader_read_member (reader, key);
        if ((parse_error = json_reader_get_error (reader)))
            goto parse_component_error;
        GET_COMPONENT_VALUE (name)
        else GET_COMPONENT_VALUE (description)
        else GET_COMPONENT_VALUE (version)
        else GET_COMPONENT_VALUE (author)
        else GET_COMPONENT_VALUE (license)
        else GET_COMPONENT_VALUE (homepage)
        else GET_COMPONENT_VALUE (textdomain)
        else GET_COMPONENT_VALUE (command_line)
        else {
            g_warning ("JsonReader does not support key %s for IBusComponent",
                       key);
        }
        if ((parse_error = json_reader_get_error (reader)))
            goto parse_component_error;
        json_reader_end_member (reader);
    }

#undef GET_COMPONENT_VALUE

    if (!name) {
        if (error) {
            g_set_error_literal (error, IBUS_ERROR, IBUS_ERROR_FAILED,
                                 "No 'component.name' JSON.");
        }
        return NULL;
    }
    return ibus_component_new_varargs ("name", name,
                                       "description", description,
                                       "version", version,
                                       "author", author,
                                       "license", license,
                                       "homepage", homepage,
                                       "textdomain", textdomain,
                                       "command-line", command_line,
                                       NULL);

parse_component_error:
    if (error)
        g_propagate_error (error, (GError *)parse_error);
    return NULL;
}


/**
 * ibus_engine_desc_new_from_json_reader:
 *
 * Returns: %NULL or #IBusEngineDesc. If %NULL, @errors is assigned.
 */
static IBusEngineDesc *
ibus_engine_desc_new_from_json_reader (JsonReader *reader,
                                       GError    **error)
{
    g_autofree gchar **keys = NULL;
    const GError *parse_error = NULL;
    int i;
    const char *key;
    const char *name = NULL;
    const char *longname = "";
    const char *description = "";
    const char *language = "";
    const char *license = "";
    const char *author = "";
    const char *icon = "";
    const char *layout = "";
    const char *layout_variant = "";

#define GET_ENGINE_DESC_VALUE(Key) \
        if (!g_strcmp0 (key, #Key)) \
            Key = json_reader_get_string_value (reader);

    if (!json_reader_count_members (reader)) {
        return ibus_engine_desc_new_varargs ("name",
                                             "ibus-engine-gui-ci",
                                             NULL);
    }
    if ((parse_error = json_reader_get_error (reader)))
        goto parse_engine_desc_error;
    keys = json_reader_list_members (reader);
    if ((parse_error = json_reader_get_error (reader)))
        goto parse_engine_desc_error;
    g_assert (keys);
    for (i = 0; keys[i]; i++) {
        key = keys[i];
        json_reader_read_member (reader, key);
        if ((parse_error = json_reader_get_error (reader)))
            goto parse_engine_desc_error;
        GET_ENGINE_DESC_VALUE (name)
        else GET_ENGINE_DESC_VALUE (longname)
        else GET_ENGINE_DESC_VALUE (description)
        else GET_ENGINE_DESC_VALUE (language)
        else GET_ENGINE_DESC_VALUE (license)
        else GET_ENGINE_DESC_VALUE (author)
        else GET_ENGINE_DESC_VALUE (icon)
        else GET_ENGINE_DESC_VALUE (layout)
        else GET_ENGINE_DESC_VALUE (layout_variant)
        else {
            g_warning ("JsonReader does not support key %s for IBusEngineDesc",
                       key);
        }
        if ((parse_error = json_reader_get_error (reader)))
            goto parse_engine_desc_error;
        json_reader_end_member (reader);
    }

#undef GET_ENGINE_DESC_VALUE

    if (!name) {
        if (error) {
            g_set_error_literal (error, IBUS_ERROR, IBUS_ERROR_FAILED,
                                 "No 'engine.name' JSON.");
        }
        return NULL;
    }
    return ibus_engine_desc_new_varargs ("name", name,
                                         "longname", longname,
                                         "description", description,
                                         "language", language,
                                         "license", license,
                                         "author", author,
                                         "icon", icon,
                                         "layout", layout,
                                         "layout_variant", layout_variant,
                                         NULL);

parse_engine_desc_error:
    if (error)
        g_propagate_error (error, (GError *)parse_error);
    return NULL;
}


static char **
get_strings_from_json_reader (JsonReader *reader,
                              const char *key_step,
                              GError    **error)
{
    const GError *parse_error = NULL;
    int nkeys_array, i;
    char **strings = NULL;
    const char *string;

    if (!json_reader_is_array (reader)) {
        if (error) {
            g_set_error (error, IBUS_ERROR, IBUS_ERROR_FAILED,
                         "No arrays in '%s.keys' JSON.",
                         key_step);
        }
        return NULL;
    }
    if ((parse_error = json_reader_get_error (reader)))
        goto get_strings_error;
    nkeys_array = json_reader_count_elements (reader);
    if (nkeys_array > 0) {
        if (!(strings = g_new0 (char *, nkeys_array + 1))) {
            if (error) {
                    g_set_error (error, IBUS_ERROR, IBUS_ERROR_FAILED,
                                 "Failed to alloc IBusCIKey in %s.%s.",
                                 key_step, "strings");
            }
            return NULL;
        }
    }
    for (i = 0; i < nkeys_array; i++) {
        /* Start init.strings.[] or tests.[].test1.commit.strings.[] */
        json_reader_read_element (reader, i);
        if ((parse_error = json_reader_get_error (reader)))
            goto get_strings_error;
        string = json_reader_get_string_value (reader);
        if ((parse_error = json_reader_get_error (reader)))
            goto get_strings_error;
        strings[i] = g_strdup (string);
        json_reader_end_element (reader);
        if ((parse_error = json_reader_get_error (reader)))
            goto get_strings_error;
    }

    return strings;

get_strings_error:
    if (error)
        g_propagate_error (error, (GError *)parse_error);
    g_free (strings);
    return NULL;
}


static IBusCIKey *
get_keys_from_json_reader (JsonReader *reader,
                           const char *key_step,
                           GError    **error)
{
    const GError *parse_error = NULL;
    int nkeys_array, i, j;
    IBusCIKey *cikey = NULL;
    g_autofree gchar **keys = NULL;
    const char *key;

#define GET_KEYS_VALUE(Key) \
        if (!g_strcmp0 (key, #Key)) \
            Key = json_reader_get_string_value (reader);

    if (!json_reader_is_array (reader)) {
        if (error) {
                g_set_error (error, IBUS_ERROR, IBUS_ERROR_FAILED,
                             "No arrays in '%s.keys' JSON.",
                             key_step);
        }
        return NULL;
    }
    if ((parse_error = json_reader_get_error (reader)))
        goto get_keys_error;
    nkeys_array = json_reader_count_elements (reader);
    if (nkeys_array > 0) {
        if (!(cikey = g_new0 (IBusCIKey, nkeys_array + 1))) {
            if (error) {
                g_set_error (error, IBUS_ERROR, IBUS_ERROR_FAILED,
                             "Failed to alloc IBusCIKey in %s.%s.",
                             key_step, "keys");
            }
            return NULL;
        }
    }
    for (i = 0; i < nkeys_array; i++) {
        const char *keyval = NULL;
        const char *keycode = NULL;
        const char *state = NULL;

        /* Start init.keys.[] or tests.[].test1.commit.keys.[] */
        json_reader_read_element (reader, i);
        if ((parse_error = json_reader_get_error (reader)))
            goto get_keys_error;
        if (!json_reader_count_members (reader)) {
            /* key_step."keys".[] is found but no keys */
            json_reader_end_element (reader);
            break;
        }
        if ((parse_error = json_reader_get_error (reader)))
            goto get_keys_error;
        keys = json_reader_list_members (reader);
        if ((parse_error = json_reader_get_error (reader)))
            goto get_keys_error;
        g_assert (keys);
        for (j = 0; keys[j]; j++) {
            key = keys[j];
            /* Start init.keys.[].keyval or
             * tests.[].test1.commit.keys.[].keyval
             */
            json_reader_read_member (reader, key);
            if ((parse_error = json_reader_get_error (reader)))
                goto get_keys_error;
            GET_KEYS_VALUE(keyval)
            else GET_KEYS_VALUE(keycode)
            else GET_KEYS_VALUE(state)
            else {
                g_warning ("Key %s should be keyval, keycode or state in "
                           "'%s' JSON.", key, key_step);
            }
            /* End init.keys.[].keyval or
             * tests.[].test1.commit.keys.[].keyval
             */
            json_reader_end_member (reader);
        }
        /* End init.keys.[] or tests.[].test1.commit.keys.[] */
        json_reader_end_element (reader);
        cikey[i].keyval = keyval_str_to_val (keyval);
        cikey[i].keycode = number_str_to_val (keycode);
        cikey[i].state = state_str_to_val (state);
    }
    return cikey;

#undef GET_KEYS_VALUE

get_keys_error:
    if (error)
        g_propagate_error (error, (GError *)parse_error);
    g_free (cikey);
    return NULL;
}


/**
 * ibus_test_init_new_from_json_reader:
 *
 * Returns: %NULL or list of #IBusCIKey. Nullable without errors in case
 * of no JSON members, I.E. no init keys.
 */
IBusCIKeySequence *
ibus_test_init_new_from_json_reader (JsonReader *reader,
                                     const char *key_step,
                                     GError    **error)
{
    IBusCIKeySequence *retval = NULL;
    const GError *parse_error = NULL;
    int nkey_types, i;
    g_autofree gchar **key_types = NULL;
    const char *key_type;

    nkey_types = json_reader_count_members (reader);
    /* No init keys */
    if (!nkey_types)
        return NULL;
    if ((parse_error = json_reader_get_error (reader)))
        goto parse_test_init_error;
    key_types = json_reader_list_members (reader);
    if ((parse_error = json_reader_get_error (reader)))
        goto parse_test_init_error;
    g_assert (key_types);
    if (!(retval = g_new0 (IBusCIKeySequence, 1))) {
        if (error) {
            g_set_error (error, IBUS_ERROR, IBUS_ERROR_FAILED,
                         "Failed to alloc IBusCIKeySequence in %s.",
                         key_step);
        }
        return NULL;
    }
    retval->length = -1;
    for (i = 0; key_types[i]; ++i) {
        key_type = key_types[i];
        /* Start init.keys or tests.[].test1.commit.keys */
        json_reader_read_member (reader, key_type);
        if ((parse_error = json_reader_get_error (reader)))
            goto parse_test_init_error;
        if (!g_strcmp0 (key_type, "string")) {
            const char *string = json_reader_get_string_value (reader);
            retval->type = g_strdup ("string");
            retval->value.string = g_strdup (string);
        } else if (!g_strcmp0 (key_type, "strings")) {
            char **strings = get_strings_from_json_reader (reader,
                                                           key_step,
                                                           error);
            if (!strings) {
                json_reader_end_member (reader);
                g_free (retval);
                return NULL;
            }
            retval->type = g_strdup ("strings");
            retval->value.strings = strings;
        } else if (!g_strcmp0 (key_type, "keys")) {
            IBusCIKey *cikey = get_keys_from_json_reader (reader,
                                                          key_step,
                                                          error);
            if (!cikey) {
                json_reader_end_member (reader);
                g_free (retval);
                return NULL;
            }
            retval->type = g_strdup ("keys");
            retval->value.keys = cikey;
        } else if (!g_strcmp0 (key_type, "length")) {
            retval->length = json_reader_get_int_value (reader);
        } else {
            if (error) {
                g_set_error (error, IBUS_ERROR, IBUS_ERROR_FAILED,
                             "The key type should be 'string', 'strings', " \
                             "'key' or 'length' but your type is '%s' in " \
                             "'%s' JSON.",
                             key_type, key_step);
            }
            json_reader_end_element (reader);
            g_free (retval->type);
            g_free (retval);
            return NULL;
        }
        /* End init.keys or tests.[].test1.commit.keys */
        json_reader_end_member (reader);
    }

    return retval;

parse_test_init_error:
    if (error)
        g_propagate_error (error, (GError *)parse_error);
    g_free (retval->type);
    g_free (retval->value.string);
    g_free (retval);
    return NULL;
}


/**
 * ibus_test_tests_new_from_json_reader:
 *
 * Returns: %NULL or list of tests. If %NULL, @errors is assigned.
 */
IBusCITest *
ibus_test_tests_new_from_json_reader (JsonReader *reader,
                                      GError    **error)
{
    IBusCITest *citest = NULL;
    const GError *parse_error = NULL;
    int i, j;
    int ntests = 0;
    int ncase_names;
    g_autofree gchar **case_names = NULL;
    const char *case_name;
    g_autofree gchar **step_names = NULL;
    const char *step_name;
    g_autofree gchar *key_step = NULL;
    IBusCIKeySequence *cisequence;

#define GET_TEST_TESTS_STEP_VALUE(Step) \
        if (!g_strcmp0 (step_name, #Step)) \
                citest[i].Step = cisequence;

    if (!json_reader_is_array (reader)) {
        if (error) {
            g_set_error_literal (error, IBUS_ERROR, IBUS_ERROR_FAILED,
                                 "No arrays in 'tests' JSON.");
        }
        return NULL;
    }
    if ((parse_error = json_reader_get_error (reader)))
        goto parse_test_tests_error;
    ntests = json_reader_count_elements (reader);
    if (!ntests) {
        if (error) {
            g_set_error_literal (error, IBUS_ERROR, IBUS_ERROR_FAILED,
                                 "No test cases in 'tests' JSON.");
        }
        return NULL;
    }
    if (!(citest = g_new0 (IBusCITest, ntests + 1))) {
        if (error) {
            g_set_error_literal (error, IBUS_ERROR, IBUS_ERROR_FAILED,
                                 "Failed to alloc IBusCITest in 'tests' JSON.");
        }
        return NULL;
    }
    for (i = 0; i < ntests; i++) {
        /* Start tests.[] */
        json_reader_read_element (reader, i);
        if ((parse_error = json_reader_get_error (reader)))
            goto parse_test_tests_error;
        ncase_names = json_reader_count_members (reader);
        if (!ncase_names) {
            /* Start tests.[] */
            json_reader_end_element (reader);
            continue;
        } else if (ncase_names > 1) {
            g_warning ("Ignore the second test by 'tests' JSON array element");
        }
        if ((parse_error = json_reader_get_error (reader)))
            goto parse_test_tests_error;
        case_names = json_reader_list_members (reader);
        if ((parse_error = json_reader_get_error (reader)))
            goto parse_test_tests_error;
        g_assert (case_names);
        case_name = case_names[0];
        /* Start tests.[].test1 */
        json_reader_read_member (reader, case_name);
        if ((parse_error = json_reader_get_error (reader)))
            goto parse_test_tests_error;
        step_names = json_reader_list_members (reader);
        if ((parse_error = json_reader_get_error (reader)))
            goto parse_test_tests_error;
        g_assert (step_names);
        for (j = 0; step_names[j]; j++) {
            step_name = step_names[j];
            /* Start tests.[].test1.commit */
            json_reader_read_member (reader, step_name);
            if ((parse_error = json_reader_get_error (reader)))
                goto parse_test_tests_error;
            key_step = g_strdup_printf ("%s.%s.%s",
                                        "tests.[]", case_name, step_name);
            cisequence = ibus_test_init_new_from_json_reader (reader,
                                                              key_step,
                                                              error);
            if (error && *error) {
                g_clear_object (&cisequence);
                json_reader_end_member (reader);
                json_reader_end_member (reader);
                json_reader_end_element (reader);
                goto parse_test_tests_alloc_error;
            }
            GET_TEST_TESTS_STEP_VALUE (preedit)
            else GET_TEST_TESTS_STEP_VALUE (conversion)
            else GET_TEST_TESTS_STEP_VALUE (commit)
            else GET_TEST_TESTS_STEP_VALUE (result)
            else {
                if (error) {
                    g_set_error (error, IBUS_ERROR, IBUS_ERROR_FAILED,
                                 "Your step name '%s' should be 'preedit', " \
                                 "'conversion', 'commit' or 'result' in '%s' " \
                                 "JSON.",
                                 step_name, key_step);
                }
                json_reader_end_member (reader);
                json_reader_end_member (reader);
                json_reader_end_element (reader);
                goto parse_test_tests_alloc_error;
            }
            /* End tests.[].test1.commit */
            json_reader_end_member (reader);
            if ((parse_error = json_reader_get_error (reader)))
                goto parse_test_tests_error;
        }
        if (!citest[i].preedit) {
            if (error) {
                g_set_error (error, IBUS_ERROR, IBUS_ERROR_FAILED,
                             "Your casue '%s' does not have 'preedit' step " \
                             "in 'tests.[]' JSON.",
                             case_name);
            }
            json_reader_end_member (reader);
            json_reader_end_element (reader);
            goto parse_test_tests_alloc_error;
        }
        if (!citest[i].result) {
            if (error) {
                g_set_error (error, IBUS_ERROR, IBUS_ERROR_FAILED,
                             "Your casue '%s' does not have 'result' step " \
                             "in 'tests.[]' JSON.",
                             case_name);
            }
            json_reader_end_member (reader);
            json_reader_end_element (reader);
            goto parse_test_tests_alloc_error;
        }
        citest[i].desc = g_strdup (case_name);
        /* End tests.[].test1 */
        json_reader_end_member (reader);
        if ((parse_error = json_reader_get_error (reader)))
            goto parse_test_tests_error;
        /* End tests.[] */
        json_reader_end_element (reader);
    }

#undef GET_TEST_TESTS_STEP_VALUE

    return citest;

parse_test_tests_error:
    if (error)
        g_propagate_error (error, (GError *)parse_error);
parse_test_tests_alloc_error:
    for (i = 0; citest && (i < ntests); i++) {
        g_clear_object (&citest[i].preedit);
        g_clear_object (&citest[i].conversion);
        g_clear_object (&citest[i].commit);
        g_clear_object (&citest[i].result);
        g_free (citest[i].desc);
    }
    g_free (citest);
    return NULL;
}


IBusEngineCIConfig *
ibus_engine_ci_config_new_from_file (const char *filename,
                                     GError    **error)
{
    IBusEngineCIConfig *retval;
    g_autoptr(JsonParser) parser = NULL;
    const GError *parse_error = NULL;
    g_autoptr(JsonReader) reader = NULL;
    g_autofree gchar **main_members = NULL;
    int i;
    const char *main_member;
    IBusComponent *component = NULL;
    IBusEngineDesc *engine = NULL;
    IBusCIKeySequence *init = NULL;
    IBusCITest *tests = NULL;

    /* FIXME: Use json_gobject_from_data() and convert JsonObject to GObject
     * with GObject->set_property() class method.
     */
    parser = json_parser_new ();
    if (!json_parser_load_from_file (parser, filename, error))
        return NULL;
    reader = json_reader_new (json_parser_get_root (parser));
    main_members = json_reader_list_members (reader);
    if ((parse_error = json_reader_get_error (reader)))
        goto main_member_error;
    g_assert (main_members);
    for (i = 0; main_members[i]; i++) {
        main_member = main_members[i];
        json_reader_read_member (reader, main_member);
        if ((parse_error = json_reader_get_error (reader)))
            goto main_member_error;
        if (!g_strcmp0 (main_member, "component")) {
            component = ibus_component_new_from_json_reader (reader, error);
            if (!component)
                goto main_subcomponent_error;
        } else if (!g_strcmp0 (main_member, "engine")) {
            engine = ibus_engine_desc_new_from_json_reader (reader, error);
            if (!engine)
                goto main_subcomponent_error;
        } else if (!g_strcmp0 (main_member, "init")) {
            init = ibus_test_init_new_from_json_reader (reader, "init", error);
            if (error && *error)
                goto main_subcomponent_error;
        } else if (!g_strcmp0 (main_member, "tests")) {
            tests = ibus_test_tests_new_from_json_reader (reader, error);
            if (!tests)
                goto main_subcomponent_error;
        }
        json_reader_end_member (reader);
    }

    retval = g_object_new (IBUS_TYPE_ENGINE_CI_CONFIG,
                           "component", component,
                           "engine", engine,
                           "init", init,
                           "tests", tests,
                           NULL);
main_member_error:
    if (error && parse_error)
        g_propagate_error (error, (GError *)parse_error);
main_subcomponent_error:
    return retval;
}


IBusComponent *
ibus_engine_ci_config_get_component (IBusEngineCIConfig *self)
{
    g_return_val_if_fail (self, NULL);
    return g_object_ref (self->component);
}


IBusEngineDesc *
ibus_engine_ci_config_get_engine_desc (IBusEngineCIConfig *self)
{
    g_return_val_if_fail (self, NULL);
    return g_object_ref (self->engine);
}


IBusCIKeySequence *
ibus_engine_ci_config_get_init (IBusEngineCIConfig *self)
{
    g_return_val_if_fail (self, NULL);
    return self->init;
}


IBusCITest *
ibus_engine_ci_config_get_tests (IBusEngineCIConfig *self)
{
    g_return_val_if_fail (self, NULL);
    return self->tests;
}
