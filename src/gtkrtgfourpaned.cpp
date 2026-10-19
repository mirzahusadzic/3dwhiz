#include <gtkrtg/gtkrtgfourpaned.h>
#include <gtkrtg/gtkrtgcanvas.h>
#include <math.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "marshalers.h"

#define GTK_RTG_FOUR_PANED_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), GTK_RTG_TYPE_FOUR_PANED, GtkRtgFourPanedPrivate))

G_DEFINE_TYPE(GtkRtgFourPaned, gtk_rtg_four_paned, GTK_TYPE_CONTAINER)

struct Pane
{
    GtkWidget *widget;
    guint resize;
    guint shrink;
};

enum
{
    PANE_0,
    PANE_1,
    PANE_2,
    PANE_3,
    PANES
};

struct Handle
{
    GdkWindow *window; /* input only  */
    GdkRectangle pos; /* dimensions  */
    guint cursor; /* cursor id	  */
    gfloat drag_coord; /* dragging coordinate (x or y) */

    /* flags */
    gboolean drag;
    gboolean visible;
};

enum
{
    HANDLE_H,
    HANDLE_V,
    HANDLES
};

enum
{
    CURSOR_H,
    CURSOR_V,
    CURSOR_CENTER,
    CURSORS
};

struct _GtkRtgFourPanedPrivate
{
    Pane p[PANES];
    Handle h[HANDLES];
    GdkCursor *curs[CURSORS];
    GtkWidget *menu;
    guint macro_pane:1;
    gboolean layout_initialized;
    gboolean saved_visible[PANES];
};

#define HANLDE_SIZE		 	 5
#define HANDLE_SIZE_CENTER	 4
#define DRAW_HANDLE_ABOVE	60		/* will not draw handle with dots below this length */
#define WIDGET_COLOR		"#f5f5f5"
#define PARENT_WIDGET_COLOR	"#c5c5c5"

/* signals */
enum
{
    TOGGLE_HANDLE_FOCUS,
    LAST_SIGNAL
};

static  gint signals[LAST_SIGNAL] = { 0 };

static void gtk_rtg_four_paned_add(GtkContainer *container, GtkWidget *widget);
static void gtk_rtg_four_paned_remove(GtkContainer *container, GtkWidget *widget);
static void gtk_rtg_four_paned_forall(GtkContainer *container, gboolean include_internals, GtkCallback callback, gpointer callback_data);
static void gtk_rtg_four_paned_destroy(GtkWidget *widget);
static void gtk_rtg_four_paned_realize(GtkWidget *widget);
static void gtk_rtg_four_paned_unrealize(GtkWidget *widget);
static void gtk_rtg_four_paned_map(GtkWidget *widget);
static void gtk_rtg_four_paned_unmap(GtkWidget *widget);
static void gtk_rtg_four_paned_preferred_width(GtkWidget *widget, gint *minimum, gint *natural);
static void gtk_rtg_four_paned_preferred_height(GtkWidget *widget, gint *minimum, gint *natural);
static gboolean gtk_rtg_four_paned_expose(GtkWidget *widget, cairo_t *cr);
static void gtk_rtg_four_paned_size_request(GtkWidget *widget, GtkRequisition *requisition);
static void gtk_rtg_four_paned_size_allocate(GtkWidget *widget, GtkAllocation  *allocation);
static gboolean gtk_rtg_four_paned_button_press(GtkWidget *widget, GdkEventButton *event);
static gboolean gtk_rtg_four_paned_motion_notify(GtkWidget *widget, GdkEventMotion *event);
static gboolean gtk_rtg_four_paned_button_release(GtkWidget *widget, GdkEventButton *event);
static void gtk_rtg_four_paned_grab_focus(GtkWidget *widget);
static gboolean gtk_rtg_four_paned_toggle_handle_focus(GtkRtgFourPaned *four_paned);
static void	gtk_rtg_canvas_focused(GtkRtgCanvas *canvas, gpointer data);
static void	gtk_rtg_canvas_gizmo_position(GtkRtgCanvas *canvas, gtkrtg::point3f *pos, gpointer data);

/* macros */
#define IS_PANE(widget)(widget && gtk_widget_get_visible(widget))
#define NOT_PANE(widget)(!IS_PANE(widget))

static GtkAllocation
widget_allocation(GtkWidget *widget)
{
    GtkAllocation allocation;
    gtk_widget_get_allocation(widget, &allocation);
    return allocation;
}

static void
event_position(GtkWidget *widget, GdkEventMotion *event, gint *x, gint *y)
{
    gint origin_x, origin_y, parent_x, parent_y;
    gdk_window_get_origin(event->window, &origin_x, &origin_y);
    gdk_window_get_origin(gtk_widget_get_window(widget), &parent_x, &parent_y);
    *x = gint(event->x) + origin_x - parent_x;
    *y = gint(event->y) + origin_y - parent_y;
}

static void
add_tab_bindings(GtkBindingSet *binding_set, GdkModifierType   modifiers)
{
    gtk_binding_entry_add_signal(binding_set, GDK_KEY_Tab,    modifiers, "toggle_handle_focus", 0);
    gtk_binding_entry_add_signal(binding_set, GDK_KEY_KP_Tab, modifiers, "toggle_handle_focus", 0);
}

