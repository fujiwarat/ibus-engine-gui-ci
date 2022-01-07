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

extern GType IBUS_TYPE_ENGINE_CI;

#define GREEN "\033[0;32m"
#define RED   "\033[0;31m"
#define NC    "\033[0m"

IBusBus *m_bus;
IBusEngine *m_engine;

IBusKeymap *keymap;

static GOptionContext *option_context;
static GOptionGroup *option_gtk;
static char *case_path;
static char *config_dir;
static char *exec_dir;
static int test_index;

static void show_version (void);
static void show_help (void);
static void show_help_gtk (void);
static void show_help_test (void);
static gboolean entry_focus_in_event_cb (GtkWidget     *entry,
                                         GdkEventFocus *event,
                                         gpointer       data);

static const GOptionEntry entries[] = {
  { "version", 'V', G_OPTION_FLAG_NO_ARG, G_OPTION_ARG_CALLBACK, show_version, "Show the application's version.", NULL },
  { "case-file", 'c', 0, G_OPTION_ARG_STRING, &case_path, "Specify an engine test CASEFILE with JSON", "CASEFILE" },
  { "configdir", 'C', 0, G_OPTION_ARG_STRING, &config_dir, "Specify CONFIGDIR", "CONFIGDIR" },
  { "execdir", 'B', 0, G_OPTION_ARG_STRING, &exec_dir, "Specify EXECDIR", "EXECDIR" },
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


static guint16 guess_keycode (IBusKeymap         *keymap,
			      guint               keyval,
			      guint32             modifiers)
{
    /* The IBusKeymap only have 256 entries here,
       Use Brute Force method to get keycode from keyval. */
    guint16 keycode = 0;
    for (; keycode < 256; ++keycode) {
	if (keyval == ibus_keymap_lookup_keysym (keymap, keycode, modifiers))
	    return keycode;
    }
    return 0;
}

static IBusEngine *
create_engine_cb (IBusFactory *factory,
                  const gchar *name,
                  gpointer     data)
{
    static int i = 1;
    gchar *engine_path =
            g_strdup_printf ("/org/freedesktop/IBus/engine/ci/%d",
                             i++);

    m_engine = ibus_engine_new_with_type (IBUS_TYPE_ENGINE_CI,
                                          name,
                                          engine_path,
                                          ibus_bus_get_connection (m_bus));
    g_free (engine_path);

    return m_engine;
}

static gboolean
register_ibus_engine ()
{
    IBusFactory *factory;
    IBusComponent *component;
    IBusEngineDesc *desc;

    m_bus = ibus_bus_new ();
    if (!ibus_bus_is_connected (m_bus)) {
        g_critical ("ibus-daemon is not running.");
        return FALSE;
    }
    factory = ibus_factory_new (ibus_bus_get_connection (m_bus));
    g_signal_connect (factory, "create-engine",
                      G_CALLBACK (create_engine_cb), NULL);

    component = ibus_component_new (
            IBUS_COMPOSE_CI_NAME,
            "Hangul Engine Test",
            "1.5.1",
            "GPL",
            "Peng Huang <shawn.p.huang@gmail.com>",
            "https://github.com/ibus/ibus/wiki",
            "",
            "ibus-hangul");
    desc = ibus_engine_desc_new (
            "hangultest",
            "Hangul Test",
            "Hangul Test",
            "en",
            "GPL",
            "Peng Huang <shawn.p.huang@gmail.com>",
            "ibus-hangul",
            "us");
    ibus_component_add_engine (component, desc);
    ibus_bus_register_component (m_bus, component);

    return TRUE;
}

static gboolean
finit (gpointer data)
{
    g_test_incomplete ("time out");
    gtk_main_quit ();
    return FALSE;
}

static gboolean
enable_ime (IBusEngineCIConfig *ciconfig)
{
    int i;
    IBusCIKeySequence *init;
    IBusCIKey *keys;

    g_return_val_if_fail (ciconfig, FALSE);
    init = ibus_engine_ci_config_get_init (ciconfig);
    keys = init->value.keys;
    if (!keys)
        return FALSE;
    if (keys->keyval == 0 && keys->keycode == 0)
        return FALSE;
    while (keys && keys->keyval != 0 && keys->keycode != 0) {
        int keyval = keys->keyval;
        int keycode = keys->keycode;
        int state = keys->state;
        gboolean retval = FALSE;

        g_signal_emit_by_name (m_engine, "process-key-event",
                               keyval, keycode, state, &retval);
        state |= IBUS_RELEASE_MASK;
        sleep(1);
        g_signal_emit_by_name (m_engine, "process-key-event",
                               keyval, keycode, state, &retval);
        keys++;
     }
     return TRUE;
}


static int
test_key_sequences_length (IBusCIKeySequence *sequence)
{
    int len = 0;
    int i;
    if (!g_strcmp0 (sequence->type, "string")) {
        len = strlen (sequence->value.string);
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
                         const char        *desc)
{
    int len, i;

    if (desc)
        g_test_message ("Start %s test\n", desc);
    len = test_key_sequences_length (sequence);
    /* Run test cases */
    for (i = 0; i < len; i++) {
        guint keyval = 0;
        guint keycode = 0;
        guint state = 0;
        gboolean retval;
        if (!g_strcmp0 (sequence->type, "string")) {
            keyval = sequence->value.string[i];
        } else if (!g_strcmp0 (sequence->type, "keys")) {
            keyval = sequence->value.keys[i].keyval;
            keycode = sequence->value.keys[i].keycode;
            state = sequence->value.keys[i].state;
        }
        if (keyval == 0 && keycode == 0 && state == 0)
            break;
        g_signal_emit_by_name (m_engine, "process-key-event",
                               keyval, keycode, state, &retval);
        state |= IBUS_RELEASE_MASK;
        sleep(1);
        g_signal_emit_by_name (m_engine, "process-key-event",
                               keyval, keycode, state, &retval);
    }
}


static void
set_engine_cb (GObject *object, GAsyncResult *res, gpointer data)
{
    IBusBus *bus = IBUS_BUS (object);
    GtkWidget *entry = GTK_WIDGET (data);
    g_autoptr(GError) error = NULL;
    IBusEngineCIConfig *ciconfig;
    IBusCITest *tests;
    g_autofree gchar *desc = NULL;

    if (g_test_verbose ())
        g_printerr ("set_engine_cb\n");
    if (!ibus_bus_set_global_engine_async_finish (bus, res, &error)) {
        g_autofree gchar *msg = g_strdup_printf ("set engine failed: %s",
                                                 error->message);
        g_test_incomplete (msg);
        return;
    }

    ciconfig = g_object_get_data (G_OBJECT (entry), "ciconfig");
    g_return_if_fail (IBUS_IS_ENGINE_CI_CONFIG (ciconfig));
    if (enable_ime (ciconfig))
        sleep (1);

    tests = ibus_engine_ci_config_get_tests (ciconfig);
    g_assert (tests);
    desc = g_strdup_printf ("%s preedit", tests[test_index].desc);
    test_key_sequences_case (tests[test_index].preedit, desc);
}

static gboolean
entry_focus_in_event_cb (GtkWidget     *entry,
                         GdkEventFocus *event,
                         gpointer       data)
{
    if (g_test_verbose ())
        g_printerr ("focus_in_event_cb\n");
    g_assert (m_bus != NULL);
    ibus_bus_set_global_engine_async (m_bus,
                                      "hangultest",
                                      -1,
                                      NULL,
                                      set_engine_cb,
                                      entry);
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
        g_printerr ("preedit_changed_cb %s\n",
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
    if (len > strlen (preedit_str))
        return;
    desc = g_strdup_printf ("%s commit", tests[test_index].desc);
    test_key_sequences_case (tests[test_index].commit, desc);
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
    const char *expected;
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
    expected = tests[test_index++].result->value.string;
    if (0 != g_strcmp0 (chars, expected))
        valid_output = FALSE;
    if (valid_output) {
        test = GREEN "PASS" NC;
    } else {
        test = RED "FAIL" NC;
        g_test_fail ();
    }
    g_print ("%05d %s expected: %s typed: %s\n",
             test_index, test, expected, chars);

    preedit = tests[test_index].preedit;
    if (!preedit) {
        g_signal_handlers_disconnect_by_func (
                entry,
                G_CALLBACK (entry_focus_in_event_cb),
                NULL);
        //g_timeout_add_seconds (10, finit, NULL);
        gtk_main_quit ();
        return;
    }

#if !GTK_CHECK_VERSION (3, 22, 16)
    n_loop++;
#endif
    gtk_entry_set_text (entry, "");
    desc = g_strdup_printf ("%s preedit", tests[test_index].desc);
    test_key_sequences_case (preedit, desc);
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
    if (!register_ibus_engine ()) {
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
    gboolean arg_fails = FALSE;
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
    //option_gtk = gtk_get_option_group (TRUE);
    option_gtk = gtk_get_option_group (FALSE);
    g_option_context_add_group (option_context, option_gtk);
    g_option_context_set_help_enabled (option_context, FALSE);

    g_test_init (&argc, &argv, NULL);
    if (!g_option_context_parse (option_context, &argc, &argv, &error)) {
        g_printerr ("Option parsing failed: %s\n", error->message);
        exit (EXIT_FAILURE);
    }
    gtk_init (&argc, &argv);
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

    test_path = g_strdup ("/ibus-engine-gui-ci/test1");
    g_test_add_data_func_full (test_path,
                               g_object_ref_sink (ciconfig),
                               test_engine_ci,
                               g_object_unref);
    g_free (test_path);

    keymap = ibus_keymap_get("us");

    return g_test_run ();
}
