#include <gtkrtg/gtkrtgtoolbar.h>
#include <set>

#define GTK_RTG_TOOLBAR_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GTK_RTG_TYPE_TOOLBAR, GtkRtgToolbarPrivate))

G_DEFINE_TYPE(GtkRtgToolbar, gtk_rtg_toolbar, GTK_TYPE_TOOLBAR)

static void gtk_rtg_toolbar_destroy(GtkWidget *object);

enum
{
	TOGGLE_VIEW,
	MAX_VIEWPORT,
	TOGGLE_SELECT,
    ZOOM_EXTENTS,
    MOVE,
	SCALE,
	ROTATE,
    SNAP_TO_GRID,
	BUTTONS
};

struct _GtkRtgToolbarPrivate
{
	GtkWidget *hbox;
	std::set<GtkRtgCanvas *> *s_canvas;
    GtkWidget *wfocused;
	GdkEventFocus event;
	guint once:1;
	GtkWidget *button[BUTTONS];
};

/* signals */
enum
{
	MAXIMIZE_VIEWPORT,
    REFOCUS,
	LAST_SIGNAL
};

static  gint tb_signals[LAST_SIGNAL] = { 0 };

static void
emit_refocus(GtkWidget *widget, GtkRtgToolbar *tb)
{
    g_signal_emit(tb, tb_signals[REFOCUS], 0);
}

static void
gtk_rtg_toolbar_class_init(GtkRtgToolbarClass *klass)
{
	GtkWidgetClass *obj_class = GTK_WIDGET_CLASS(klass);

	obj_class->destroy = gtk_rtg_toolbar_destroy;

	/*
     * GtkRtgToolbar: maximize-viewport
     *
     * Toolbar emits this signal when "maximize viewport" button is clicked..
     *
     */
	tb_signals[MAXIMIZE_VIEWPORT] = g_signal_new("maximize-viewport", G_OBJECT_CLASS_TYPE(klass), (GSignalFlags)G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(GtkRtgToolbarClass, maximize_viewport), NULL, NULL,
												   g_cclosure_marshal_VOID__OBJECT,
												   G_TYPE_NONE,
												   1,
												   G_TYPE_OBJECT);

	/*
     * GtkRtgToolbar: done
     *
     * Toolbar emits this signal, in order to refocus on widget.
     *
     */
	tb_signals[REFOCUS] = g_signal_new("refocus", G_OBJECT_CLASS_TYPE(klass), (GSignalFlags)G_SIGNAL_RUN_FIRST, G_STRUCT_OFFSET(GtkRtgToolbarClass, refocus), NULL, NULL,
										 g_cclosure_marshal_VOID__VOID,
										 G_TYPE_NONE,
										 0);

	g_type_class_add_private(G_OBJECT_CLASS(klass), sizeof(GtkRtgToolbarPrivate));
}

GtkWidget *
gtk_rtg_toolbar_new()
{
	return GTK_WIDGET(g_object_new(GTK_RTG_TYPE_TOOLBAR, NULL));
}

static void
gtk_rtg_toolbar_destroy(GtkWidget *object)
{
	GtkRtgToolbar *tb = GTK_RTG_TOOLBAR(object);

	if(tb->priv->s_canvas)
	{
		for(std::set<GtkRtgCanvas *>::const_iterator it=tb->priv->s_canvas->begin(); it!=tb->priv->s_canvas->end(); ++it)
			g_object_unref(G_OBJECT(*it));

		delete  tb->priv->s_canvas;
		tb->priv->s_canvas = NULL;
	}
    GTK_WIDGET_CLASS(gtk_rtg_toolbar_parent_class)->destroy(object);
}

static void
select_tool(GtkRtgToolbar *tb, GtkWidget *selected)
{
    for(guint id : {TOGGLE_VIEW, TOGGLE_SELECT, MOVE, ROTATE, SCALE})
    {
        GtkWidget *button = tb->priv->button[id];
        if(button && button != selected && gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)))
        {
            /* GTK also emits clicked when set_active changes a toggle button. */
            g_signal_handlers_block_matched(button, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, tb);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), FALSE);
            g_signal_handlers_unblock_matched(button, G_SIGNAL_MATCH_DATA, 0, 0, NULL, NULL, tb);
        }
    }
}