static void
gtk_rtg_four_paned_class_init(GtkRtgFourPanedClass *klass)
{
    GObjectClass *o_class = G_OBJECT_CLASS(klass);
    GtkWidgetClass *widget_class = GTK_WIDGET_CLASS(klass);
    GtkContainerClass *container_class = GTK_CONTAINER_CLASS(klass);
    GtkBindingSet *binding_set;

    widget_class->destroy = gtk_rtg_four_paned_destroy;
    widget_class->realize = gtk_rtg_four_paned_realize;
    widget_class->draw = gtk_rtg_four_paned_expose;
    widget_class->get_preferred_width = gtk_rtg_four_paned_preferred_width;
    widget_class->get_preferred_height = gtk_rtg_four_paned_preferred_height;
    widget_class->unrealize = gtk_rtg_four_paned_unrealize;
    widget_class->map = gtk_rtg_four_paned_map;
    widget_class->unmap = gtk_rtg_four_paned_unmap;
    widget_class->size_allocate = gtk_rtg_four_paned_size_allocate;
    widget_class->button_press_event = gtk_rtg_four_paned_button_press;
    widget_class->motion_notify_event = gtk_rtg_four_paned_motion_notify;
    widget_class->button_release_event = gtk_rtg_four_paned_button_release;
    widget_class->grab_focus = gtk_rtg_four_paned_grab_focus;
    container_class->add = gtk_rtg_four_paned_add;
    container_class->remove = gtk_rtg_four_paned_remove;
    container_class->forall = gtk_rtg_four_paned_forall;
    klass->toggle_handle_focus = gtk_rtg_four_paned_toggle_handle_focus;

    signals[TOGGLE_HANDLE_FOCUS] = g_signal_new("toggle_handle_focus",
                                                G_TYPE_FROM_CLASS(o_class),
                                                (GSignalFlags)(G_SIGNAL_RUN_LAST | G_SIGNAL_ACTION),
                                                G_STRUCT_OFFSET(GtkRtgFourPanedClass, toggle_handle_focus),
                                                NULL,
                                                NULL,
                                                gtkrtg_marshal__BOOLEAN__VOID,
                                                G_TYPE_BOOLEAN,
                                                0);

    binding_set = gtk_binding_set_by_class(klass);

    add_tab_bindings(binding_set, (GdkModifierType)0);
    add_tab_bindings(binding_set, GDK_CONTROL_MASK);
    add_tab_bindings(binding_set, GDK_SHIFT_MASK);
    add_tab_bindings(binding_set, (GdkModifierType)(GDK_CONTROL_MASK | GDK_SHIFT_MASK));

    g_type_class_add_private(G_OBJECT_CLASS(klass), sizeof(GtkRtgFourPanedPrivate));
}


GtkWidget *
gtk_rtg_four_paned_new()
{
    return GTK_WIDGET(g_object_new(GTK_RTG_TYPE_FOUR_PANED, NULL));
}


static void
reset_layout(GtkMenuItem *item, gpointer data)
{
    GtkRtgFourPaned *paned = GTK_RTG_FOUR_PANED(data);
    Handle *h = paned->priv->h;
    GtkAllocation allocation = widget_allocation(GTK_WIDGET(paned));
    GtkAllocation *alloc = &allocation;

    h[HANDLE_H].drag_coord = gfloat(alloc->y) + gfloat(alloc->height) * .5f - gfloat(h[HANDLE_H].pos.height >> 1);
    h[HANDLE_V].drag_coord = gfloat(alloc->x) + gfloat(alloc->width)  * .5f - gfloat(h[HANDLE_V].pos.width  >> 1);

    gtk_widget_queue_resize(GTK_WIDGET(paned));
}


static void
gtk_rtg_four_paned_init(GtkRtgFourPaned *paned)
{
    gtk_widget_set_has_window(GTK_WIDGET(paned), FALSE);

    paned->priv = GTK_RTG_FOUR_PANED_GET_PRIVATE(paned);

    paned->priv->h[HANDLE_H].pos.x = 0;
    paned->priv->h[HANDLE_H].pos.y = -1;
    paned->priv->h[HANDLE_H].pos.width = -1;
    paned->priv->h[HANDLE_H].pos.height = HANLDE_SIZE;
    paned->priv->h[HANDLE_H].cursor = CURSOR_H;
    paned->priv->h[HANDLE_V].pos.x = -1;
    paned->priv->h[HANDLE_V].pos.width = HANLDE_SIZE;
    paned->priv->h[HANDLE_V].pos.height = -1;
    paned->priv->h[HANDLE_V].cursor = CURSOR_V;
    paned->priv->curs[CURSOR_H] = gdk_cursor_new(GDK_SB_V_DOUBLE_ARROW);
    paned->priv->curs[CURSOR_V] = gdk_cursor_new(GDK_SB_H_DOUBLE_ARROW);
    paned->priv->curs[CURSOR_CENTER] = gdk_cursor_new(GDK_FLEUR);

    /* menu */
    paned->priv->menu = gtk_menu_new();
    GtkWidget *menu_item = gtk_menu_item_new_with_label("Reset Layout");
    g_signal_connect(GTK_MENU_ITEM(menu_item), "activate", G_CALLBACK(reset_layout), paned);
    gtk_menu_shell_append(GTK_MENU_SHELL(paned->priv->menu), menu_item);
    gtk_widget_show_all(paned->priv->menu);
}

static void
create_handle_window(GtkWidget	   *widget,
                     GdkWindow	  **window,
                     GdkRectangle  &pos,
                     GdkCursorType  cursor)
{
    GdkWindowAttr attributes = {};
    gint attributes_mask;

    attributes.window_type = GDK_WINDOW_CHILD;
    attributes.wclass      = GDK_INPUT_ONLY;
    attributes.x           = pos.x;
    attributes.y           = pos.y;
    attributes.width       = MAX(pos.width, 1);
    attributes.height      = MAX(pos.height, 1);
    attributes.event_mask  = gtk_widget_get_events(widget);
    attributes.event_mask |= (GDK_BUTTON_PRESS_MASK |
                              GDK_BUTTON_RELEASE_MASK |
                              GDK_ENTER_NOTIFY_MASK |
                              GDK_LEAVE_NOTIFY_MASK |
                              GDK_POINTER_MOTION_MASK);

    attributes_mask = GDK_WA_X | GDK_WA_Y;

    if(gtk_widget_is_sensitive(GTK_WIDGET(widget)))
    {
        attributes.cursor = gdk_cursor_new_for_display(gtk_widget_get_display(widget), cursor);
        attributes_mask  |= GDK_WA_CURSOR;
    }

    *window = gdk_window_new(gtk_widget_get_window(widget), &attributes, attributes_mask);

    gtk_widget_register_window(widget, *window);

    if (attributes_mask & GDK_WA_CURSOR)
        g_object_unref(attributes.cursor);
}


static void
update_handles_visibility(Handle *h, Pane *p)
{
    const gboolean top = IS_PANE(p[PANE_0].widget) || IS_PANE(p[PANE_1].widget);
    const gboolean bottom = IS_PANE(p[PANE_2].widget) || IS_PANE(p[PANE_3].widget);
    const gboolean left = IS_PANE(p[PANE_0].widget) || IS_PANE(p[PANE_2].widget);
    const gboolean right = IS_PANE(p[PANE_1].widget) || IS_PANE(p[PANE_3].widget);
    h[HANDLE_H].visible = top && bottom;
    h[HANDLE_V].visible = left && right;
    for(guint i = 0; i < HANDLES; ++i)
        if(h[i].window)
        {
            if(h[i].visible) gdk_window_show(h[i].window);
            else gdk_window_hide(h[i].window);
        }
}


