#include <gtkrtg/gtkrtg.h>
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

using namespace gtkrtg;

struct Editor
{
    GtkWidget *window = nullptr;
    GtkWidget *panes = nullptr;
    GtkWidget *toolbar = nullptr;
    GtkWidget *status = nullptr;
    GtkRtgCanvas *views[4] = {};
    GtkRtgCamera *cameras[4] = {};
    GtkRtgScene *scene = nullptr;
    GtkRtgPrimitive *sample = nullptr;
    std::string model;
    bool smoke_test = false;
    int result = 0;
    std::string screenshot;
};

static void
redraw(Editor *editor)
{
    for(auto *view : editor->views)
        gtk_rtg_canvas_render(view);
}

static void
initialize_gl(GtkRtgCanvas *, gpointer)
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    const GLfloat position[] = {4.f, 8.f, 6.f, 0.f};
    const GLfloat ambient[] = {.32f, .32f, .32f, 1.f};
    glLightfv(GL_LIGHT0, GL_POSITION, position);
    glLightfv(GL_LIGHT0, GL_AMBIENT, ambient);
    glEnable(GL_LIGHT0);
    glEnable(GL_LIGHTING);
}

static void
finish_mesh(GtkRtgPrimitive *mesh, const char *name, const char *color)
{
    GdkColor rgb;
    gdk_color_parse(color, &rgb);
    gtk_rtg_primitive_set_name(mesh, const_cast<gchar *>(name));
    gtk_rtg_primitive_set_color(mesh, &rgb);
    const auto &indices = *VINDICES(mesh);
    for(size_t i = 0; i + 2 < indices.size(); i += 3)
    {
        for(int j = 0; j < 3; ++j)
        {
            guint_ind2 a = indices[i + j], b = indices[i + (j + 1) % 3];
            EDGES(mesh)->insert((std::max(a, b) << IND_BITS) | std::min(a, b));
        }
    }
    gtk_rtg_primitive_calc_bbox_bsphere(mesh);
    gtk_rtg_primitive_set_pivot(mesh, &BBOX(mesh)->p[BBOX_CENTER]);
}

static GtkRtgPrimitive *
make_cube()
{
    GtkRtgPrimitive *mesh = gtk_rtg_tri_mesh_new();
    *VERTS(mesh) = {{-1.f, -1.f, -1.f}, {1.f, -1.f, -1.f}, {1.f, 1.f, -1.f}, {-1.f, 1.f, -1.f},
                    {-1.f, -1.f, 1.f},  {1.f, -1.f, 1.f},  {1.f, 1.f, 1.f},  {-1.f, 1.f, 1.f}};
    *VINDICES(mesh) = {0, 2, 1, 0, 3, 2, 4, 5, 6, 4, 6, 7, 0, 1, 5, 0, 5, 4,
                       3, 7, 6, 3, 6, 2, 0, 4, 7, 0, 7, 3, 1, 2, 6, 1, 6, 5};
    finish_mesh(mesh, "Cube", "#df963f");
    vector3f translation(-2.f, 1.f, 0.f);
    gtk_rtg_primitive_translate(mesh, translation);
    return mesh;
}

static GtkRtgPrimitive *
make_torus()
{
    GtkRtgPrimitive *mesh = gtk_rtg_tri_mesh_new();
    constexpr int rings = 48, sides = 16;
    for(int i = 0; i < rings; ++i)
    {
        const double u = 2.0 * _PI * i / rings;
        for(int j = 0; j < sides; ++j)
        {
            const double v = 2.0 * _PI * j / sides;
            const double radius = 1.25 + .4 * cos(v);
            VERTS(mesh)->push_back(point3f(radius * cos(u), .4 * sin(v), radius * sin(u)));
            VNORMALS(mesh)->push_back(vector3f(cos(v) * cos(u), sin(v), cos(v) * sin(u)));
            const guint_ind a = i * sides + j;
            const guint_ind b = ((i + 1) % rings) * sides + j;
            const guint_ind c = ((i + 1) % rings) * sides + (j + 1) % sides;
            const guint_ind d = i * sides + (j + 1) % sides;
            for(guint_ind index : {a, c, b, a, d, c})
                VINDICES(mesh)->push_back(index);
        }
    }
    finish_mesh(mesh, "Torus", "#64a7bb");
    vector3f translation(2.f, 1.f, 0.f);
    gtk_rtg_primitive_translate(mesh, translation);
    return mesh;
}

