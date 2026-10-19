#include <glib/gstdio.h>
#include <vector>

struct ImportDialogCheck
{
    Editor *editor;
    const char *path;
    bool expect_error;
    bool selected = false;
    bool responded = false;
    bool completed = false;
    guint attempts = 0;
};

static gboolean
respond_to_import(gpointer data)
{
    auto *check = static_cast<ImportDialogCheck *>(data);
    g_assert_cmpuint(++check->attempts, <, 500);
    GList *windows = gtk_window_list_toplevels();
    for(GList *node = windows; node; node = node->next)
    {
        auto *widget = GTK_WIDGET(node->data);
        if(!gtk_widget_get_visible(widget) ||
           gtk_window_get_transient_for(GTK_WINDOW(widget)) != GTK_WINDOW(check->editor->window))
            continue;
        if(GTK_IS_MESSAGE_DIALOG(widget) && check->expect_error && check->responded)
        {
            gchar *detail = nullptr;
            g_object_get(widget, "secondary-text", &detail, nullptr);
            g_assert_nonnull(strstr(detail, "OBJ line"));
            g_free(detail);
            check->completed = true;
            gtk_dialog_response(GTK_DIALOG(widget), GTK_RESPONSE_CLOSE);
            break;
        }
        if(!GTK_IS_FILE_CHOOSER_DIALOG(widget) || check->responded)
            continue;
        if(!check->path)
        {
            check->completed = true;
            gtk_dialog_response(GTK_DIALOG(widget), GTK_RESPONSE_CANCEL);
            break;
        }
        auto *chooser = GTK_FILE_CHOOSER(widget);
        if(!check->selected)
        {
            g_assert_true(gtk_file_chooser_set_filename(chooser, check->path));
            check->selected = true;
            break;
        }
        gchar *selected = gtk_file_chooser_get_filename(chooser);
        const bool ready = g_strcmp0(selected, check->path) == 0;
        g_free(selected);
        if(ready)
        {
            check->responded = true;
            check->completed = !check->expect_error;
            gtk_dialog_response(GTK_DIALOG(widget), GTK_RESPONSE_ACCEPT);
            break;
        }
    }
    g_list_free(windows);
    return check->completed ? G_SOURCE_REMOVE : G_SOURCE_CONTINUE;
}

static void
check_import_dialog(Editor *editor)
{
    GtkWidget *button = find_button(editor->toolbar, "Import OBJ…");
    g_assert_nonnull(button);
    gchar *directory = g_dir_make_tmp("3dwhiz-import-XXXXXX", nullptr);
    g_assert_nonnull(directory);
    gchar *valid = g_build_filename(directory, "Tetra with spaces.OBJ", nullptr);
    gchar *invalid = g_build_filename(directory, "broken.obj", nullptr);
    g_assert_true(g_file_set_contents(valid, "v 0 0 0\nv 1 0 0\nv 0 1 0\nv 0 0 1\n"
                                            "f 1 3 2\nf 1 2 4\nf 1 4 3\nf 2 3 4\n", -1, nullptr));
    g_assert_true(g_file_set_contents(invalid, "v nope 0 0\nf 1 2 3\n", -1, nullptr));
    GtkRtgPrimitive *original = editor->sample;
    const auto original_count = gtk_rtg_scene_get_all_prims(editor->scene)->size();
    const char *paths[] = {nullptr, valid, invalid};
    for(const char *path : paths)
    {
        std::vector<GtkRtgPrimitive *> hidden;
        if(path == valid)
            for(auto *primitive : *gtk_rtg_scene_get_all_prims(editor->scene))
                if(!GTK_RTG_IS_CAMERA(primitive) && IS_VISIBLE(primitive))
                {
                    hidden.push_back(primitive);
                    VISIBLE(primitive, FALSE);
                }
        ImportDialogCheck check{editor, path, path == invalid};
        g_timeout_add(20, respond_to_import, &check);
        g_signal_emit_by_name(button, "clicked");
        g_assert_true(check.completed);
        settle_ui();
        if(path == valid)
        {
            g_assert_cmpuint(gtk_rtg_scene_get_all_prims(editor->scene)->size(), ==, original_count + 1);
            g_assert_true(editor->sample != original);
            g_assert_cmpuint(VERTS(editor->sample)->size(), ==, 4);
            g_assert_cmpuint(VINDICES(editor->sample)->size(), ==, 12);
            for(int i = 0; i < 4; ++i)
                sample_hit(editor, i);
            gtk_rtg_scene_primitive_pop(editor->scene, editor->sample);
            editor->sample = original;
            for(auto *primitive : hidden)
                VISIBLE(primitive, TRUE);
            fit_views(nullptr, editor);
        }
        else
            g_assert_true(editor->sample == original);
        g_assert_cmpuint(gtk_rtg_scene_get_all_prims(editor->scene)->size(), ==, original_count);
    }
    g_unlink(valid);
    g_unlink(invalid);
    g_rmdir(directory);
    g_free(valid);
    g_free(invalid);
    g_free(directory);
    g_print("PASS: Import OBJ dialog opens, cancels, imports a model and reports malformed input\n");
}