static void
gtk_rtg_four_paned_destroy(GtkWidget *widget)
{
    GtkRtgFourPaned *paned = GTK_RTG_FOUR_PANED(widget);

    if(paned->priv->menu)
    {
        g_object_ref_sink(paned->priv->menu);
        g_object_unref(paned->priv->menu);
        paned->priv->menu = 0;
    }

    for(guint i=0; i<CURSORS; ++i)
        if(paned->priv->curs[i])
        {
            g_object_unref(paned->priv->curs[i]);
            paned->priv->curs[i] = NULL;
        }

    GTK_WIDGET_CLASS(gtk_rtg_four_paned_parent_class)->destroy(widget);
}


static void
gtk_rtg_four_paned_realize(GtkWidget	*widget)
{
    GtkRtgFourPaned *paned = GTK_RTG_FOUR_PANED(widget);
    Handle *h = paned->priv->h;

    gtk_widget_set_realized(widget, TRUE);

    gtk_widget_set_window(widget, gtk_widget_get_parent_window(widget));
    g_object_ref(gtk_widget_get_window(widget));

    create_handle_window(widget, &h[HANDLE_H].window,      h[HANDLE_H].pos, GDK_SB_V_DOUBLE_ARROW);
    create_handle_window(widget, &h[HANDLE_V].window,      h[HANDLE_V].pos, GDK_SB_H_DOUBLE_ARROW);

    update_handles_visibility(h, paned->priv->p);

    gtk_widget_queue_resize(widget);
}


static void
menu_position(GtkMenu  *menu,
              gint     *x,
              gint     *y,
              gboolean *push_in,
              gpointer  user_data)
{
    GdkEventButton *event = (GdkEventButton*) user_data;

    *x = gint(event->x_root);
    *y = gint(event->y_root);
    *push_in = TRUE;
}


static gboolean
gtk_rtg_four_paned_button_press(GtkWidget *widget, GdkEventButton *event)
{
    GtkRtgFourPaned *paned = GTK_RTG_FOUR_PANED(widget);

    if(paned->priv->macro_pane)
        return FALSE;

    if(event->button > 1)
    {
        Handle *h = paned->priv->h;

        if(h[HANDLE_H].window == event->window || h[HANDLE_V].window == event->window)
        {
            /* show menu */
            gtk_menu_popup(GTK_MENU(paned->priv->menu), NULL, NULL, menu_position,
                           event, event->button, gdk_event_get_time((GdkEvent*)event));

        }
    }

    return FALSE;
}


static gboolean
gtk_rtg_four_paned_motion_notify(GtkWidget *widget, GdkEventMotion *event)
{
    GtkRtgFourPaned *paned = GTK_RTG_FOUR_PANED(widget);

    if(paned->priv->macro_pane)
        return FALSE;

    Handle *h = paned->priv->h;
    if(event->state & GDK_BUTTON1_MASK)
    {
        if(event->window != h[HANDLE_H].window && event->window != h[HANDLE_V].window)
            return FALSE;

        gint x, y;
        event_position(widget, event, &x, &y);
        const GtkAllocation allocation = widget_allocation(widget);
        const gboolean center = h[HANDLE_H].cursor == CURSOR_CENTER;
        if(center || event->window == h[HANDLE_H].window)
        {
            h[HANDLE_H].drag = TRUE;
            h[HANDLE_H].drag_coord = CLAMP(y - (h[HANDLE_H].pos.height >> 1),
                                          allocation.y,
                                          allocation.y + allocation.height - h[HANDLE_H].pos.height);
        }
        if(center || event->window == h[HANDLE_V].window)
        {
            h[HANDLE_V].drag = TRUE;
            h[HANDLE_V].drag_coord = CLAMP(x - (h[HANDLE_V].pos.width >> 1),
                                          allocation.x,
                                          allocation.x + allocation.width - h[HANDLE_V].pos.width);
        }

        /* Resize and repaint the children throughout the drag. */
        gtk_widget_queue_resize(widget);
        gtk_widget_queue_draw(widget);
        return TRUE;
    }else
    /* only handles */
    if(h[HANDLE_H].window == event->window || h[HANDLE_V].window == event->window)
    {
        if(gtk_widget_get_window(widget) && GDK_IS_WINDOW(gtk_widget_get_window(widget)))
        {
            gint width_2 = (h[HANDLE_V].pos.width  * HANDLE_SIZE_CENTER)>>1;
            gint height_2 = (h[HANDLE_H].pos.height * HANDLE_SIZE_CENTER)>>1;
            gint x = h[HANDLE_V].pos.x + (h[HANDLE_V].pos.width>>1);
            gint y = h[HANDLE_H].pos.y + (h[HANDLE_H].pos.height>>1);

            gint _x, _y;
            event_position(widget, event, &_x, &_y);

            /* center hit */
            if(_x > x - width_2 && _x < x + width_2 && _y > y - height_2 && _y < y + height_2)
            {
                if(h[HANDLE_H].cursor != CURSOR_CENTER)
                    gdk_window_set_cursor(h[HANDLE_H].window, paned->priv->curs[h[HANDLE_H].cursor=CURSOR_CENTER]);

                if(h[HANDLE_V].cursor != CURSOR_CENTER)
                    gdk_window_set_cursor(h[HANDLE_V].window, paned->priv->curs[h[HANDLE_V].cursor=CURSOR_CENTER]);
            }else
            /* reset cursors */
            {
                if(h[HANDLE_H].cursor != CURSOR_H)
                    gdk_window_set_cursor(h[HANDLE_H].window, paned->priv->curs[h[HANDLE_H].cursor=CURSOR_H]);

                if(h[HANDLE_V].cursor != CURSOR_V)
                    gdk_window_set_cursor(h[HANDLE_V].window, paned->priv->curs[h[HANDLE_V].cursor=CURSOR_V]);
            }
        }
    }else
    /* widget: reset cursors */
    {
        if(h[HANDLE_H].cursor != CURSOR_H)
            gdk_window_set_cursor(h[HANDLE_H].window, paned->priv->curs[h[HANDLE_H].cursor=CURSOR_H]);

        if(h[HANDLE_V].cursor != CURSOR_V)
            gdk_window_set_cursor(h[HANDLE_V].window, paned->priv->curs[h[HANDLE_V].cursor=CURSOR_V]);
    }

    return FALSE;
}


static gboolean
gtk_rtg_four_paned_button_release(GtkWidget *widget, GdkEventButton *event)
{
    GtkRtgFourPaned *paned = GTK_RTG_FOUR_PANED(widget);

    if(paned->priv->macro_pane)
        return FALSE;

    Handle *h = paned->priv->h;

    if(h[HANDLE_H].drag || h[HANDLE_V].drag)
    {
        h[HANDLE_H].drag = FALSE;
        h[HANDLE_V].drag = FALSE;
        gtk_widget_queue_draw(widget);
        gtk_widget_queue_resize(widget);
    }

    return FALSE;
}


