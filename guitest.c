#include <unistd.h>
#include <gtk/gtk.h>
#include <ibus.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "guitest.h"
#include "ciconfig.h"

#define GREEN "\033[0;32m"
#define RED   "\033[0;31m"
#define NC    "\033[0m"

typedef struct {
    const char * input;
    const char * output;
}TestCase;

IBusBus *m_bus;
IBusEngine *m_engine;

IBusKeymap *keymap;

const guint m_switch_hangul[3] = {IBUS_Hangul, 122, 0};

const TestCase m_test_cases[] =
{
   {"rk ", "가"}
};

static GOptionContext *option_context;
static GOptionGroup *option_gtk;
static char *case_path;
static char *config_dir;
static char *exec_dir;

static void show_version (void);
static void show_help (void);
static void show_help_gtk (void);
static void show_help_test (void);
static gboolean window_focus_in_event_cb (GtkWidget     *entry,
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
    char * const argv[] = { "ibus-engine-gui-ci", "--help" };
    g_autoptr(GOptionContext) context = g_option_context_new ("- ibus-engine-gui-ci");
    char *help_contents = g_option_context_get_help (context, TRUE, NULL);
    g_print ("%s\n", help_contents);
    g_free (help_contents);
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
enable_ime ()
{
    int i;
    const IBusCIKey *keys = IBUS_CI_TEST_CASES.init;

    if (!keys)
        return FALSE;
    if (keys->keyval == 0 || keys->keycode == 0)
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

static gboolean
test_keypress_cases ()
{
    const IBusCITest *tests = IBUS_CI_TEST_CASES.tests;
    g_return_val_if_fail (tests, FALSE);
    while (tests) {
        const IBusCIKeySequence *preedit = &tests->preedit;
        const IBusCIKeySequence *conversion = &tests->conversion;
        const IBusCIKeySequence *commit = &tests->commit;
        tests++;
    }
}

static void
set_engine_cb (GObject *object, GAsyncResult *res, gpointer data)
{
    IBusBus *bus = IBUS_BUS (object);
    GtkWidget *entry = GTK_WIDGET (data);
    GError *error = NULL;
    gint i;
    const gchar *p = NULL;

    if (!ibus_bus_set_global_engine_async_finish (bus, res, &error)) {
        gchar *msg = g_strdup_printf ("set engine failed: %s", error->message);
        g_test_incomplete (msg);
        g_free (msg);
        g_error_free (error);
        return;
    }

    if (enable_ime ())
        sleep (1);

    {
        /* Run test cases */
        for (i = 0;
             i < G_N_ELEMENTS(m_test_cases);
             i++) {
            for (p = m_test_cases[i].input; *p; p++) {
                gboolean retval;
                guint keyval = *p;
                guint modifiers = 0;
                guint keycode = guess_keycode (keymap, keyval, modifiers);

                if (keyval == 0)
                    break;
                g_signal_emit_by_name (m_engine, "process-key-event",
                                       keyval, keycode, modifiers, &retval);
                modifiers |= IBUS_RELEASE_MASK;
                sleep(1);
                g_signal_emit_by_name (m_engine, "process-key-event",
                                       keyval, keycode, modifiers, &retval);
            }
        }
    }

    g_signal_handlers_disconnect_by_func (entry,
                                          G_CALLBACK (window_focus_in_event_cb),
                                          NULL);
    g_timeout_add_seconds (10, finit, NULL);
}

static gboolean
window_focus_in_event_cb (GtkWidget *entry, GdkEventFocus *event, gpointer data)
{
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
window_inserted_text_cb (GtkEntryBuffer *buffer,
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
    static guint index = 0;
    gunichar code = g_utf8_get_char (chars);
    const gchar *test;
    GtkEntry *entry = GTK_ENTRY (data);

#if !GTK_CHECK_VERSION (3, 22, 16)
    if (n_loop % 2 == 1) {
        n_loop = 0;
        return;
    }
#endif

    {
        /* Run test case */
        const gchar *p = chars;
        const gchar *output = m_test_cases[index].output;
        guint j = 0;
        gboolean valid_output = TRUE;

        if (0 != g_strcmp0 (p, output))
            valid_output = FALSE;
        index++;

        if (valid_output) {
            test = GREEN "PASS" NC;
        } else {
            test = RED "FAIL" NC;
            g_test_fail ();
        }
        g_print ("%05d %s expected: %s typed: %s\n",
                 index, test, output, p);
    }

    if (index == G_N_ELEMENTS(m_test_cases)) {
        gtk_main_quit ();
        return;
    }

#if !GTK_CHECK_VERSION (3, 22, 16)
    n_loop++;
#endif
    gtk_entry_set_text (entry, "");
}

static void
create_window ()
{
    GtkWidget *window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    GtkWidget *entry = gtk_entry_new ();
    GtkEntryBuffer *buffer;

    g_signal_connect (window, "destroy",
                      G_CALLBACK (gtk_main_quit), NULL);
    g_signal_connect (entry, "focus-in-event",
                      G_CALLBACK (window_focus_in_event_cb), NULL);
    buffer = gtk_entry_get_buffer (GTK_ENTRY (entry));
    g_signal_connect (buffer, "inserted-text",
                      G_CALLBACK (window_inserted_text_cb), entry);
    gtk_container_add (GTK_CONTAINER (window), entry);
    gtk_widget_show_all (window);
}

static void
test_hangul (void)
{
    GLogLevelFlags flags;
    if (!register_ibus_engine ()) {
        g_test_fail ();
        return;
    }

    ibus_engine_ci_init (m_bus);

    create_window ();
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
    g_autoptr(IBusEngineCIConfig) ciconfig;
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

    if (!g_option_context_parse (option_context, &argc, &argv, &error)) {
        g_printerr ("Option parsing failed: %s\n", error->message);
        exit (EXIT_FAILURE);
    }
    g_test_init (&argc, &argv, NULL);
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

    test_path = g_strdup ("/ibus-hangul/test1");
    g_test_add_func (test_path, test_hangul);
    g_free (test_path);

    keymap = ibus_keymap_get("us");

    return g_test_run ();
}
