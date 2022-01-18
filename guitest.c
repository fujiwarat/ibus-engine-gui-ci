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

#include <unistd.h>
#include <gtk/gtk.h>
#include <ibus.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "common.h"
#include "ciconfig.h"

#ifdef HAVE_ENGINE_TYPE_H
#include ENGINE_TYPE_H_STR
#endif

#define GREEN "\033[0;32m"
#define RED   "\033[0;31m"
#define NC    "\033[0m"

IBusBus *m_bus;
IBusEngine *m_engine;

static GOptionContext *option_context;
static GOptionGroup *option_gtk;
static char *case_path;
static char *config_dir;
static char *exec_dir;
static gboolean calc_keycode;
static int test_index;
static int num_preedit_changes;
static int result_index;
static gboolean rerun;

static void     show_version                 (void);
static void     show_help                    (void);
static void     show_help_gtk                (void);
static void     show_help_test               (void);
static void     ciengine_focus_in_cb         (IBusEngine    *engine,
                                              gpointer       data);
static void     ciengine_focus_out_cb        (IBusEngine    *engine,
                                              gpointer       data);
static gboolean entry_focus_in_event_cb      (GtkWidget     *entry,
                                              GdkEventFocus *event,
                                              gpointer       data);
static gboolean entry_focus_out_event_cb     (GtkWidget     *entry,
                                              GdkEventFocus *event,
                                              gpointer       data);

static const GOptionEntry entries[] = {
  { "version", 'V', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, show_version, "Show the application's version.", NULL },
  { "case-file", 'c', 0, G_OPTION_ARG_STRING, &case_path, "Specify an engine test CASEFILE with JSON", "CASEFILE" },
  { "configdir", 'C', 0, G_OPTION_ARG_STRING, &config_dir, "Specify CONFIGDIR", "CONFIGDIR" },
  { "execdir", 'B', 0, G_OPTION_ARG_STRING, &exec_dir, "Specify EXECDIR", "EXECDIR" },
  { "calc-keycode", 'K', 0, G_OPTION_ARG_NONE, &calc_keycode, "Calculate keycode from keyval", NULL },
  { "help", '?', G_OPTION_FLAG_NO_ARG | G_OPTION_FLAG_HIDDEN, G_OPTION_ARG_CALLBACK, show_help, "Show the help menu.", NULL },
  { "help-gtk", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, show_help_gtk, "Show the GTK help menu.", NULL },
  { "help-test", 0, G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, show_help_test, "Show the GTest help menu.", NULL },
  { NULL }
};


static void
show_version (void)
{
    g_print ("%s - Version %s\n", g_get_application_name (), VERSION);
    exit (EXIT_SUCCESS);
}


static void
show_help (void)
{
    char *help_contents = g_option_context_get_help (option_context, TRUE, NULL);
    g_print ("%s\n", help_contents);
    g_free (help_contents);
    exit (EXIT_SUCCESS);
}


static void
show_help_gtk (void)
{
    g_autoptr(GOptionContext) context = g_option_context_new ("- ibus-engine-gui-ci");
    char *help_contents = g_option_context_get_help (context, TRUE, option_gtk);
    g_print ("%s\n", help_contents);
    g_free (help_contents);
    exit (EXIT_SUCCESS);
}


static void
show_help_test (void)
{
    int argc = 2;
    g_autofree gchar **argv = g_new0 (char *, 2);
    argv[0] = "ibus-engine-gui-ci";
    argv[1] = "--help";
    g_test_init (&argc, (char ***)&argv, NULL);
    exit (EXIT_SUCCESS);
}


guint
calculate_keycode_from_keyval (guint keyval)
{
    GdkDisplay *display = gdk_display_get_default ();
    GdkKeymap *keymap;
    GdkKeymapKey *keys = NULL;
    gint n_keys = 0;
    guint keycode = 0;

    /* Do not use g_return_val_if_fail(g_log) */
    if (!display) {
        g_printerr ("Failed to open display\n");
        return 0;
    }
    keymap = gdk_keymap_get_for_display (display);
    if (!keymap) {
        g_printerr ("Failed to get keymap\n");
        return 0;
    }
    if (gdk_keymap_get_entries_for_keyval (keymap, keyval, &keys, &n_keys))
        keycode = keys->keycode;
    else
        g_printerr ("Failed to parse keycode from keyval %x\n", keyval);
    return keycode;
}