static void
render_handle(GtkWidget *widget, cairo_t *cr, const GdkRectangle &rectangle, gboolean horizontal)
{
    GtkAllocation allocation = widget_allocation(widget);
    const gint x = rectangle.x - allocation.x;
    const gint y = rectangle.y - allocation.y;
    GtkStyleContext *style = gtk_widget_get_style_context(widget);
    gtk_style_context_save(style);
    gtk_style_context_add_class(style, GTK_STYLE_CLASS_PANE_SEPARATOR);
    gtk_render_handle(style, cr, x, y, rectangle.width, rectangle.height);
    gtk_style_context_restore(style);

    if((horizontal ? rectangle.width : rectangle.height) > DRAW_HANDLE_ABOVE)
    {
        cairo_set_source_rgb(cr, .65, .65, .65);
        for(gint i = -1; i <= 1; ++i)
        {
            const gint cx = x + rectangle.width / 2 + (horizontal ? i * 4 : 0);
            const gint cy = y + rectangle.height / 2 + (horizontal ? 0 : i * 4);
            cairo_rectangle(cr, cx, cy, 1, 1);
        }
        cairo_fill(cr);
    }
}

static gboolean
gtk_rtg_four_paned_expose(GtkWidget *widget, cairo_t *cr)
{
    GtkRtgFourPaned *paned = GTK_RTG_FOUR_PANED(widget);
    Handle *h = paned->priv->h;
    GtkAllocation allocation = widget_allocation(widget);

    if(gtk_cairo_should_draw_window(cr, gtk_widget_get_window(widget)))
    {
        gtk_render_background(gtk_widget_get_style_context(widget), cr, 0, 0, allocation.width, allocation.height);
        GTK_WIDGET_CLASS(gtk_rtg_four_paned_parent_class)->draw(widget, cr);
        for(guint i = 0; i < HANDLES; ++i)
            if(h[i].visible)
                render_handle(widget, cr, h[i].pos, i == HANDLE_H);
    }
    return FALSE;
}

static void
gtk_rtg_four_paned_preferred_width(GtkWidget *widget, gint *minimum, gint *natural)
{
    GtkRequisition size;
    gtk_rtg_four_paned_size_request(widget, &size);
    *minimum = *natural = MAX(size.width, 1);
}

static void
gtk_rtg_four_paned_preferred_height(GtkWidget *widget, gint *minimum, gint *natural)
{
    GtkRequisition size;
    gtk_rtg_four_paned_size_request(widget, &size);
    *minimum = *natural = MAX(size.height, 1);
}

static void
gtk_rtg_four_paned_map(GtkWidget *widget)
{
    GTK_WIDGET_CLASS(gtk_rtg_four_paned_parent_class)->map(widget);
    GtkRtgFourPaned *paned = GTK_RTG_FOUR_PANED(widget);
    update_handles_visibility(paned->priv->h, paned->priv->p);
}

static void
gtk_rtg_four_paned_unmap(GtkWidget *widget)
{
    Handle *h = GTK_RTG_FOUR_PANED(widget)->priv->h;
    for(guint i = 0; i < HANDLES; ++i)
    {
        if(h[i].window) gdk_window_hide(h[i].window);
    }
    GTK_WIDGET_CLASS(gtk_rtg_four_paned_parent_class)->unmap(widget);
}

static void
gtk_rtg_four_paned_unrealize(GtkWidget *widget)
{
    Handle *h = GTK_RTG_FOUR_PANED(widget)->priv->h;
    for(guint i = 0; i < HANDLES; ++i)
    {
        if(h[i].window)
        {
            gtk_widget_unregister_window(widget, h[i].window);
            gdk_window_destroy(h[i].window);
            h[i].window = NULL;
        }
    }
    GTK_WIDGET_CLASS(gtk_rtg_four_paned_parent_class)->unrealize(widget);
}


static void
gtk_rtg_four_paned_size_request(GtkWidget *widget, GtkRequisition *requisition)
{
    GtkRtgFourPaned *paned = GTK_RTG_FOUR_PANED(widget);
    GtkRequisition child_requisition;

    requisition->width = 0;
    requisition->height = 0;

    Pane *p = paned->priv->p;
    Handle *h = paned->priv->h;


    /*
     *                  HANDLE_V
     *                     ^
     *                     |
     *              ---------------
     *             |       |       |
     *             | PANE1 | PANE2 |
     *             |       |       |
     *              ------------------> HANDLE_H
     *             |       |       |
     *             | PANE3 | PANE4 |
     *             |       |       |
     *              ---------------
     *              GtkRtgFourPaned
     */

    if(IS_PANE(p[PANE_0].widget))
    {
        gtk_widget_get_preferred_size(p[PANE_0].widget, &child_requisition, NULL);

        if(child_requisition.width > 0 && child_requisition.height > 0)
        {
            requisition->height = child_requisition.height;
            requisition->width = child_requisition.width;
        }
    }

    if(IS_PANE(p[PANE_1].widget))
    {
        gtk_widget_get_preferred_size(p[PANE_1].widget, &child_requisition, NULL);

        if(child_requisition.width > 0 && child_requisition.height > 0)
        {
            /* snap to higher */
            if(child_requisition.height > requisition->height )
                requisition->height = child_requisition.height;

            requisition->width += child_requisition.width;

            if(IS_PANE(p[PANE_0].widget))
                requisition->width += h[HANDLE_V].pos.width;
        }

    }

    if(IS_PANE(p[PANE_2].widget))
    {
        gtk_widget_get_preferred_size(p[PANE_2].widget, &child_requisition, NULL);

        if(child_requisition.width > 0 && child_requisition.height > 0)
        {
            /* only pane0 */
            if(IS_PANE(p[PANE_0].widget) && NOT_PANE(p[PANE_1].widget))
            {
                /* snap to wider */
                if(child_requisition.width > requisition->width)
                    requisition->width = child_requisition.width;
            }else
            /* only pane1 */
            if(IS_PANE(p[PANE_1].widget) && NOT_PANE(p[PANE_0].widget))
            {
                requisition->width += child_requisition.width;
                requisition->width += h[HANDLE_V].pos.width;
            }else
                /* only pane2 */
                if(NOT_PANE(p[PANE_0].widget) && NOT_PANE(p[PANE_1].widget))
                    requisition->width = child_requisition.width;

            /* pane0 or pane1 */
            if(IS_PANE(p[PANE_0].widget) || IS_PANE(p[PANE_1].widget))
                requisition->height += h[HANDLE_H].pos.height;

            requisition->height += child_requisition.height;
        }
    }

    if(IS_PANE(p[PANE_3].widget))
    {
        gtk_widget_get_preferred_size(p[PANE_3].widget, &child_requisition, NULL);

        if(child_requisition.width > 0 && child_requisition.height > 0)
        {
            if(IS_PANE(p[PANE_2].widget))
            {
                if(NOT_PANE(p[PANE_1].widget))
                {
                    requisition->width += child_requisition.width;
                    requisition->width += h[HANDLE_V].pos.width;
                }

            }else
            /* no pane2 */
            {
                /* only pane0 */
                if(IS_PANE(p[PANE_0].widget) && NOT_PANE(p[PANE_1].widget))
                {
                    requisition->height += h[HANDLE_H].pos.height;
                    requisition->width  += child_requisition.width;
                    requisition->width  += h[HANDLE_V].pos.width;
                }else
                /* only pane1 */
                if(IS_PANE(p[PANE_1].widget) && NOT_PANE(p[PANE_0].widget))
                {
                    requisition->height += h[HANDLE_H].pos.height;
                    if(child_requisition.width > requisition->width)
                        requisition->width = child_requisition.width;
                }

                requisition->height += child_requisition.height;
            }
        }
    }
}