static void
focus_view(GtkRtgCanvas *focused, gpointer data)
{
    Editor *editor = static_cast<Editor *>(data);
    for(auto *view : editor->views)
        if(view != focused)
            gtk_rtg_canvas_set_focused(view, FALSE);
}

static void
maximize_view(GtkRtgToolbar *, GtkRtgCanvas *view, gpointer data)
{
    Editor *editor = static_cast<Editor *>(data);
    auto *panes = GTK_RTG_FOUR_PANED(editor->panes);
    if(gtk_rtg_four_paned_is_macro_pane(panes))
        gtk_rtg_four_paned_restore_panes(panes);
    else if(view)
        gtk_rtg_four_paned_macro_pane(panes, GTK_WIDGET(view));
}

static void
fit_views(GtkButton *, gpointer data)
{
    Editor *editor = static_cast<Editor *>(data);
    for(auto *view : editor->views)
        gtk_rtg_canvas_view_zoom_extents(view);
    redraw(editor);
}

static void
selection_changed(GtkRtgScene *scene, gpointer data)
{
    Editor *editor = static_cast<Editor *>(data);
    const auto *selected = gtk_rtg_scene_get_selected(scene);
    gchar *label = selected->empty()
                       ? g_strdup("Select an object to move, rotate or scale it")
                       : g_strdup_printf("%zu selected — %s", selected->size(), NAME(*selected->begin()));
    gtk_label_set_text(GTK_LABEL(editor->status), label);
    g_free(label);
    redraw(editor);
}

static void
import_model(Editor *editor, const char *path)
{
    GtkRtgPrimitive *mesh = gtk_rtg_tri_mesh_new();
    try
    {
        gtk_rtg_tri_mesh_load_from_wavefront_obj(GTK_RTG_TRI_MESH(mesh), path);
        if(VNORMALS(mesh)->empty())
            gtk_rtg_tri_mesh_calc_vertex_normals(GTK_RTG_TRI_MESH(mesh), 180.f);
        gchar *name = g_path_get_basename(path);
        finish_mesh(mesh, name, "#df963f");
        g_free(name);
        gtk_rtg_scene_primitive_push(editor->scene, mesh);
        editor->sample = mesh;
        g_print("Loaded %s: %zu vertices, %zu triangles\n", path, VERTS(mesh)->size(),
                VINDICES(mesh)->size() / 3);
        g_object_unref(mesh);
    }
    catch(...)
    {
        g_object_unref(mesh);
        throw;
    }
}

static void
open_model(GtkToolButton *, gpointer data)
{
    Editor *editor = static_cast<Editor *>(data);
    GtkWidget *dialog =
        gtk_file_chooser_dialog_new("Import OBJ", GTK_WINDOW(editor->window), GTK_FILE_CHOOSER_ACTION_OPEN,
                                    "Cancel", GTK_RESPONSE_CANCEL, "Import", GTK_RESPONSE_ACCEPT, nullptr);
    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "Wavefront OBJ (*.obj)");
    gtk_file_filter_add_pattern(filter, "*.obj");
    gtk_file_filter_add_pattern(filter, "*.OBJ");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    if(gtk_dialog_run(GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT)
    {
        gchar *path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
        try
        {
            import_model(editor, path);
            fit_views(nullptr, editor);
        }
        catch(const GtkRtgException &error)
        {
            GtkWidget *message =
                gtk_message_dialog_new(GTK_WINDOW(editor->window), GTK_DIALOG_MODAL, GTK_MESSAGE_ERROR,
                                       GTK_BUTTONS_CLOSE, "Could not import OBJ");
            gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(message), "%s", error.what());
            gtk_dialog_run(GTK_DIALOG(message));
            gtk_widget_destroy(message);
        }
        g_free(path);
    }
    gtk_widget_destroy(dialog);
}

static bool
save_screenshot(Editor *editor)
{
    if(editor->screenshot.empty())
        return true;
    const int width = gtk_widget_get_allocated_width(editor->window);
    const int height = gtk_widget_get_allocated_height(editor->window);
    cairo_surface_t *surface = cairo_image_surface_create(CAIRO_FORMAT_RGB24, width, height);
    cairo_t *cr = cairo_create(surface);
    gtk_widget_draw(editor->window, cr);
    cairo_status_t result = cairo_surface_write_to_png(surface, editor->screenshot.c_str());
    cairo_destroy(cr);
    cairo_surface_destroy(surface);
    return result == CAIRO_STATUS_SUCCESS;
}

