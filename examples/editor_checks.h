#include "../src/gtkrtgextras.h"
#include "../src/gtkrtgcolorid.h"

/* Integration checks use the same GTK event handlers as pointer interaction. */
static void
settle_ui()
{
    const gint64 until = g_get_monotonic_time() + 60000;
    do
    {
        while(gtk_events_pending())
            gtk_main_iteration_do(FALSE);
        g_usleep(1000);
    } while(g_get_monotonic_time() < until);
}

static void
send_motion(GtkWidget *widget, GdkWindow *window, double x, double y, guint state = 0)
{
    GdkEvent *event = gdk_event_new(GDK_MOTION_NOTIFY);
    event->motion.window = GDK_WINDOW(g_object_ref(window));
    event->motion.x = x;
    event->motion.y = y;
    event->motion.state = state;
    gboolean handled = FALSE;
    g_signal_emit_by_name(widget, "motion-notify-event", event, &handled);
    gdk_event_free(event);
}

static void
send_button(GtkWidget *widget, GdkWindow *window, double x, double y, GdkEventType type, guint button = 1)
{
    GdkEvent *event = gdk_event_new(type);
    event->button.window = GDK_WINDOW(g_object_ref(window));
    event->button.x = x;
    event->button.y = y;
    event->button.button = button;
    gboolean handled = FALSE;
    g_signal_emit_by_name(widget, type == GDK_BUTTON_PRESS ? "button-press-event" : "button-release-event",
                          event, &handled);
    gdk_event_free(event);
}

static void
drag_canvas(GtkRtgCanvas *view, GdkPoint start, int dx, int dy, guint button = 1)
{
    GtkWidget *widget = GTK_WIDGET(view);
    GdkWindow *window = gtk_widget_get_window(widget);
    send_motion(widget, window, start.x, start.y);
    settle_ui();
    send_button(widget, window, start.x, start.y, GDK_BUTTON_PRESS, button);
    settle_ui();
    const guint mask = button == 1 ? GDK_BUTTON1_MASK : button == 2 ? GDK_BUTTON2_MASK : GDK_BUTTON3_MASK;
    for(int i = 1; i <= 4; ++i)
    {
        send_motion(widget, window, start.x + dx * i / 4, start.y + dy * i / 4, mask);
        settle_ui();
    }
    send_button(widget, window, start.x + dx, start.y + dy, GDK_BUTTON_RELEASE, button);
    settle_ui();
}

static GtkWidget *
find_button(GtkWidget *widget, const char *label)
{
    if(GTK_IS_TOOL_BUTTON(widget) && g_strcmp0(gtk_tool_button_get_label(GTK_TOOL_BUTTON(widget)), label) == 0)
        return widget;
    if(GTK_IS_BUTTON(widget) && g_strcmp0(gtk_button_get_label(GTK_BUTTON(widget)), label) == 0)
        return widget;
    if(GTK_IS_CONTAINER(widget))
    {
        GList *children = gtk_container_get_children(GTK_CONTAINER(widget));
        GtkWidget *found = nullptr;
        for(GList *child = children; child && !found; child = child->next)
            found = find_button(GTK_WIDGET(child->data), label);
        g_list_free(children);
        return found;
    }
    return nullptr;
}

static void
choose_tool(Editor *editor, const char *label)
{
    GtkWidget *button = find_button(editor->toolbar, label);
    g_assert_nonnull(button);
    if(!GTK_IS_TOGGLE_BUTTON(button) || !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)))
        gtk_button_clicked(GTK_BUTTON(button));
    settle_ui();
    if(GTK_IS_TOGGLE_BUTTON(button))
        g_assert_true(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(button)));
}