static void
update_panes_allocation(GtkRtgFourPaned *paned, GtkAllocation *alloc)
{
    /*
     *                  HANDLE_V
     *                     ^
     *                     |
     *              ---------------
     *             |       |       |
     *             | PANE0 | PANE1 |
     *             |       |       |
     *              ------------------> HANDLE_H
     *             |       |       |
     *             | PANE2 | PANE3 |
     *             |       |       |
     *              ---------------
     *              GtkRtgFourPaned
     */

    Pane *p = paned->priv->p;
    Handle *h = paned->priv->h;
    GtkWidget *widget = GTK_WIDGET(paned);

    gboolean &init = paned->priv->layout_initialized;
    if(!init)
    {
        h[HANDLE_H].drag_coord = gfloat(widget_allocation(widget).y) + gfloat(widget_allocation(widget).height) * 0.5f - gfloat(h[HANDLE_H].pos.height >> 1);
        h[HANDLE_V].drag_coord = gfloat(widget_allocation(widget).x) + gfloat(widget_allocation(widget).width)  * 0.5f - gfloat(h[HANDLE_V].pos.width  >> 1);
        init = TRUE;
    }

    /* update handles length */
    h[HANDLE_H].pos.width = alloc->width;
    h[HANDLE_V].pos.height = alloc->height;

    /* adapt uniform position to the new allocation (vertical handle) */
    if(paned->priv->macro_pane || h[HANDLE_V].visible)
    {
        /* unpack */
        gfloat drag_pos = h[HANDLE_V].drag_coord - gfloat(widget_allocation(widget).x) + gfloat(h[HANDLE_V].pos.width  >> 1);
        drag_pos /= gfloat(widget_allocation(widget).width);

        h[HANDLE_V].drag_coord = gfloat(alloc->x) + gfloat(alloc->width) * drag_pos - gfloat(h[HANDLE_V].pos.width  >> 1);
    }

    /* update position (only if visible) */
    if(h[HANDLE_V].visible)
    {
        h[HANDLE_V].pos.x = gint(h[HANDLE_V].drag_coord);
        h[HANDLE_V].pos.y = alloc->y;
    }

    /* adapt uniform position to the new allocation (horizontal handle) */
    if(paned->priv->macro_pane || h[HANDLE_H].visible)
    {
        /* unpack */
        gfloat drag_pos = h[HANDLE_H].drag_coord - gfloat(widget_allocation(widget).y) + gfloat(h[HANDLE_H].pos.height >> 1);
        drag_pos /= gfloat(widget_allocation(widget).height);

        h[HANDLE_H].drag_coord = gfloat(alloc->y) + gfloat(alloc->height) * drag_pos - gfloat(h[HANDLE_H].pos.height >> 1);
    }

    /* update position (only if visible) */
    if(h[HANDLE_H].visible)
    {
        h[HANDLE_H].pos.x = alloc->x;
        h[HANDLE_H].pos.y = gint(h[HANDLE_H].drag_coord);
    }

    /*
     * Update panes allocation which relates to handle position and widget size.
     */

    GtkRequisition child_requisition;
    GtkAllocation child_allocation;
    gint border_width = /*GTK_CONTAINER(paned)->border_width*/ 0; /* not supported yet */

    /* PANE 0 */
    if(IS_PANE(p[PANE_0].widget))
    {
        gtk_widget_get_preferred_size(p[PANE_0].widget, &child_requisition, NULL);

        if(p[PANE_0].resize)	/* resize */
        {
            child_allocation.x = alloc->x + border_width;
            child_allocation.y = alloc->y + border_width;
            child_allocation.width = h[HANDLE_V].visible ? h[HANDLE_V].pos.x - alloc->x : alloc->width  - border_width;
            child_allocation.height = h[HANDLE_H].visible ? h[HANDLE_H].pos.y - alloc->y : alloc->height - border_width;

            if(!p[PANE_0].shrink)	/* no shrink */
            {
                /*
                 * Snap to requested size: - update handles positions.
                 *
                 */

                if(child_allocation.width < child_requisition.width)
                {
                    child_allocation.width = child_requisition.width;

                    /* snap to requested width */
                    if(h[HANDLE_V].visible)
                        h[HANDLE_V].pos.x = alloc->x + child_requisition.width;

                }

                if(child_allocation.height < child_requisition.height)
                {
                    child_allocation.height = child_requisition.height;

                    /* snap to requested height */
                    if(h[HANDLE_H].visible)
                        h[HANDLE_H].pos.y = alloc->y + child_requisition.height;
                }
            }
        }else
        /* no resize allowed, stick to requested dimension */
        {
            child_allocation.x = alloc->x + border_width;
            child_allocation.y = alloc->y + border_width;
            child_allocation.width = child_requisition.width;
            child_allocation.height = child_requisition.height;
        }

        child_allocation.width = MAX(child_allocation.width, 1);
        child_allocation.height = MAX(child_allocation.height, 1);
        gtk_widget_size_allocate(p[PANE_0].widget, &child_allocation);
    }

    /* PANE 1 */
    if(IS_PANE(p[PANE_1].widget))
    {
        gtk_widget_get_preferred_size(p[PANE_1].widget, &child_requisition, NULL);

        if(p[PANE_1].resize)	/* resize */
        {
            child_allocation.x = h[HANDLE_V].visible ? h[HANDLE_V].pos.x + h[HANDLE_V].pos.width : alloc->x + border_width;
            child_allocation.y = alloc->y + border_width;
            child_allocation.width = h[HANDLE_V].visible ? alloc->width  - child_allocation.x + alloc->x : alloc->width  - border_width;
            child_allocation.height = h[HANDLE_H].visible ? h[HANDLE_H].pos.y - alloc->y : alloc->height - border_width;

            if(!p[PANE_1].shrink)	/* no shrink */
            {
                /*
                 * Snap to requested size: - update handle positions.
                 *
                 */

                if(child_allocation.width < child_requisition.width)
                    child_allocation.width = child_requisition.width;

                if(child_allocation.height < child_requisition.height)
                {
                    child_allocation.height = child_requisition.height;

                    /* snap to requested height */
                    if(h[HANDLE_H].visible)
                        h[HANDLE_H].pos.y = alloc->y + child_requisition.height;
                }
            }

        }else
        /* no resize allowed, stick to requested dimension */
        {
            child_allocation.x = h[HANDLE_V].visible ? h[HANDLE_V].pos.x + h[HANDLE_V].pos.width : alloc->x + border_width;
            child_allocation.y = alloc->y + border_width;
            child_allocation.width = child_requisition.width;
            child_allocation.height = child_requisition.height;
        }

        child_allocation.width = MAX(child_allocation.width, 1);
        child_allocation.height = MAX(child_allocation.height, 1);
        gtk_widget_size_allocate(p[PANE_1].widget, &child_allocation);
    }

    /* PANE 2 */
    if(IS_PANE(p[PANE_2].widget))
    {
        gtk_widget_get_preferred_size(p[PANE_2].widget, &child_requisition, NULL);

        if(p[PANE_2].resize) /* resize */
        {
            child_allocation.x = alloc->x + border_width;
            child_allocation.y = h[HANDLE_H].visible ? h[HANDLE_H].pos.y + h[HANDLE_H].pos.height : alloc->y + border_width;
            child_allocation.width = h[HANDLE_V].visible ? h[HANDLE_V].pos.x - alloc->x : alloc->width  - border_width;
            child_allocation.height = h[HANDLE_H].visible ? alloc->height - child_allocation.y + alloc->y : alloc->height - border_width;

            if(!p[PANE_2].shrink) /* no shrink */
            {
                /*
                 * Snap to requested size: - update handle positions.
                 *
                 */

                if(child_allocation.width < child_requisition.width)
                {
                    child_allocation.width = child_requisition.width;

                    /* snap to requested width */
                    if(h[HANDLE_V].visible)
                        h[HANDLE_V].pos.x = alloc->x + child_requisition.width;

                }

                if(child_allocation.height < child_requisition.height)
                    child_allocation.height = child_requisition.height;
            }
        }else
        /* no resize allowed, stick to requested dimension */
        {
            child_allocation.x = alloc->x + border_width;
            child_allocation.y = h[HANDLE_H].visible ? h[HANDLE_H].pos.y + h[HANDLE_H].pos.height : alloc->y + border_width;
            child_allocation.width = child_requisition.width;
            child_allocation.height = child_requisition.height;
        }

        child_allocation.width = MAX(child_allocation.width, 1);
        child_allocation.height = MAX(child_allocation.height, 1);
        gtk_widget_size_allocate(p[PANE_2].widget, &child_allocation);
    }

    /* PANE 3 */
    if(IS_PANE(p[PANE_3].widget))
    {
        gtk_widget_get_preferred_size(p[PANE_3].widget, &child_requisition, NULL);

        if(p[PANE_3].resize)	/* resize */
        {
            child_allocation.x = h[HANDLE_V].visible ? h[HANDLE_V].pos.x + h[HANDLE_V].pos.width : alloc->x + border_width;
            child_allocation.y = h[HANDLE_H].visible ? h[HANDLE_H].pos.y + h[HANDLE_H].pos.height : alloc->y + border_width;
            child_allocation.width = h[HANDLE_V].visible ? alloc->width  - child_allocation.x + alloc->x : alloc->width - border_width;
            child_allocation.height = h[HANDLE_H].visible ? alloc->height - child_allocation.y + alloc->y : alloc->height - border_width;

            if(!p[PANE_3].shrink)	/* no shrink */
            {
                /*
                 * Snap to requested size
                 *
                 */

                if(child_allocation.width < child_requisition.width)
                    child_allocation.width = child_requisition.width;

                if(child_allocation.height < child_requisition.height)
                    child_allocation.height = child_requisition.height;
            }
        }else
        /* no resize allowed, stick to requested dimension */
        {
            child_allocation.x = h[HANDLE_V].visible ? h[HANDLE_V].pos.x + h[HANDLE_V].pos.width  : alloc->x + border_width;
            child_allocation.y = h[HANDLE_H].visible ? h[HANDLE_H].pos.y + h[HANDLE_H].pos.height : alloc->y + border_width;
            child_allocation.width = child_requisition.width;
            child_allocation.height = child_requisition.height;
        }

        child_allocation.width = MAX(child_allocation.width, 1);
        child_allocation.height = MAX(child_allocation.height, 1);
        gtk_widget_size_allocate(p[PANE_3].widget, &child_allocation);
    }
}