static gboolean
canvas_view_ctrl(GtkWidget *widget, gpointer data)
{
	GtkRtgToolbar *tb = GTK_RTG_TOOLBAR(data);
    GtkRtgCanvasInteractiveMode mode = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) ? GTK_RTG_CANVAS_INT_MODE_VIEW : GTK_RTG_CANVAS_INT_MODE_OFF;

	/* fix toggle */
    select_tool(tb, widget);

	for(std::set<GtkRtgCanvas *>::const_iterator it=tb->priv->s_canvas->begin(); it!=tb->priv->s_canvas->end(); ++it)
		gtk_rtg_canvas_interactive_set_mode(*it, mode);

	return FALSE;
}


static gboolean
canvas_view_select(GtkWidget *widget, gpointer data)
{
    GtkRtgToolbar *tb = GTK_RTG_TOOLBAR(data);

    GtkRtgCanvasInteractiveMode mode = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) ? GTK_RTG_CANVAS_INT_MODE_SELECT : GTK_RTG_CANVAS_INT_MODE_OFF;

	/* fix toggle */
    select_tool(tb, widget);

    for(std::set<GtkRtgCanvas *>::const_iterator it=tb->priv->s_canvas->begin(); it!=tb->priv->s_canvas->end(); ++it)
        gtk_rtg_canvas_interactive_set_mode(*it, mode);

    return FALSE;
}

static gboolean
canvas_view_move(GtkWidget *widget, gpointer data)
{
    GtkRtgToolbar *tb = GTK_RTG_TOOLBAR(data);

    GtkRtgCanvasInteractiveMode mode = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) ? GTK_RTG_CANVAS_INT_MODE_MOVE : GTK_RTG_CANVAS_INT_MODE_OFF;

    /* fix toggle */
    select_tool(tb, widget);

    for(std::set<GtkRtgCanvas *>::const_iterator it=tb->priv->s_canvas->begin(); it!=tb->priv->s_canvas->end(); ++it)
        gtk_rtg_canvas_interactive_set_mode(*it, mode);

    return FALSE;
}


static gboolean
canvas_view_scale(GtkWidget *widget, gpointer data)
{
	GtkRtgToolbar *tb = GTK_RTG_TOOLBAR(data);

	GtkRtgCanvasInteractiveMode mode = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) ? GTK_RTG_CANVAS_INT_MODE_SCALE: GTK_RTG_CANVAS_INT_MODE_OFF;

	/* fix toggle */
	select_tool(tb, widget);

	for(std::set<GtkRtgCanvas *>::const_iterator it=tb->priv->s_canvas->begin(); it!=tb->priv->s_canvas->end(); ++it)
		gtk_rtg_canvas_interactive_set_mode(*it, mode);

	return FALSE;
}


static gboolean
canvas_snap_to_grid(GtkWidget *widget, gpointer data)
{
	GtkRtgToolbar *tb = GTK_RTG_TOOLBAR(data);

	for(std::set<GtkRtgCanvas *>::const_iterator it=tb->priv->s_canvas->begin(); it!=tb->priv->s_canvas->end(); ++it)
    {
        GtkRtgCanvasGrid *grid = gtk_rtg_canvas_get_grid(*it);

        /* toggle snap-cursor */
        if(grid)
            gtk_rtg_canvas_grid_set_snap_cursor(grid, gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
    }

    return FALSE;
}


static gboolean
canvas_view_rotate(GtkWidget *widget, gpointer data)
{
	GtkRtgToolbar *tb = GTK_RTG_TOOLBAR(data);

	GtkRtgCanvasInteractiveMode mode = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) ? GTK_RTG_CANVAS_INT_MODE_ROTATE: GTK_RTG_CANVAS_INT_MODE_OFF;

	/* fix toggle */
	select_tool(tb, widget);

	for(std::set<GtkRtgCanvas *>::const_iterator it=tb->priv->s_canvas->begin(); it!=tb->priv->s_canvas->end(); ++it)
		gtk_rtg_canvas_interactive_set_mode(*it, mode);

	return FALSE;
}