static IBusEngine *
create_engine_cb (IBusFactory *factory,
                  const gchar *name,
                  gpointer     data)
{
    static int i = 1;
    g_autofree gchar *engine_path =
            g_strdup_printf ("/org/freedesktop/IBus/engine/ci/%d",
                             i++);
    IBusEngineCIConfig *ciconfig = (IBusEngineCIConfig *)data;

    if (g_test_verbose ())
        g_printerr ("create_engine_cb\n");
    m_engine = ibus_engine_new_with_type (IBUS_TYPE_ENGINE_CI,
                                          name,
                                          engine_path,
                                          ibus_bus_get_connection (m_bus));
    g_signal_connect (m_engine, "focus-in",
                      G_CALLBACK (ciengine_focus_in_cb), ciconfig);
    g_signal_connect (m_engine, "focus-out",
                      G_CALLBACK (ciengine_focus_out_cb), ciconfig);
    return m_engine;
}


static gboolean
register_ibus_engine (IBusEngineCIConfig *ciconfig)
{
    IBusFactory *factory;
    IBusComponent *component;
    IBusEngineDesc *engine_desc;

    m_bus = ibus_bus_new ();
    if (!ibus_bus_is_connected (m_bus)) {
        g_critical ("ibus-daemon is not running.");
        return FALSE;
    }
    factory = ibus_factory_new (ibus_bus_get_connection (m_bus));
    g_signal_connect (factory, "create-engine",
                      G_CALLBACK (create_engine_cb), ciconfig);

    component = ibus_engine_ci_config_get_component (ciconfig);
    engine_desc = ibus_engine_ci_config_get_engine_desc (ciconfig);
    ibus_component_add_engine (component, engine_desc);
    ibus_bus_register_component (m_bus, component);

    return TRUE;
}


static int
test_key_sequences_length (IBusCIKeySequence *sequence)
{
    int len = 0;
    int i;

    g_assert (sequence);
    if (sequence->length >= 0)
        return sequence->length;
    if (!g_strcmp0 (sequence->type, "string")) {
        len = strlen (sequence->value.string);
    } else if (!g_strcmp0 (sequence->type, "strings")) {
        for (i = 0; i < G_MAXINT; i++) {
            const char *string = sequence->value.strings[i];
            if (!string)
                break;
            len += strlen (string);
        }
    } else if (!g_strcmp0 (sequence->type, "keys")) {
        for (i = 0; i < G_MAXINT; i++) {
            guint keyval = sequence->value.keys[i].keyval;
            guint keycode = sequence->value.keys[i].keycode;
            guint state = sequence->value.keys[i].state;
            if (keyval == 0 && keycode == 0 && state == 0)
                break;
        }
        len = i;
    }
    return len;
}


static gboolean
test_key_sequences_case (IBusCIKeySequence *sequence,
                         const char        *desc,
                         GString          **false_str)
{
    int len, i;
    int j = 0;

    if (desc)
        g_test_message ("Start %s test\n", desc);
    if (!(len = test_key_sequences_length (sequence)))
        return FALSE;
    /* Run test cases */
    for (i = 0; i < len; i++) {
        guint keyval = 0;
        guint keycode = 0;
        guint state = 0;
        gboolean retval;
        if (!g_strcmp0 (sequence->type, "string")) {
            keyval = sequence->value.string[i];
        } else if (!g_strcmp0 (sequence->type, "strings")) {
            if (!sequence->value.strings[j])
                break;
            if (!sequence->value.strings[j][i]) {
                ++j;
                len -= i;
                i = 0;
                if (!sequence->value.strings[j])
                    break;
            }
            keyval = sequence->value.strings[j][i];
        } else if (!g_strcmp0 (sequence->type, "keys")) {
            keyval = sequence->value.keys[i].keyval;
            keycode = sequence->value.keys[i].keycode;
            state = sequence->value.keys[i].state;
        }
        if (keyval == 0 && keycode == 0 && state == 0)
            break;
        if (calc_keycode)
            keycode = calculate_keycode_from_keyval (keyval);
        g_signal_emit_by_name (m_engine, "process-key-event",
                               keyval, keycode, state, &retval);
        if (!retval && false_str) {
            gunichar ch = ibus_keyval_to_unicode (keyval);
            if (!(*false_str))
              *false_str  = g_string_new ("");
            g_string_append_unichar (*false_str, ch);
        }
        if (g_test_verbose ()) {
            g_printerr ("process_key_event U+%04X U+%04X u+%04X %s\n",
                        keyval, keycode, state, retval ? "TRUE" : "FALSE");
        }
        state |= IBUS_RELEASE_MASK;
        sleep(1);
        g_signal_emit_by_name (m_engine, "process-key-event",
                               keyval, keycode, state, &retval);
    }
    return TRUE;
}