static void
gtk_rtg_four_paned_size_allocate(GtkWidget *widget, GtkAllocation *allocation)
{
    GtkRtgFourPaned *paned = GTK_RTG_FOUR_PANED(widget);
    Handle *h = paned->priv->h;
    update_handles_visibility(h, paned->priv->p);
    update_panes_allocation(paned, allocation);
    gtk_widget_set_allocation(widget, allocation);
    if(gtk_widget_get_realized(widget))
        for(guint i = 0; i < HANDLES; ++i)
            if(h[i].visible)
                gdk_window_move_resize(h[i].window, h[i].pos.x, h[i].pos.y,
                                      MAX(h[i].pos.width, 1), MAX(h[i].pos.height, 1));
}


static void
gtk_rtg_four_paned_add(GtkContainer *container, GtkWidget *widget)
{
    g_throw_if_fail(GTK_IS_CONTAINER(container));
    g_throw_if_fail(GTK_IS_WIDGET(widget));

    GtkRtgFourPaned *paned = GTK_RTG_FOUR_PANED(container);
    Pane *p = paned->priv->p;

    if(!p[PANE_0].widget)
        gtk_rtg_four_paned_pack1(paned, widget);
    else
    if(!p[PANE_1].widget)
        gtk_rtg_four_paned_pack2(paned, widget);
    else
    if(!p[PANE_2].widget)
        gtk_rtg_four_paned_pack3(paned, widget);
    else
    if(!p[PANE_3].widget)
        gtk_rtg_four_paned_pack4(paned, widget);
    else
        g_warning ("GtkFourPaned cannot have more than 4 children\n");
}