static GdkPoint
sample_hit(Editor *editor, int view)
{
    GtkRtgCanvas *canvas = editor->views[view];
    g_assert_true(gtk_rtg_canvas_make_current(canvas));
    const int width = gtk_widget_get_allocated_width(GTK_WIDGET(canvas));
    const int height = gtk_widget_get_allocated_height(GTK_WIDGET(canvas));
    for(int y = height / 4; y < height * 3 / 4; y += 7)
        for(int x = width / 4; x < width * 3 / 4; x += 7)
        {
            GdkPoint point = {x, height - y};
            if(gtk_rtg_scene_point_select(editor->scene, point, canvas, TRUE) ==
               int(COLOR_ID(editor->sample)))
                return {x, y};
        }
    g_error("Sample mesh was not pickable in viewport %d", view);
    return {};
}

static GdkPoint
axis_hit(Editor *editor)
{
    GtkRtgCanvas *canvas = editor->views[3];
    g_assert_true(gtk_rtg_canvas_make_current(canvas));
    GdkPoint point = {2, 2};
    gtk_rtg_scene_point_select(editor->scene, point, canvas, TRUE);
    auto *buffer = static_cast<GtkRtgPickingBuffer *>(g_object_get_data(G_OBJECT(canvas), "pickbuff"));
    for(int y = 0; y < buffer->height; ++y)
        for(int x = 0; x < buffer->width; ++x)
        {
            const auto *pixel = buffer->buff + (y * buffer->width + x) * 4;
            const guint id = pixel[0] | (pixel[1] << 8) | (pixel[2] << 16);
            if(id == COLOR_ID_GIZMO_AX_X)
                return {x, buffer->height - y - 1};
        }
    g_error("Scale X handle is not pickable");
    return {};
}

static void
start_scroll(GtkRtgCanvas *canvas)
{
    GdkEvent *event = gdk_event_new(GDK_SCROLL);
    event->scroll.window = GDK_WINDOW(g_object_ref(gtk_widget_get_window(GTK_WIDGET(canvas))));
    event->scroll.direction = GDK_SCROLL_SMOOTH;
    event->scroll.delta_y = -1.;
    gboolean handled = FALSE;
    g_signal_emit_by_name(canvas, "scroll-event", event, &handled);
    gdk_event_free(event);
}

static gboolean
count_draw(GtkWidget *, cairo_t *, gpointer data)
{
    ++*static_cast<int *>(data);
    return FALSE;
}