#include "editor_checks.h"

static gboolean
finish_smoke(gpointer data)
{
    Editor *editor = static_cast<Editor *>(data);
    try
    {
        for(int i = 0; i < 4; ++i)
        {
            g_assert_true(gtk_rtg_canvas_make_current(editor->views[i]));
            g_assert_nonnull(glGetString(GL_VERSION));
            g_assert_true(gtk_rtg_camera_is_initialized(editor->cameras[i]));
            g_assert_cmpuint(glGetError(), ==, GL_NO_ERROR);
        }
        check_interactions(editor);
        g_assert_true(save_screenshot(editor));
        g_print("PASS: four GTK 3 viewports initialized and rendered\n");
    }
    catch(GtkRtgException &error)
    {
        error.trace();
        editor->result = 1;
    }
    gtk_rtg_canvas_interactive_set_mode(editor->views[3], GTK_RTG_CANVAS_INT_MODE_VIEW);
    start_scroll(editor->views[3]);
    gtk_widget_destroy(editor->window);
    settle_ui();
    g_print("PASS: shutdown during animated scroll\n");
    return G_SOURCE_REMOVE;
}

static gboolean
capture_only(gpointer data)
{
    Editor *editor = static_cast<Editor *>(data);
    if(!save_screenshot(editor))
        g_warning("Could not save screenshot");
    return G_SOURCE_REMOVE;
}

