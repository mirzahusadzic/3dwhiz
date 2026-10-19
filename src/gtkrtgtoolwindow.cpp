#include <gtkrtg/gtkrtgtoolwindow.h>

G_DEFINE_TYPE(GtkRtgToolWindow, gtk_rtg_tool_window, GTK_TYPE_WINDOW)

static void
gtk_rtg_tool_window_class_init(GtkRtgToolWindowClass *)
{
}

static void
gtk_rtg_tool_window_init(GtkRtgToolWindow *window)
{
    window->priv = nullptr;
    gtk_window_set_type_hint(GTK_WINDOW(window), GDK_WINDOW_TYPE_HINT_UTILITY);
    gtk_window_set_skip_taskbar_hint(GTK_WINDOW(window), TRUE);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER_ON_PARENT);
}

GtkWidget *
gtk_rtg_tool_window_new(GtkWindow *transient)
{
    auto *widget = GTK_WIDGET(g_object_new(GTK_RTG_TYPE_TOOL_WINDOW, "resizable", FALSE, nullptr));
    if(transient)
        gtk_window_set_transient_for(GTK_WINDOW(widget), transient);
    return widget;
}