static gboolean
maximize_viewport(GtkWidget *widget, gpointer data)
{
	GtkRtgToolbar *tb = GTK_RTG_TOOLBAR(data);

	for(std::set<GtkRtgCanvas *>::const_iterator it=tb->priv->s_canvas->begin(); it!=tb->priv->s_canvas->end(); ++it)
		if(GTK_RTG_IS_CANVAS(*it) && gtk_rtg_canvas_is_focused(*it))
		{
			g_signal_emit(tb, tb_signals[MAXIMIZE_VIEWPORT], 0, *it);
			break;
		}

	return FALSE;
}


static gboolean
zoom_extents(GtkWidget *widget, gpointer data)
{
    GtkRtgToolbar *tb = GTK_RTG_TOOLBAR(data);

    for(std::set<GtkRtgCanvas *>::const_iterator it=tb->priv->s_canvas->begin(); it!=tb->priv->s_canvas->end(); ++it)
        if(GTK_RTG_IS_CANVAS(*it) && gtk_rtg_canvas_is_focused(*it))
        {
            gtk_rtg_canvas_view_zoom_extents(GTK_RTG_CANVAS(*it), TRUE);
            break;
        }

    return FALSE;
}


static inline GtkWidget *
create_n_pack_toggle_button(GtkWidget *group, const gchar *stock_id, const gchar *tooltip, gboolean toggle = TRUE)
{
	GtkWidget *button = toggle ? gtk_toggle_button_new() : gtk_button_new();

    if(gtk_icon_theme_has_icon(gtk_icon_theme_get_default(), stock_id))
        gtk_button_set_image(GTK_BUTTON(button), gtk_image_new_from_icon_name(stock_id, GTK_ICON_SIZE_LARGE_TOOLBAR));
	gtk_widget_set_tooltip_text(button, tooltip);
    gchar *label = g_strdup(tooltip);
    gtk_button_set_label(GTK_BUTTON(button), g_strstrip(label));
    gtk_button_set_always_show_image(GTK_BUTTON(button), TRUE);
    g_free(label);
	gtk_box_pack_start(GTK_BOX(group), button, FALSE, FALSE, 0);

    return button;
}