static void
gtk_rtg_four_paned_remove(GtkContainer *container, GtkWidget *widget)
{
    g_throw_if_fail(GTK_IS_CONTAINER(container));
    g_throw_if_fail(GTK_IS_WIDGET(widget));

    GtkRtgFourPaned *paned = GTK_RTG_FOUR_PANED(container);
    gboolean was_visible = gtk_widget_get_visible(GTK_WIDGET(widget));

    Pane *p = paned->priv->p;

    for(guint i=0; i<PANES; ++i)

        if(p[i].widget == widget)
        {
            if(GTK_RTG_IS_CANVAS(widget))
            {
                g_signal_handlers_disconnect_by_func(widget, (void *)gtk_rtg_canvas_focused, paned);
                g_signal_handlers_disconnect_by_func(widget, (void *)gtk_rtg_canvas_gizmo_position, paned);
            }

            gtk_widget_unparent(widget);

            p[i].widget = NULL;

            if(was_visible && gtk_widget_get_visible(GTK_WIDGET(container)))
                gtk_widget_queue_resize(GTK_WIDGET(container));

            break;
        }
}


static void
gtk_rtg_four_paned_forall(GtkContainer *container,
                          gboolean include_internals,
                          GtkCallback callback,
                          gpointer	callback_data)
{
    g_throw_if_fail(GTK_IS_CONTAINER(container));
    g_throw_if_fail(callback != NULL);

    GtkRtgFourPaned *paned = GTK_RTG_FOUR_PANED(container);

    Pane *p = paned->priv->p;

    if(p[PANE_0].widget) (*callback)(p[PANE_0].widget, callback_data);
    if(p[PANE_1].widget) (*callback)(p[PANE_1].widget, callback_data);
    if(p[PANE_2].widget) (*callback)(p[PANE_2].widget, callback_data);
    if(p[PANE_3].widget) (*callback)(p[PANE_3].widget, callback_data);
}


inline static void
focus_widget(GtkWidget *widget)
{
    GTK_RTG_IS_CANVAS(widget) ? gtk_rtg_canvas_set_focused(GTK_RTG_CANVAS(widget), TRUE)
                              : gtk_widget_grab_focus(widget);
}


static gboolean
gtk_rtg_four_paned_toggle_handle_focus(GtkRtgFourPaned *four_paned)
{
    if(four_paned->priv->macro_pane)
        return FALSE;

    Pane *p = four_paned->priv->p;

    for(guint i=PANE_0; i<PANES; ++i)

        if(p[i].widget)
        {
            /* canvas */
            if(GTK_RTG_IS_CANVAS(p[i].widget))
            {
                if(gtk_rtg_canvas_is_focused(GTK_RTG_CANVAS(p[i].widget)))
                    goto focus;
            }else
            /* general widget */
            if(gtk_widget_has_focus(GTK_WIDGET(p[i].widget)))
            {
                focus:

                guint j = (i+1)%PANES;
                guint k = (i+2)%PANES;
                guint l = (i+3)%PANES;

                if(IS_PANE(p[j].widget)) focus_widget(p[j].widget);
                else
                if(IS_PANE(p[k].widget)) focus_widget(p[k].widget);
                else
                if(IS_PANE(p[l].widget)) focus_widget(p[l].widget);

                break;
            }
        }

    return TRUE;
}

static void
gtk_rtg_four_paned_grab_focus(GtkWidget *widget)
{
    GtkRtgFourPaned *four_paned = GTK_RTG_FOUR_PANED(widget);
    Pane *p = four_paned->priv->p;

    GTK_WIDGET_CLASS(gtk_rtg_four_paned_parent_class)->grab_focus(widget);

    /* re-focus canvas */
    for(guint i=0; i<PANES; ++i)
        if(   p[i].widget
           && GTK_RTG_IS_CANVAS(p[i].widget)
           && gtk_rtg_canvas_is_focused(GTK_RTG_CANVAS(p[i].widget)))
        {
            gtk_widget_grab_focus(p[i].widget);
            break;
        }
}


static void
gtk_rtg_canvas_focused(GtkRtgCanvas *canvas, gpointer data)
{
    GtkRtgFourPaned *four_paned = GTK_RTG_FOUR_PANED(data);
    Pane *p = four_paned->priv->p;
    GtkWidget *widget = GTK_WIDGET(canvas);

    /* defocus the rest */
    for(guint i=0; i<PANES; ++i)
        if(p[i].widget && p[i].widget != widget && GTK_RTG_IS_CANVAS(p[i].widget))
            gtk_rtg_canvas_set_focused(GTK_RTG_CANVAS(p[i].widget), FALSE);
}


static void
gtk_rtg_canvas_gizmo_position(GtkRtgCanvas *canvas, gtkrtg::point3f *pos, gpointer data)
{
    GtkRtgFourPaned *four_paned = GTK_RTG_FOUR_PANED(data);
    Pane *p = four_paned->priv->p;
    GtkWidget *widget = GTK_WIDGET(canvas);

    /* update gizmos */
    for(guint i=0; i<PANES; ++i)
        if(p[i].widget && p[i].widget != widget && GTK_RTG_IS_CANVAS(p[i].widget))
        {
            GtkRtgCanvas *c = GTK_RTG_CANVAS(p[i].widget);
            gtk_rtg_canvas_gizmo_update_position(c, pos);
            gtk_rtg_canvas_render(c);
        }
}



/*
 * Public
 *
 *
 */