static gboolean
enable_ime (IBusEngineCIConfig *ciconfig)
{
    IBusCIKeySequence *init;

    g_return_val_if_fail (ciconfig, FALSE);
    if (!(init = ibus_engine_ci_config_get_init (ciconfig)))
        return FALSE;
    return test_key_sequences_case (init, "init", NULL);
}


static void
set_engine_cb (GObject *object, GAsyncResult *res, gpointer data)
{
    IBusBus *bus = IBUS_BUS (object);
    g_autoptr(GError) error = NULL;

    if (g_test_verbose ())
        g_printerr ("set_engine_cb\n");
    if (!ibus_bus_set_global_engine_async_finish (bus, res, &error)) {
        g_autofree gchar *msg = g_strdup_printf ("set engine failed: %s",
                                                 error->message);
        g_test_incomplete (msg);
        return;
    }
}


static void
ciengine_focus_in_cb (IBusEngine *engine,
                      gpointer    data)
{
    IBusEngineCIConfig *ciconfig = (IBusEngineCIConfig *)data;
    IBusCITest *tests;
    g_autoptr(GString) init_false_str = NULL;
    g_autoptr(GString) preedit_false_str = NULL;
    g_autofree gchar *desc = NULL;

    if (g_test_verbose ())
        g_printerr ("engine_focus_in_cb\n");
    /* Workaround because focus-out resets the preedit text
     * ibus_bus_set_global_engine() calls bus_input_context_set_engine()
     * twice and it causes bus_engine_proxy_focus_out()
     */
    if (!rerun)
        return;

    g_return_if_fail (IBUS_IS_ENGINE_CI_CONFIG (ciconfig));
    if (enable_ime (ciconfig))
        sleep (1);

    tests = ibus_engine_ci_config_get_tests (ciconfig);
    g_assert (tests);
    desc = g_strdup_printf ("%s preedit", tests[test_index].desc);
    test_key_sequences_case (tests[test_index].preedit,
                             desc,
                             &tests[test_index].false_str);
}


static void
ciengine_focus_out_cb (IBusEngine *enigne,
                      gpointer    data)
{
    if (g_test_verbose ())
        g_printerr ("engine_focus_out_cb\n");
    rerun = TRUE;
}


static gboolean
entry_focus_in_event_cb (GtkWidget     *entry,
                         GdkEventFocus *event,
                         gpointer       data)
{
    IBusEngineCIConfig *ciconfig;
    IBusEngineDesc *engine_desc = NULL;
    const char *name;

    if (g_test_verbose ())
        g_printerr ("entry_focus_in_event_cb\n");
    g_assert (m_bus != NULL);
    g_assert (GTK_IS_ENTRY (entry));

    ciconfig = g_object_get_data (G_OBJECT (entry), "ciconfig");
    g_return_val_if_fail (ciconfig, FALSE);
    engine_desc = ibus_engine_ci_config_get_engine_desc (ciconfig);
    g_return_val_if_fail (engine_desc, FALSE);
    name = ibus_engine_desc_get_name (engine_desc);
    ibus_bus_set_global_engine_async (m_bus,
                                      name,
                                      -1,
                                      NULL,
                                      set_engine_cb,
                                      entry);
    g_object_unref (engine_desc);
    return FALSE;
}


static gboolean
entry_focus_out_event_cb (GtkWidget     *entry,
                          GdkEventFocus *event,
                          gpointer       data)
{
    if (g_test_verbose ())
        g_printerr ("entry_focus_out_event_cb\n");
    return FALSE;
}