int
main(int argc, char **argv)
{
    Editor editor;
    for(int i = 1; i < argc; ++i)
    {
        if(!strcmp(argv[i], "--smoke-test"))
            editor.smoke_test = true;
        else if(!strcmp(argv[i], "--model") && i + 1 < argc)
            editor.model = argv[++i];
        else if(!strcmp(argv[i], "--screenshot") && i + 1 < argc)
            editor.screenshot = argv[++i];
        else
        {
            g_print("Usage: %s [--model FILE.obj] [--smoke-test] [--screenshot FILE.png]\n", argv[0]);
            return strcmp(argv[i], "--help") ? 2 : 0;
        }
    }
    try
    {
        gtk_rtg_init(nullptr, nullptr);
        g_object_set(gtk_settings_get_default(), "gtk-application-prefer-dark-theme", TRUE, nullptr);
        editor.window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
        gtk_window_set_title(GTK_WINDOW(editor.window), "3dWhiz");
        gtk_window_set_default_size(GTK_WINDOW(editor.window), 1200, 820);
        g_signal_connect(editor.window, "destroy", G_CALLBACK(gtk_main_quit), nullptr);
        GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
        gtk_container_add(GTK_CONTAINER(editor.window), box);
        editor.toolbar = gtk_rtg_toolbar_new();
        gtk_box_pack_start(GTK_BOX(box), editor.toolbar, FALSE, FALSE, 0);
        GtkToolItem *fit = gtk_tool_button_new(nullptr, "Fit all views");
        gtk_toolbar_insert(GTK_TOOLBAR(editor.toolbar), fit, -1);
        g_signal_connect(fit, "clicked", G_CALLBACK(fit_views), &editor);
        GtkToolItem *open = gtk_tool_button_new(nullptr, "Import OBJ…");
        gtk_toolbar_insert(GTK_TOOLBAR(editor.toolbar), open, -1);
        g_signal_connect(open, "clicked", G_CALLBACK(open_model), &editor);
        editor.panes = gtk_rtg_four_paned_new();
        gtk_box_pack_start(GTK_BOX(box), editor.panes, TRUE, TRUE, 0);
        for(int i = 0; i < 4; ++i)
        {
            editor.views[i] = GTK_RTG_CANVAS(gtk_rtg_canvas_new());
            gtk_widget_set_size_request(GTK_WIDGET(editor.views[i]), 180, 150);
            gtk_rtg_canvas_init_funct_add(editor.views[i], initialize_gl, nullptr);
            g_signal_connect(editor.views[i], "focused", G_CALLBACK(focus_view), &editor);
            GdkColor background;
            gdk_color_parse("#42464c", &background);
            g_object_set(editor.views[i], "bg-color", &background, nullptr);
        }
        auto *panes = GTK_RTG_FOUR_PANED(editor.panes);
        gtk_rtg_four_paned_pack1(panes, GTK_WIDGET(editor.views[0]));
        gtk_rtg_four_paned_pack2(panes, GTK_WIDGET(editor.views[1]));
        gtk_rtg_four_paned_pack3(panes, GTK_WIDGET(editor.views[2]));
        gtk_rtg_four_paned_pack4(panes, GTK_WIDGET(editor.views[3]));
        editor.status = gtk_label_new("Select an object to move, rotate or scale it");
        gtk_widget_set_margin_top(editor.status, 6);
        gtk_widget_set_margin_bottom(editor.status, 6);
        gtk_box_pack_start(GTK_BOX(box), editor.status, FALSE, FALSE, 0);
        GtkWidget *hint = gtk_label_new("Orbit mode: left drag · Pan: middle drag · Zoom: scroll / right "
                                        "drag · F3: wireframe · F4: edges");
        gtk_widget_set_margin_bottom(hint, 6);
        gtk_box_pack_start(GTK_BOX(box), hint, FALSE, FALSE, 0);
        gtk_rtg_toolbar_canvas_push(GTK_RTG_TOOLBAR(editor.toolbar), editor.views[0], editor.views[1],
                                    editor.views[2], editor.views[3], nullptr);
        g_signal_connect(editor.toolbar, "maximize-viewport", G_CALLBACK(maximize_view), &editor);
        gtk_widget_show_all(editor.window);

        editor.scene = gtk_rtg_scene_new();
        if(!editor.model.empty())
            import_model(&editor, editor.model.c_str());
        else
        {
            editor.sample = make_cube();
            gtk_rtg_scene_primitive_push(editor.scene, editor.sample);
            g_object_unref(editor.sample);
            GtkRtgPrimitive *torus = make_torus();
            gtk_rtg_scene_primitive_push(editor.scene, torus);
            g_object_unref(torus);
        }
        for(auto &camera : editor.cameras)
        {
            camera = GTK_RTG_CAMERA(gtk_rtg_camera_new());
            gtk_rtg_scene_primitive_push(editor.scene, GTK_RTG_PRIMITIVE(camera));
            g_object_unref(camera);
        }
        const GtkRtgCanvasViewPredefined presets[] = {GTK_RTG_CANVAS_VIEW_TOP, GTK_RTG_CANVAS_VIEW_FRONT,
                                                      GTK_RTG_CANVAS_VIEW_RIGHT, GTK_RTG_CANVAS_VIEW_CUSTOM};
        for(int i = 0; i < 4; ++i)
        {
            g_throw_if_fail(gtk_rtg_canvas_make_current(editor.views[i]));
            GtkRtgCanvasGrid *grid = gtk_rtg_canvas_grid_new();
            gtk_rtg_canvas_grid_set_spacing(grid, 1.f);
            gtk_rtg_canvas_grid_set_lines(grid, 30);
            gtk_rtg_canvas_grid_push(editor.views[i], grid);
            g_object_unref(grid);
            gtk_rtg_canvas_scene_push(editor.views[i], editor.scene);
            gtk_rtg_camera_prioritize(editor.cameras[i], editor.views[i]);
            vector3d direction(-1., -.7, -1.);
            direction.norm();
            gtk_rtg_camera_view_set_direction(editor.cameras[i], &direction);
            gtk_rtg_canvas_view_set_predefined(editor.views[i], presets[i]);
            gtk_rtg_canvas_view_zoom_extents(editor.views[i]);
            gtk_rtg_canvas_interactive_set_mode(editor.views[i], GTK_RTG_CANVAS_INT_MODE_SELECT);
        }
        g_signal_connect(editor.scene, "prim-sel-changed", G_CALLBACK(selection_changed), &editor);
        gtk_rtg_canvas_set_focused(editor.views[3], TRUE);
        redraw(&editor);
        if(editor.smoke_test)
            g_timeout_add(1000, finish_smoke, &editor);
        else if(!editor.screenshot.empty())
            g_timeout_add(1000, capture_only, &editor);
        gtk_main();
        g_signal_handlers_disconnect_by_data(editor.scene, &editor);
        g_object_unref(editor.scene);
    }
    catch(GtkRtgException &error)
    {
        error.trace();
        return 1;
    }
    return editor.result;
}