static void
gtk_rtg_toolbar_init(GtkRtgToolbar *tb)
{
	tb->priv = GTK_RTG_TOOLBAR_GET_PRIVATE(tb);

	tb->priv->s_canvas = new std::set<GtkRtgCanvas *>;

	tb->priv->hbox   = gtk_hbox_new(FALSE, 3);
	GtkWidget *group = gtk_hbox_new(TRUE, 1);

	tb->priv->button[TOGGLE_VIEW] = create_n_pack_toggle_button(group, "view-refresh-symbolic", " Orbit ");
    g_signal_connect(tb->priv->button[TOGGLE_VIEW], "clicked", G_CALLBACK(canvas_view_ctrl), tb);

	tb->priv->button[MAX_VIEWPORT] = create_n_pack_toggle_button(group, "view-fullscreen-symbolic", " Maximize ");
    gtk_widget_set_sensitive(tb->priv->button[MAX_VIEWPORT], FALSE);
    g_signal_connect(tb->priv->button[MAX_VIEWPORT], "clicked", G_CALLBACK(maximize_viewport), tb);

	tb->priv->button[ZOOM_EXTENTS] = create_n_pack_toggle_button(group, "zoom-fit-best-symbolic", " Fit ", FALSE);
    g_signal_connect(tb->priv->button[ZOOM_EXTENTS], "clicked", G_CALLBACK(zoom_extents), tb);

    GtkWidget *vseparator = gtk_vseparator_new();
	gtk_box_pack_start(GTK_BOX(group), vseparator, FALSE, FALSE, 0);

	tb->priv->button[TOGGLE_SELECT] = create_n_pack_toggle_button(group, "edit-select-all-symbolic", " Select ");
    g_signal_connect(tb->priv->button[TOGGLE_SELECT], "clicked", G_CALLBACK(canvas_view_select), tb);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(tb->priv->button[TOGGLE_SELECT]), TRUE);

	tb->priv->button[MOVE] = create_n_pack_toggle_button(group, "transform-move-symbolic", " Move ");
    g_signal_connect(tb->priv->button[MOVE], "clicked", G_CALLBACK(canvas_view_move), tb);

	tb->priv->button[ROTATE] = create_n_pack_toggle_button(group, "object-rotate-right-symbolic", " Rotate ");
    g_signal_connect(tb->priv->button[ROTATE], "clicked", G_CALLBACK(canvas_view_rotate), tb);

	tb->priv->button[SCALE] = create_n_pack_toggle_button(group, "view-restore-symbolic", " Scale ");
    g_signal_connect(tb->priv->button[SCALE], "clicked", G_CALLBACK(canvas_view_scale), tb);

	vseparator = gtk_vseparator_new();
	gtk_box_pack_start(GTK_BOX(group), vseparator, FALSE, FALSE, 0);

    gtk_box_pack_start(GTK_BOX(tb->priv->hbox), group, FALSE, FALSE, 0);

    // start new group
	group = gtk_hbox_new(TRUE, 1);

	tb->priv->button[SNAP_TO_GRID] = create_n_pack_toggle_button(group, "view-grid-symbolic", " Snap to Grid ");
    g_signal_connect(tb->priv->button[SNAP_TO_GRID], "clicked", G_CALLBACK(canvas_snap_to_grid), tb);

    gtk_box_pack_start(GTK_BOX(tb->priv->hbox), group, FALSE, FALSE, 0);
	GtkToolItem *item = gtk_tool_item_new();
    gtk_container_add(GTK_CONTAINER(item), tb->priv->hbox);
    gtk_toolbar_insert(GTK_TOOLBAR(tb), item, -1);

    /* connect refocus */
    for(guint i=0; i<BUTTONS; ++i)
        g_signal_connect_after(tb->priv->button[i], "clicked", G_CALLBACK(emit_refocus), tb);
}


/*
 * Publics
 *
 */

 void
gtk_rtg_toolbar_canvas_push(GtkRtgToolbar *tb, GtkRtgCanvas *canvas, ...) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_TOOLBAR(tb));
	g_throw_if_fail(GTK_RTG_IS_CANVAS(canvas));

	va_list args;
	va_start(args, canvas);

	GtkRtgCanvas *c = canvas;

	while(c != NULL)
	{
		std::set<GtkRtgCanvas *>::const_iterator it = tb->priv->s_canvas->find(c);

		if(it == tb->priv->s_canvas->end()) /* avoid duplicates (over-referencing) */
		{
			g_object_ref(G_OBJECT(c));
			tb->priv->s_canvas->insert(c);
        }

		c = va_arg(args, GtkRtgCanvas *);
		if(c == NULL)
			break;

		g_throw_if_fail(GTK_RTG_IS_CANVAS(c));
	}

	va_end(args);

	if(tb->priv->s_canvas->size() > 1)
		gtk_widget_set_sensitive(tb->priv->button[1], TRUE);
}

gboolean
gtk_rtg_toolbar_canvas_is_pushed(GtkRtgToolbar *tb, GtkRtgCanvas *canvas) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_TOOLBAR(tb));
	g_throw_if_fail(GTK_RTG_IS_CANVAS(canvas));

	std::set<GtkRtgCanvas *>::const_iterator it = tb->priv->s_canvas->find(canvas);

	return it != tb->priv->s_canvas->end();
}

 void
gtk_rtg_toolbar_canvas_pop(GtkRtgToolbar *tb, GtkRtgCanvas *canvas) throw (GtkRtgException)
{
	g_throw_if_fail(GTK_RTG_IS_TOOLBAR(tb));
	g_throw_if_fail(GTK_RTG_IS_CANVAS(canvas));

	std::set<GtkRtgCanvas *>::const_iterator it = tb->priv->s_canvas->find(canvas);

	if(it != tb->priv->s_canvas->end())
	{
		g_object_unref(G_OBJECT(*it));
		tb->priv->s_canvas->erase(it);
	}
}