static void
entry_preedit_changed_cb (GtkWidget  *entry,
                          const char *preedit_str,
                          gpointer   data)
{
    IBusEngineCIConfig *ciconfig;
    IBusCITest *tests;
    IBusCIKeySequence *preedit;
    int len;
    g_autofree gchar *desc = NULL;

    if (g_test_verbose ()) {
        g_printerr ("preedit_changed_cb \"%s\"\n",
                    preedit_str ? preedit_str : "(null)");
    }
    ciconfig = g_object_get_data (G_OBJECT (entry), "ciconfig");
    g_return_if_fail (IBUS_IS_ENGINE_CI_CONFIG (ciconfig));

    tests = ibus_engine_ci_config_get_tests (ciconfig);
    g_assert (tests);
    preedit = tests[test_index].preedit;
    if (!preedit)
        return;
    len = test_key_sequences_length (preedit);
    if (!len)
        return;
    ++num_preedit_changes;
    /* preedit_str can be multi-byte chars and strlen(preedit_str) cannot be
     * compared with len.
     */
    if (len != num_preedit_changes)
        return;
    desc = g_strdup_printf ("%s commit", tests[test_index].desc);
    test_key_sequences_case (tests[test_index].commit,
                             desc,
                             &tests[test_index].false_str);
    return;
}


static void
buffer_inserted_text_cb (GtkEntryBuffer *buffer,
                         guint           position,
                         const gchar    *chars,
                         guint           nchars,
                         gpointer        data)
{
/* https://gitlab.gnome.org/GNOME/gtk/commit/9981f46e0b
 * The latest GTK does not emit "inserted-text" when the text is "".
 */
#if !GTK_CHECK_VERSION (3, 22, 16)
    static int n_loop = 0;
#endif
    GtkEntry *entry = GTK_ENTRY (data);
    IBusEngineCIConfig *ciconfig;
    IBusCITest *tests;
    gboolean valid_output = TRUE;
    const char *expected = "";
    gboolean wait_next_commit_text = FALSE;
    const char *test;
    IBusCIKeySequence *preedit;
    g_autofree gchar *desc = NULL;

    if (g_test_verbose ())
        g_printerr ("buffer_inserted_text_cb %s\n", chars ? chars : "(null)");
#if !GTK_CHECK_VERSION (3, 22, 16)
    if (n_loop % 2 == 1) {
        n_loop = 0;
        return;
    }
#endif

    ciconfig = g_object_get_data (G_OBJECT (entry), "ciconfig");
    g_return_if_fail (IBUS_IS_ENGINE_CI_CONFIG (ciconfig));
    tests = ibus_engine_ci_config_get_tests (ciconfig);
    g_assert (tests);
    g_assert (tests[test_index].result);
    if (!g_strcmp0 (tests[test_index].result->type, "string")) {
        expected = tests[test_index].result->value.string;
    } else if (!g_strcmp0 (tests[test_index].result->type, "strings")) {
        expected = tests[test_index].result->value.strings[result_index++];
        if (tests[test_index].result->value.strings[result_index])
            wait_next_commit_text = TRUE;
    }
    if (0 != g_strcmp0 (chars, expected))
        valid_output = FALSE;
    if (valid_output) {
        test = GREEN "PASS" NC;
    } else {
        test = RED "FAIL" NC;
        g_test_fail ();
    }
    g_print ("%05d %s expected: \"%s\" typed: \"%s\"\n",
             test_index + 1, test, expected, chars);
    if (wait_next_commit_text) {
        if (!tests[test_index].result->value.strings[result_index + 1] &&
            tests[test_index].false_str) {
            GString *false_str = tests[test_index].false_str;
            buffer_inserted_text_cb (buffer,
                                     position,
                                     false_str->str,
                                     false_str->len,
                                     data);
            g_string_free (false_str, TRUE);
            tests[test_index].false_str = NULL;
        }
        return;
    } else {
         ++test_index;
         result_index = 0;
    }

    preedit = tests[test_index].preedit;
    if (!preedit) {
        g_signal_handlers_disconnect_by_func (
                entry,
                G_CALLBACK (entry_focus_in_event_cb),
                NULL);
        gtk_main_quit ();
        return;
    }

#if !GTK_CHECK_VERSION (3, 22, 16)
    n_loop++;
#endif
    gtk_entry_set_text (entry, "");
    num_preedit_changes = 0;
    desc = g_strdup_printf ("%s preedit", tests[test_index].desc);
    test_key_sequences_case (preedit,
                             desc,
                             &tests[test_index].false_str);
}

