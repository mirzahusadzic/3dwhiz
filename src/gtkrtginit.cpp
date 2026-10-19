#include <gtkrtg/gtkrtginit.h>
#include "gtkrtgresources.h"
#include <cstdlib>

gboolean
gtk_rtg_init_check(int *argc, char ***argv)
{
#ifdef WHIZ_GTK_DATA_DIR
    /* Set this before GTK/GLib cache their data directories and schema source. */
    const gchar *configured = WHIZ_GTK_DATA_DIR;
    const gchar *inherited = g_getenv("XDG_DATA_DIRS");
    if(!inherited || !*inherited)
        inherited = "/usr/local/share:/usr/share";
    gchar **directories = g_strsplit(inherited, G_SEARCHPATH_SEPARATOR_S, -1);
    gboolean present = FALSE;
    for(gchar **directory = directories; *directory; ++directory)
        if(g_strcmp0(*directory, configured) == 0)
            present = TRUE;
    g_strfreev(directories);
    if(!present && g_file_test(configured, G_FILE_TEST_IS_DIR))
    {
        gchar *combined = g_strconcat(inherited, G_SEARCHPATH_SEPARATOR_S, configured, nullptr);
        g_setenv("XDG_DATA_DIRS", combined, TRUE);
        g_free(combined);
    }
#endif
#ifndef __APPLE__
    // The recovered renderer uses fixed-function desktop OpenGL.
    if(!g_getenv("GDK_GL"))
        g_setenv("GDK_GL", "legacy", FALSE);
#endif
    if(!gtk_init_check(argc, argv))
        return FALSE;
    static bool initialized = false;
    if(!initialized)
    {
        gtk_rtg_resources_init();
        std::atexit(gtk_rtg_resources_destroy);
        initialized = true;
    }
    return TRUE;
}

void
gtk_rtg_init(int *argc, char ***argv)
{
    if(!gtk_rtg_init_check(argc, argv))
        std::exit(1);
}