static void
check_splitters(Editor *editor)
{
    GtkWidget *panes = editor->panes;
    GdkWindow *horizontal = nullptr, *vertical = nullptr;
    GList *windows = gdk_window_get_children(gtk_widget_get_window(panes));
    for(GList *node = windows; node; node = node->next)
    {
        auto *window = GDK_WINDOW(node->data);
        gpointer owner = nullptr;
        gdk_window_get_user_data(window, &owner);
        if(owner != panes || !gdk_window_is_visible(window))
            continue;
        if(gdk_window_get_width(window) > gdk_window_get_height(window))
            horizontal = window;
        else
            vertical = window;
    }
    g_list_free(windows);
    g_assert_nonnull(horizontal);
    g_assert_nonnull(vertical);
    int draws[4] = {};
    gulong handlers[4];
    for(int i = 0; i < 4; ++i)
        handlers[i] = g_signal_connect_after(editor->views[i], "draw", G_CALLBACK(count_draw), &draws[i]);
    auto check_redraw = [&]() {
        settle_ui();
        for(int &count : draws)
        {
            g_assert_cmpint(count, >, 0);
            count = 0;
        }
    };
    const int before_width = gtk_widget_get_allocated_width(GTK_WIDGET(editor->views[0]));
    const int before_height = gtk_widget_get_allocated_height(GTK_WIDGET(editor->views[0]));
    send_motion(panes, vertical, 2, 30);
    send_button(panes, vertical, 2, 30, GDK_BUTTON_PRESS);
    send_motion(panes, vertical, 22, 30, GDK_BUTTON1_MASK);
    check_redraw();
    g_assert_cmpint(gtk_widget_get_allocated_width(GTK_WIDGET(editor->views[0])), >, before_width + 10);
    send_motion(panes, vertical, 22, 30, GDK_BUTTON1_MASK);
    check_redraw();
    g_assert_cmpint(gtk_widget_get_allocated_width(GTK_WIDGET(editor->views[0])), >, before_width + 30);
    send_button(panes, vertical, 2, 30, GDK_BUTTON_RELEASE);
    settle_ui();
    send_motion(panes, horizontal, 30, 2);
    send_button(panes, horizontal, 30, 2, GDK_BUTTON_PRESS);
    send_motion(panes, horizontal, 30, 32, GDK_BUTTON1_MASK);
    check_redraw();
    g_assert_cmpint(gtk_widget_get_allocated_height(GTK_WIDGET(editor->views[0])), >, before_height + 15);
    send_button(panes, horizontal, 30, 2, GDK_BUTTON_RELEASE);
    settle_ui();

    gint hx, hy, vx, vy;
    gdk_window_get_position(horizontal, &hx, &hy);
    gdk_window_get_position(vertical, &vx, &vy);
    const int center = vx - hx + 2;
    send_motion(panes, horizontal, center, 2);
    send_button(panes, horizontal, center, 2, GDK_BUTTON_PRESS);
    send_motion(panes, horizontal, center - 25, -18, GDK_BUTTON1_MASK);
    check_redraw();
    g_assert_cmpint(gtk_widget_get_allocated_width(GTK_WIDGET(editor->views[0])), <, before_width + 35);
    g_assert_cmpint(gtk_widget_get_allocated_height(GTK_WIDGET(editor->views[0])), <, before_height + 25);
    if(!editor->screenshot.empty())
    {
        const std::string final_path = editor->screenshot;
        editor->screenshot += ".drag.png";
        g_assert_true(save_screenshot(editor));
        editor->screenshot = final_path;
    }
    send_button(panes, horizontal, center - 25, 2, GDK_BUTTON_RELEASE);
    settle_ui();
    for(int i = 0; i < 4; ++i)
        g_signal_handler_disconnect(editor->views[i], handlers[i]);

    gtk_rtg_canvas_set_focused(editor->views[3], TRUE);
    choose_tool(editor, "Maximize");
    g_assert_false(gtk_widget_get_visible(GTK_WIDGET(editor->views[0])));
    g_assert_cmpint(gtk_widget_get_allocated_width(GTK_WIDGET(editor->views[3])), >, 1000);
    choose_tool(editor, "Move");
    g_assert_true(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(find_button(editor->toolbar, "Maximize"))));
    gtk_button_clicked(GTK_BUTTON(find_button(editor->toolbar, "Maximize")));
    settle_ui();
    for(auto *view : editor->views)
        g_assert_true(gtk_widget_get_visible(GTK_WIDGET(view)));
    g_print("PASS: panes resize and repaint during horizontal, vertical and center drags; toolbar maximize/restore\n");

    /* Regression for the old function-static layout initialization. */
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    GtkWidget *second = gtk_rtg_four_paned_new();
    gtk_window_set_default_size(GTK_WINDOW(window), 640, 480);
    gtk_container_add(GTK_CONTAINER(window), second);
    GtkWidget *a = gtk_label_new("A"), *b = gtk_label_new("B");
    gtk_rtg_four_paned_pack1(GTK_RTG_FOUR_PANED(second), a);
    gtk_rtg_four_paned_pack4(GTK_RTG_FOUR_PANED(second), b);
    gtk_widget_show_all(window);
    settle_ui();
    g_assert_cmpint(gtk_widget_get_allocated_width(a), >, 200);
    g_assert_cmpint(gtk_widget_get_allocated_width(b), >, 200);
    gtk_widget_hide(b);
    gtk_rtg_four_paned_macro_pane(GTK_RTG_FOUR_PANED(second), a);
    gtk_rtg_four_paned_restore_panes(GTK_RTG_FOUR_PANED(second));
    g_assert_false(gtk_widget_get_visible(b));
    gtk_widget_destroy(window);
    g_print("PASS: independent and partially populated four-pane instances\n");
}