void
gtk_rtg_four_paned_pack1(GtkRtgFourPaned *paned, GtkWidget *child, gboolean resize, gboolean shrink) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_FOUR_PANED(paned));
    g_throw_if_fail(GTK_IS_WIDGET(child));

    if(paned->priv->p[PANE_0].widget)
    {
        gtk_container_remove(GTK_CONTAINER(paned), paned->priv->p[PANE_0].widget);
    }

    paned->priv->p[PANE_0].widget = child;
    paned->priv->p[PANE_0].resize = resize;
    paned->priv->p[PANE_0].shrink = shrink;

    gtk_widget_set_parent(child, GTK_WIDGET(paned));

    if(gtk_widget_get_realized(GTK_WIDGET(GTK_WIDGET(paned))))
    {
        update_handles_visibility(paned->priv->h, paned->priv->p);
        gtk_widget_queue_resize(GTK_WIDGET(paned));
    }

    if(GTK_RTG_IS_CANVAS(child))
    {
        g_signal_connect(G_OBJECT(child), "focused",	    G_CALLBACK(gtk_rtg_canvas_focused), paned);
        g_signal_connect(G_OBJECT(child), "gizmo-position", G_CALLBACK(gtk_rtg_canvas_gizmo_position), paned);
    }
}


void
gtk_rtg_four_paned_pack2(GtkRtgFourPaned *paned, GtkWidget *child, gboolean resize, gboolean shrink) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_FOUR_PANED(paned));
    g_throw_if_fail(GTK_IS_WIDGET(child));

    if(paned->priv->p[PANE_1].widget)
    {
        gtk_container_remove(GTK_CONTAINER(paned), paned->priv->p[PANE_1].widget);
    }

    paned->priv->p[PANE_1].widget = child;
    paned->priv->p[PANE_1].resize = resize;
    paned->priv->p[PANE_1].shrink = shrink;

    gtk_widget_set_parent(child, GTK_WIDGET(paned));

    if(gtk_widget_get_realized(GTK_WIDGET(GTK_WIDGET(paned))))
    {
        update_handles_visibility(paned->priv->h, paned->priv->p);
        gtk_widget_queue_resize(GTK_WIDGET(paned));
    }

    if(GTK_RTG_IS_CANVAS(child))
    {
        g_signal_connect(G_OBJECT(child), "focused", G_CALLBACK(gtk_rtg_canvas_focused), paned);
        g_signal_connect(G_OBJECT(child), "gizmo-position", G_CALLBACK(gtk_rtg_canvas_gizmo_position), paned);
    }
}


void
gtk_rtg_four_paned_pack3(GtkRtgFourPaned *paned, GtkWidget *child, gboolean resize, gboolean shrink) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_FOUR_PANED(paned));
    g_throw_if_fail(GTK_IS_WIDGET(child));

    if(paned->priv->p[PANE_2].widget)
    {
        gtk_container_remove(GTK_CONTAINER(paned), paned->priv->p[PANE_2].widget);
    }

    paned->priv->p[PANE_2].widget = child;
    paned->priv->p[PANE_2].resize = resize;
    paned->priv->p[PANE_2].shrink = shrink;

    gtk_widget_set_parent(child, GTK_WIDGET(paned));

    if(gtk_widget_get_realized(GTK_WIDGET(GTK_WIDGET(paned))))
    {
        update_handles_visibility(paned->priv->h, paned->priv->p);
        gtk_widget_queue_resize(GTK_WIDGET(paned));
    }

    if(GTK_RTG_IS_CANVAS(child))
    {
        g_signal_connect(G_OBJECT(child), "focused", G_CALLBACK(gtk_rtg_canvas_focused), paned);
        g_signal_connect(G_OBJECT(child), "gizmo-position", G_CALLBACK(gtk_rtg_canvas_gizmo_position), paned);
    }
}


void
gtk_rtg_four_paned_pack4(GtkRtgFourPaned *paned, GtkWidget *child, gboolean resize, gboolean shrink) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_FOUR_PANED(paned));
    g_throw_if_fail(GTK_IS_WIDGET(child));

    if(paned->priv->p[PANE_3].widget)
    {
        gtk_container_remove(GTK_CONTAINER(paned), paned->priv->p[PANE_3].widget);
    }

    paned->priv->p[PANE_3].widget = child;
    paned->priv->p[PANE_3].resize = resize;
    paned->priv->p[PANE_3].shrink = shrink;

    gtk_widget_set_parent(child, GTK_WIDGET(paned));

    if(gtk_widget_get_realized(GTK_WIDGET(GTK_WIDGET(paned))))
    {
        update_handles_visibility(paned->priv->h, paned->priv->p);
        gtk_widget_queue_resize(GTK_WIDGET(paned));
    }

    if(GTK_RTG_IS_CANVAS(child))
    {
        g_signal_connect(G_OBJECT(child), "focused", G_CALLBACK(gtk_rtg_canvas_focused), paned);
        g_signal_connect(G_OBJECT(child), "gizmo-position", G_CALLBACK(gtk_rtg_canvas_gizmo_position), paned);
    }
}


void
gtk_rtg_four_paned_macro_pane(GtkRtgFourPaned *paned, GtkWidget *child) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_FOUR_PANED(paned));
    g_throw_if_fail(paned->priv->macro_pane == FALSE);
    g_throw_if_fail(GTK_IS_WIDGET(child))
    g_throw_if_fail(GTK_WIDGET(paned) == gtk_widget_get_parent(child));  /* integrity check */

    Pane *p = paned->priv->p;
    Handle *h = paned->priv->h;

    for(guint i = 0; i < PANES; ++i)
        if(p[i].widget)
        {
            paned->priv->saved_visible[i] = gtk_widget_get_visible(p[i].widget);
            gtk_widget_set_visible(p[i].widget, p[i].widget == child);
        }

    if(h[HANDLE_H].visible)
    {
        gdk_window_hide(h[HANDLE_H].window);
        h[HANDLE_H].visible = 0;
    }

    if(h[HANDLE_V].visible)
    {
        gdk_window_hide(h[HANDLE_V].window);
        h[HANDLE_V].visible = 0;
    }

    update_handles_visibility(h, p);
    gtk_widget_queue_resize(GTK_WIDGET(paned));

    paned->priv->macro_pane = TRUE;
}


gboolean
gtk_rtg_four_paned_is_macro_pane(GtkRtgFourPaned *paned) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_FOUR_PANED(paned));
    return paned->priv->macro_pane;
}


void
gtk_rtg_four_paned_restore_panes(GtkRtgFourPaned *paned) throw (GtkRtgException)
{
    g_throw_if_fail(GTK_RTG_IS_FOUR_PANED(paned));

    Pane *p = paned->priv->p;
    Handle *h = paned->priv->h;

    if(!paned->priv->macro_pane)
        return;
    for(guint i = 0; i < PANES; ++i)
        if(p[i].widget)
            gtk_widget_set_visible(p[i].widget, paned->priv->saved_visible[i]);

    update_handles_visibility(h, p);
    gtk_widget_queue_resize(GTK_WIDGET(paned));

    paned->priv->macro_pane = FALSE;
}