static void
create_window (IBusEngineCIConfig *ciconfig)
{
    GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    GtkWidget *entry = gtk_entry_new ();
    GtkEntryBuffer *buffer;

    g_object_set_data_full (G_OBJECT (entry),
                            "ciconfig",
                            g_object_ref (ciconfig),
                            g_object_unref);
    g_signal_connect (window, "destroy",
                      G_CALLBACK (gtk_main_quit), NULL);
    g_signal_connect (entry, "focus-in-event",
                      G_CALLBACK (entry_focus_in_event_cb), NULL);
    g_signal_connect (entry, "focus-out-event",
                      G_CALLBACK (entry_focus_out_event_cb), NULL);
    g_signal_connect (entry, "preedit-changed",
                      G_CALLBACK (entry_preedit_changed_cb), NULL);
    buffer = gtk_entry_get_buffer (GTK_ENTRY (entry));
    g_signal_connect (buffer, "inserted-text",
                      G_CALLBACK (buffer_inserted_text_cb), entry);
    gtk_container_add (GTK_CONTAINER (window), entry);
    gtk_widget_show_all (window);
}

static void
test_engine_ci (gconstpointer user_data)
{
    IBusEngineCIConfig *ciconfig = (IBusEngineCIConfig *)user_data;
    GLogLevelFlags flags;
    if (!register_ibus_engine (ciconfig)) {
        g_test_fail ();
        return;
    }

    ibus_engine_ci_init (m_bus);

    create_window (ciconfig);
    /* FIXME:
     * IBusIMContext opens GtkIMContextSimple as the slave and
     * GtkIMContextSimple opens the compose table on el_GR.UTF-8, and the
     * multiple outputs in el_GR's compose causes a warning in gtkcomposetable
     * and the warning always causes a fatal in GTest:
     " "GTK+ supports to output one char only: "
     */
    flags = g_log_set_always_fatal (G_LOG_LEVEL_CRITICAL);
    gtk_main ();
    g_log_set_always_fatal (flags);
}

int
main (int argc, char *argv[])
{
    g_autoptr(GError) error = NULL;
    int i;
    gboolean has_help_option = FALSE;
    IBusEngineCIConfig *ciconfig;
    char *test_path;

    /* Run test cases with X Window. */
    g_setenv ("GDK_BACKEND", "x11", TRUE);

    ibus_init ();
    /* Avoid a warning of "AT-SPI: Could not obtain desktop path or name"
     * with gtk_main().
     */
    g_setenv ("NO_AT_BRIDGE", "1", TRUE);

    option_context = g_option_context_new ("- ibus-engine-gui-ci");
    g_option_context_add_main_entries (option_context, entries, "ibus-engine-gui-ci");
    option_gtk = gtk_get_option_group (FALSE);
    g_option_context_add_group (option_context, option_gtk);
    g_option_context_set_help_enabled (option_context, FALSE);

    for (i = 1; i < argc; i++) {
        if (!g_strcmp0 (argv[i], "--help") || !g_strcmp0 (argv[i], "-h")) {
            has_help_option = TRUE;
            break;
        }
    }
    if (!has_help_option)
        g_test_init (&argc, &argv, NULL);
    if (!g_option_context_parse (option_context, &argc, &argv, &error)) {
        g_printerr ("Option parsing failed: %s\n", error->message);
        exit (EXIT_FAILURE);
    }
    if (!case_path) {
        g_printerr ("You have to specify --case-file option.\n");
        exit (EXIT_FAILURE);
    }
    if (!config_dir)
        config_dir = g_path_get_dirname (argv[0]);
    if (!exec_dir)
        exec_dir = g_path_get_dirname (argv[0]);

    ciconfig = ibus_engine_ci_config_new_from_file (case_path, &error);
    if (!ciconfig) {
        g_printerr ("IBusEngineCIConfig failed: %s\n", error->message);
        exit (EXIT_FAILURE);
    }

    /* Do not require to open Display until parse IBusEngineCIConfig */
    gtk_init (&argc, &argv);
    test_path = g_strdup ("/ibus-engine-gui-ci/test1");
    g_test_add_data_func_full (test_path,
                               g_object_ref_sink (ciconfig),
                               test_engine_ci,
                               g_object_unref);
    g_free (test_path);

    return g_test_run ();
}
