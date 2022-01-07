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
    GSList *init;
    GSList *tests;
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
    IBusEngineCIConfig *self = (IBusEngineCIConfig *)object;
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
    g_autofree gchar **states_str;
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
    g_autofree gchar **keys;
    const GError *parse_error = NULL;
    int i;
    const char *key;
    const char *name;
    const char *description = NULL;
    const char *version = NULL;
    const char *author = NULL;
    const char *license = NULL;
    const char *homepage = NULL;
    const char *textdomain = NULL;

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
                                 "No 'component.name' JSON");
        }
        return NULL;
    }
#define COMPONENT_NEW_ARG1(Key) (#Key), (Key) ? (Key) : ""
    return ibus_component_new_varargs ("name", name,
                                       COMPONENT_NEW_ARG1 (description),
                                       COMPONENT_NEW_ARG1 (version),
                                       COMPONENT_NEW_ARG1 (author),
                                       COMPONENT_NEW_ARG1 (license),
                                       COMPONENT_NEW_ARG1 (homepage),
                                       COMPONENT_NEW_ARG1 (textdomain),
                                       NULL);
#undef COMPONENT_NEW_ARG1

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
    g_autofree gchar **keys;
    const GError *parse_error = NULL;
    int i;
    const char *key;
    const char *name = NULL;

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
                                 "No 'engine.name' JSON");
        }
        return NULL;
    }
    return ibus_engine_desc_new_varargs ("name", name, NULL);

parse_engine_desc_error:
    if (error)
        g_propagate_error (error, (GError *)parse_error);
    return NULL;
}


/**
 * ibus_test_init_new_from_json_reader:
 *
 * Returns: %NULL or list of #IBusCIKey. Nullable without errors in case
 * of no JSON members, I.E. no init keys.
 */