static void
capture_transform(GtkRtgPrimitive *, guint, gpointer data)
{
    glGetFloatv(GL_MODELVIEW_MATRIX, static_cast<float *>(data));
}

static matrix4x4f
rendered_transform(GtkRtgPrimitive *primitive)
{
    matrix4x4f matrix;
    const gulong handler =
        g_signal_connect(primitive, "render-event", G_CALLBACK(capture_transform), matrix.m);
    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glLoadIdentity();
    gtk_rtg_primitive_render(primitive);
    glPopMatrix();
    g_signal_handler_disconnect(primitive, handler);
    return matrix;
}

#include "pointer_checks.h"
#include "import_checks.h"

static void
check_interactions(Editor *editor)
{
    check_import_dialog(editor);
    check_splitters(editor);
    choose_tool(editor, "Select");
    check_snap_cursor(editor);
    for(int i = 0; i < 4; ++i)
    {
        GdkPoint point = sample_hit(editor, i);
        GtkWidget *widget = GTK_WIDGET(editor->views[i]);
        GdkWindow *window = gtk_widget_get_window(widget);
        send_button(widget, window, point.x, point.y, GDK_BUTTON_PRESS);
        send_button(widget, window, point.x, point.y, GDK_BUTTON_RELEASE);
        g_assert_true(IS_SELECTED(editor->sample));
        send_button(widget, window, 15, 30, GDK_BUTTON_PRESS);
        send_button(widget, window, 15, 30, GDK_BUTTON_RELEASE);
        g_assert_cmpuint(SELECTED(editor->scene), ==, 0);
    }
    g_print("PASS: pointer selection and background deselection in all four views\n");

    choose_tool(editor, "Move");
    for(auto up : {GTK_RTG_CANVAS_VIEW_UP_Y, GTK_RTG_CANVAS_VIEW_UP_Z})
        for(int i = 0; i < 4; ++i)
        {
            GtkRtgCanvas *canvas = editor->views[i];
            const auto preset = gtk_rtg_canvas_view_get_predefined(canvas);
            gtk_rtg_canvas_view_set_up(canvas, up);
            gtk_rtg_canvas_view_set_predefined(canvas, preset);
            gtk_rtg_canvas_view_zoom_extents(canvas);
            settle_ui();
            GdkPoint point = sample_hit(editor, i);
            GtkWidget *widget = GTK_WIDGET(canvas);
            send_button(widget, gtk_widget_get_window(widget), point.x, point.y, GDK_BUTTON_PRESS);
            send_button(widget, gtk_widget_get_window(widget), point.x, point.y, GDK_BUTTON_RELEASE);
            settle_ui();
            point = sample_hit(editor, i);
            const auto before = rendered_transform(editor->sample);
            const auto start = screen_point(canvas, before * *PIVOT(editor->sample));
            drag_canvas(canvas, point, 30, -22);
            const auto after = rendered_transform(editor->sample);
            const auto end = screen_point(canvas, after * *PIVOT(editor->sample));
            g_assert_cmpfloat(X(end) - X(start), >, 5.);
            g_assert_cmpfloat(Y(end) - Y(start), <, -5.);
            g_assert_cmpfloat((end - start).mag(), <, 60.);
            gtk_rtg_canvas_view_set_up(canvas, GTK_RTG_CANVAS_VIEW_UP_Y);
            gtk_rtg_canvas_view_set_predefined(canvas, preset);
        }
    fit_views(nullptr, editor);
    settle_ui();
    g_print("PASS: Move follows the pointer in all four views, Y-up and Z-up\n");
    check_rotation_direction(editor);

    GdkPoint hit = sample_hit(editor, 3);
    GtkWidget *view = GTK_WIDGET(editor->views[3]);
    send_button(view, gtk_widget_get_window(view), hit.x, hit.y, GDK_BUTTON_PRESS);
    send_button(view, gtk_widget_get_window(view), hit.x, hit.y, GDK_BUTTON_RELEASE);
    for(const char *tool : {"Move", "Scale", "Rotate"})
    {
        choose_tool(editor, tool);
        const auto before = rendered_transform(editor->sample);
        hit = sample_hit(editor, 3);
        drag_canvas(editor->views[3], hit, 30, -22);
        const auto after = rendered_transform(editor->sample);
        g_assert_true(memcmp(&before, &after, sizeof(before)) != 0);
        for(float component : after.m)
            g_assert_true(std::isfinite(component));
        g_print("PASS: %s gizmo drag changes the object transform\n", tool);
    }

    choose_tool(editor, "Scale");
    hit = axis_hit(editor);
    const auto before_scale = rendered_transform(editor->sample);
    drag_canvas(editor->views[3], hit, 25, -15);
    const auto after_scale = rendered_transform(editor->sample);
    int changed_axes = 0;
    for(int row = 0; row < 3; ++row)
    {
        double before_length = 0., after_length = 0.;
        for(int column = 0; column < 3; ++column)
        {
            const int index = row + column * 4;
            before_length += before_scale.m[index] * before_scale.m[index];
            after_length += after_scale.m[index] * after_scale.m[index];
        }
        if(fabs(before_length - after_length) > .0001)
            ++changed_axes;
    }
    g_assert_cmpint(changed_axes, ==, 1);
    g_print("PASS: axis-constrained nonuniform scaling\n");

    choose_tool(editor, "Orbit");
    const auto direction = *gtk_rtg_camera_view_get_direction(editor->cameras[3]);
    drag_canvas(editor->views[3], {220, 180}, 30, 20);
    g_assert_cmpfloat((direction - *gtk_rtg_camera_view_get_direction(editor->cameras[3])).mag(), >, .001);
    const auto center = *gtk_rtg_camera_view_get_center(editor->cameras[3]);
    drag_canvas(editor->views[3], {220, 180}, 30, 20, 2);
    g_assert_cmpfloat((center - *gtk_rtg_camera_view_get_center(editor->cameras[3])).mag(), >, .001);
    const double distance = gtk_rtg_camera_view_get_distance(editor->cameras[3]);
    drag_canvas(editor->views[3], {220, 180}, 0, 20, 3);
    g_assert_cmpfloat(gtk_rtg_camera_view_get_distance(editor->cameras[3]), !=, distance);
    const double before_scroll = gtk_rtg_camera_view_get_distance(editor->cameras[3]);
    start_scroll(editor->views[3]);
    settle_ui();
    g_assert_cmpfloat(gtk_rtg_camera_view_get_distance(editor->cameras[3]), <, before_scroll);
    for(int i = 0; i < 4; ++i)
    {
        gtk_rtg_camera_view_set_distance(editor->cameras[i], .02);
        gtk_rtg_canvas_set_focused(editor->views[i], TRUE);
        choose_tool(editor, "Fit");
        g_assert_cmpint(gtk_rtg_canvas_interactive_get_mode(editor->views[i]), ==, GTK_RTG_CANVAS_INT_MODE_VIEW);
        g_assert_true(std::isfinite(gtk_rtg_camera_view_get_distance(editor->cameras[i])));
        g_assert_cmpfloat(gtk_rtg_camera_view_get_distance(editor->cameras[i]), >, .1);
    }
    settle_ui();
    g_print("PASS: camera orbit/pan/zoom and Fit from a near-zero distance\n");

    gtk_widget_hide(editor->window);
    gtk_widget_unrealize(editor->window);
    gtk_widget_show_all(editor->window);
    settle_ui();
    for(auto *canvas : editor->views)
    {
        g_assert_true(gtk_rtg_canvas_make_current(canvas));
        g_assert_cmpuint(glGetError(), ==, GL_NO_ERROR);
    }
    g_print("PASS: unrealize/realize recreates all viewport contexts\n");
    choose_tool(editor, "Move");
    fit_views(nullptr, editor);
    settle_ui();
}