GSList *
ibus_test_init_new_from_json_reader (JsonReader *reader,
                                       GError    **error)
{
    const GError *parse_error = NULL;
    int ncounts, i, j;
    g_autofree gchar **keys;
    const char *key;
    const char *keyval;
    const char *keycode;
    const char *state;
    IBusCIKey *cikey;
    GSList *retval = NULL;

#define GET_TEST_INIT_VALUE(Key) \
        if (!g_strcmp0 (key, #Key)) \
            Key = json_reader_get_string_value (reader);

    /* No init keys */
    if (!json_reader_count_members (reader))
        return NULL;
    if ((parse_error = json_reader_get_error (reader)))
        goto parse_test_init_error;
    json_reader_read_member (reader, "keys");
    if ((parse_error = json_reader_get_error (reader)))
        goto parse_test_init_error;
    if (!json_reader_is_array (reader)) {
        if (error) {
            g_set_error_literal (error, IBUS_ERROR, IBUS_ERROR_FAILED,
                                 "No arrays in 'init.keys' JSON");
        }
        return NULL;
    }
    if ((parse_error = json_reader_get_error (reader)))
        goto parse_test_init_error;
    ncounts = json_reader_count_elements (reader);
    for (i = 0; i < ncounts; i++) {
        json_reader_read_element (reader, i);
        if ((parse_error = json_reader_get_error (reader)))
            goto parse_test_init_error;
        keys = json_reader_list_members (reader);
        if ((parse_error = json_reader_get_error (reader)))
            goto parse_test_init_error;
        g_assert (keys);
        keyval = NULL;
        keycode = NULL;
        state = NULL;
        for (j = 0; keys[j]; j++) {
            key = keys[j];
            json_reader_read_member (reader, key);
            if ((parse_error = json_reader_get_error (reader)))
                goto parse_test_init_error;
            GET_TEST_INIT_VALUE(keyval)
            else GET_TEST_INIT_VALUE(keycode)
            else GET_TEST_INIT_VALUE(state)
            else {
                g_warning ("Key %s should be keyval, keycode or state in init",
                           key);
            }
            json_reader_end_member (reader);
        }
        json_reader_end_element (reader);
        cikey = g_new0 (IBusCIKey, 1);
        cikey->keyval = keyval_str_to_val (keyval);
        cikey->keycode = number_str_to_val (keycode);
        cikey->state = state_str_to_val (state);
        retval = g_slist_append (retval, cikey);
    }
    json_reader_end_member (reader);

#undef GET_TEST_INIT_VALUE

    return retval;

parse_test_init_error:
    if (error)
        g_propagate_error (error, (GError *)parse_error);
    return NULL;
}


/**
 * ibus_test_tests_new_from_json_reader:
 *
 * Returns: %NULL or list of tests. If %NULL, @errors is assigned.
 */
GSList *
ibus_test_tests_new_from_json_reader (JsonReader *reader,
                                      GError    **error)
{
    GSList *retval = NULL;
    const GError *parse_error = NULL;
    int ntests, i, j, k;
    int ncase_names;
    g_autofree gchar **case_names;
    const char *case_name;
    g_autofree gchar **step_names;
    const char *step_name;
    const char *preedit;
    const char *conversion;
    const char *commit;
    const char *result;
    int nkey_types;
    g_autofree gchar **key_types;
    const char *key_type;
    const char *string;
    int nkeys;
    g_autofree gchar **keys;
    const char *key;
    const char *keyval;
    const char *keycode;
    const char *state;
    IBusCITest *citest;

#define GET_TEST_TESTS_STEP_VALUE(Key) \
        if (!g_strcmp0 (key, #Key)) \
            Key = json_reader_get_string_value (reader);

    if (!json_reader_is_array (reader)) {
        if (error) {
            g_set_error_literal (error, IBUS_ERROR, IBUS_ERROR_FAILED,
                                 "No arrays in 'tests' JSON");
        }
        return NULL;
    }
    if ((parse_error = json_reader_get_error (reader)))
        goto parse_test_tests_error;
    ntests = json_reader_count_elements (reader);
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
            nkey_types = json_reader_count_members (reader);
            if (!nkey_types) {
                /* End tests.[].test1.commit */
                json_reader_end_member (reader);
                continue;
            }
            if (nkey_types > 1) {
                g_warning ("Ignore the second step 'string' or 'key' in a " \
                           "test case in 'tests' JSON array");
            }
            if ((parse_error = json_reader_get_error (reader)))
                goto parse_test_tests_error;
            key_types = json_reader_list_members (reader);
            if ((parse_error = json_reader_get_error (reader)))
                goto parse_test_tests_error;
            g_assert (key_types);
            key_type = key_types[0];
            /* Start tests.[].test1.commit.keys */
            json_reader_read_member (reader, key_type);
            if ((parse_error = json_reader_get_error (reader)))
                goto parse_test_tests_error;
            if (!g_strcmp0 (key_type, "string")) {
                string = json_reader_get_string_value (reader);
            } else if (!g_strcmp0 (key_type, "keys")) {
                if (!json_reader_is_array (reader)) {
                    if (error) {
                        g_set_error (error,
                                     IBUS_ERROR, IBUS_ERROR_FAILED,
                                     "No arrays in 'tests.%s.%s.%s' JSON",
                                     case_name, step_name, key_type);
                    }
                    return NULL;
                }
                if ((parse_error = json_reader_get_error (reader)))
                    goto parse_test_tests_error;
                nkeys = json_reader_count_elements (reader);
                for (i = 0; i < nkeys; i++) {
                    /* Start tests.[].test1.commit.keys.[] */
                    json_reader_read_element (reader, i);
                    if ((parse_error = json_reader_get_error (reader)))
                        goto parse_test_tests_error;
                    keys = json_reader_list_members (reader);
                    if ((parse_error = json_reader_get_error (reader)))
                        goto parse_test_tests_error;
                    g_assert (keys);
                    keyval = NULL;
                    keycode = NULL;
                    state = NULL;
                    for (k = 0; keys[k]; k++) {
                        key = keys[k];
                        /* Start tests.[].test1.commit.keys.[].keyval */
                        json_reader_read_member (reader, key);
                        if ((parse_error = json_reader_get_error (reader)))
                            goto parse_test_tests_error;
                        GET_TEST_TESTS_STEP_VALUE (keyval)
                        else GET_TEST_TESTS_STEP_VALUE (keycode)
                        else GET_TEST_TESTS_STEP_VALUE (state)
                        else {
                            g_warning ("Key %s should be keyval, keycode or " \
                                       "state in init",
                                        key);
                        }
                        /* End tests.[].test1.commit.keys.[].keyval */
                        json_reader_end_member (reader);
                    }
                    /* End tests.[].test1.commit.keys.[] */
                    json_reader_end_element (reader);
                }
            } else {
                g_warning ("The key type should be 'string' or 'key' " \
                           "but your type is %s in a " \
                           "test case in 'tests' JSON array", key_type);
            }
            /* End tests.[].test1.commit.keys */
            json_reader_end_member (reader);
            if ((parse_error = json_reader_get_error (reader)))
                goto parse_test_tests_error;
            /* End tests.[].test1.commit */
            json_reader_end_member (reader);
            if ((parse_error = json_reader_get_error (reader)))
                goto parse_test_tests_error;
        }
        citest = g_new0 (IBusCITest, 1);
        citest->desc = g_strdup (case_name);
        retval = g_slist_append (retval, citest);
        /* End tests.[].test1 */
        json_reader_end_member (reader);
        if ((parse_error = json_reader_get_error (reader)))
            goto parse_test_tests_error;
        /* End tests.[] */
        json_reader_end_element (reader);
    }

#undef GET_TEST_TESTS_STEP_VALUE

    return retval;

parse_test_tests_error:
    if (error)
        g_propagate_error (error, (GError *)parse_error);
    return NULL;
}


IBusEngineCIConfig *
ibus_engine_ci_config_new_from_file (const char *filename,
                                     GError    **error)
{
    IBusEngineCIConfig *retval;
    g_autoptr(JsonParser) parser;
    const GError *parse_error = NULL;
    g_autoptr(JsonReader) reader;
    g_autofree gchar **main_members;
    int i;
    const char *main_member;
    IBusComponent *component = NULL;
    IBusEngineDesc *engine = NULL;
    GSList *init = NULL;
    GSList *tests = NULL;

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
            init = ibus_test_init_new_from_json_reader (reader, error);
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
    g_clear_object (&component);
    g_clear_object (&engine);
    //g_clear_slist (&init, g_free);
    return retval;
}

