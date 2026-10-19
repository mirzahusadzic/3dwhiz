/* These checks compare pointer motion with projected and rendered geometry. */
static point3d
screen_point(GtkRtgCanvas *canvas, const point3f &point)
{
    GtkRtgCamera *camera = gtk_rtg_canvas_camera_get_default(canvas);
    GLdouble model[16], projection[16];
    for(int i = 0; i < 16; ++i)
    {
        model[i] = M_VIEW(camera).m[i];
        projection[i] = M_PROJ(camera).m[i];
    }
    GLint viewport[] = {0, 0, gtk_widget_get_allocated_width(GTK_WIDGET(canvas)),
                            gtk_widget_get_allocated_height(GTK_WIDGET(canvas))};
    point3d result;
    g_assert_true(gluProject(X(point), Y(point), Z(point), model, projection, viewport,
                            &X(result), &Y(result), &Z(result)));
    Y(result) = viewport[3] - Y(result);
    return result;
}

static point3f
world_point(GtkRtgCanvas *canvas, double x, double y, double depth)
{
    GtkRtgCamera *camera = gtk_rtg_canvas_camera_get_default(canvas);
    const auto &view = M_VIEW(camera);
    GLdouble identity[16] = {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
    GLdouble projection[16];
    for(int i = 0; i < 16; ++i)
        projection[i] = M_PROJ(camera).m[i];
    GLint viewport[] = {0, 0, gtk_widget_get_allocated_width(GTK_WIDGET(canvas)),
                            gtk_widget_get_allocated_height(GTK_WIDGET(canvas))};
    point3d eye;
    g_assert_true(gluUnProject(x, viewport[3] - y, depth, identity, projection, viewport,
                              &X(eye), &Y(eye), &Z(eye)));
    eye -= point3d(view.m[12], view.m[13], view.m[14]);
    /* Invert the rigid camera analytically; GLU's old pivoting fails near a pole. */
    return point3f(view.m[0] * X(eye) + view.m[1] * Y(eye) + view.m[2] * Z(eye),
                   view.m[4] * X(eye) + view.m[5] * Y(eye) + view.m[6] * Z(eye),
                   view.m[8] * X(eye) + view.m[9] * Y(eye) + view.m[10] * Z(eye));
}

static void
check_snap_cursor(Editor *editor)
{
    GtkRtgCanvas *canvas = editor->views[0];
    GtkWidget *widget = GTK_WIDGET(canvas);
    GtkRtgCamera *camera = editor->cameras[0];
    GtkRtgCanvasGrid *grid = gtk_rtg_canvas_get_grid(canvas);
    const auto saved_center = *gtk_rtg_camera_view_get_center(camera);
    const auto saved_direction = *gtk_rtg_camera_view_get_direction(camera);
    const double saved_distance = gtk_rtg_camera_view_get_distance(camera);
    const auto saved_preset = gtk_rtg_canvas_view_get_predefined(canvas);
    const auto saved_up = gtk_rtg_canvas_view_get_up(canvas);
    gtk_rtg_canvas_grid_set_snap_cursor(grid, TRUE);

    for(auto up : {GTK_RTG_CANVAS_VIEW_UP_Y, GTK_RTG_CANVAS_VIEW_UP_Z})
        for(int preset = GTK_RTG_CANVAS_VIEW_FRONT; preset <= GTK_RTG_CANVAS_VIEW_CUSTOM; ++preset)
        {
            gtk_rtg_canvas_view_set_up(canvas, up);
            gtk_rtg_canvas_view_set_predefined(canvas, GtkRtgCanvasViewPredefined(preset));
            if(preset == GTK_RTG_CANVAS_VIEW_CUSTOM)
            {
                vector3d direction(-1., -.7, -1.);
                direction.norm();
                gtk_rtg_camera_view_set_direction(camera, &direction);
            }
            point3d center(1.7, 2.3, .8);
            gtk_rtg_camera_view_set_center(camera, &center);
            gtk_rtg_camera_view_set_distance(camera, 20.);
            gtk_rtg_camera_view_do_matrix(camera);
            settle_ui();

            int normal_axis;
            if(preset == GTK_RTG_CANVAS_VIEW_FRONT || preset == GTK_RTG_CANVAS_VIEW_BACK)
                normal_axis = up == GTK_RTG_CANVAS_VIEW_UP_Y ? 0 : 1;
            else if(preset == GTK_RTG_CANVAS_VIEW_LEFT || preset == GTK_RTG_CANVAS_VIEW_RIGHT)
                normal_axis = up == GTK_RTG_CANVAS_VIEW_UP_Y ? 2 : 0;
            else
                normal_axis = up == GTK_RTG_CANVAS_VIEW_UP_Y ? 1 : 2;

            for(float sign : {-1.f, 1.f})
            {
                point3f expected(2.f * sign, -sign, sign);
                VALUES(expected)[normal_axis] = 0.f;
                point3f nearby = expected;
                VALUES(nearby)[(normal_axis + 1) % 3] += .2f;
                const auto pointer = screen_point(canvas, nearby);
                /* A different-sized pane deliberately owns the current GL context. */
                g_assert_true(gtk_rtg_canvas_make_current(editor->views[3]));
                send_motion(widget, gtk_widget_get_window(widget), X(pointer), Y(pointer));
                const auto actual = gtk_rtg_canvas_grid_get_snap_point(grid);
                if((actual - expected).mag() >= .001)
                {
                    g_print("SNAP up=%d preset=%d actual=(%g,%g,%g) expected=(%g,%g,%g) pointer=(%g,%g)\n",
                            up, preset, X(actual), Y(actual), Z(actual), X(expected), Y(expected), Z(expected), X(pointer), Y(pointer));
                }
                g_assert_cmpfloat((actual - expected).mag(), <, .001);

                g_assert_true(gtk_rtg_canvas_make_current(canvas));
                glMatrixMode(GL_PROJECTION);
                glPushMatrix();
                glLoadMatrixf(M_PROJ(camera).m);
                glMatrixMode(GL_MODELVIEW);
                glPushMatrix();
                glLoadMatrixf(M_VIEW(camera).m);
                GLfloat feedback[64];
                glFeedbackBuffer(64, GL_2D, feedback);
                glRenderMode(GL_FEEDBACK);
                gtk_rtg_canvas_grid_render_snap_cursor(grid, canvas);
                const GLint count = glRenderMode(GL_RENDER);
                glPopMatrix();
                glMatrixMode(GL_PROJECTION);
                glPopMatrix();
                glMatrixMode(GL_MODELVIEW);
                g_assert_cmpint(count, ==, 30);
                const auto projected = screen_point(canvas, expected);
                /* Perspective does not preserve the midpoint of a line segment. */
                const double ax = feedback[3] - feedback[1], ay = feedback[4] - feedback[2];
                const double bx = feedback[8] - feedback[6], by = feedback[9] - feedback[7];
                const double cross = ax * by - ay * bx;
                g_assert_cmpfloat(fabs(cross), >, .001);
                const double t = ((feedback[6] - feedback[1]) * by -
                                  (feedback[7] - feedback[2]) * bx) / cross;
                g_assert_cmpfloat_with_epsilon(feedback[1] + t * ax, X(projected), .02);
                g_assert_cmpfloat_with_epsilon(feedback[2] + t * ay,
                                              gtk_widget_get_allocated_height(widget) - Y(projected), .02);
            }
        }
    gtk_rtg_canvas_grid_set_snap_cursor(grid, FALSE);
    gtk_rtg_canvas_view_set_up(canvas, saved_up);
    gtk_rtg_canvas_view_set_predefined(canvas, saved_preset);
    gtk_rtg_camera_view_set_direction(camera, &saved_direction);
    gtk_rtg_camera_view_set_center(camera, &saved_center);
    gtk_rtg_camera_view_set_distance(camera, saved_distance);
    gtk_rtg_camera_view_do_matrix(camera);
    settle_ui();
    g_print("PASS: snap point and rendered cursor agree in all seven views, Y-up and Z-up\n");
}

static GdkPoint
gizmo_hit(Editor *editor, int view, guint wanted)
{
    GtkRtgCanvas *canvas = editor->views[view];
    g_assert_true(gtk_rtg_canvas_make_current(canvas));
    GdkPoint point = {2, 2};
    gtk_rtg_scene_point_select(editor->scene, point, canvas, TRUE);
    auto *buffer = static_cast<GtkRtgPickingBuffer *>(g_object_get_data(G_OBJECT(canvas), "pickbuff"));
    GdkPoint best = {-1, 0};
    for(int y = 10; y < buffer->height - 10; ++y)
        for(int x = 10; x < buffer->width - 10; ++x)
        {
            const auto *pixel = buffer->buff + (y * buffer->width + x) * 4;
            const guint id = pixel[0] | (pixel[1] << 8) | (pixel[2] << 16);
            if(id == wanted && x > best.x)
                best = {x, buffer->height - y - 1};
        }
    g_assert_cmpint(best.x, >=, 0);
    return best;
}

static void
check_rotation_direction(Editor *editor)
{
    choose_tool(editor, "Rotate");
    /* XY, XZ and YZ ring IDs retain their original internal ordering. */
    const guint rings[] = {COLOR_ID_GIZMO_AX_Y, COLOR_ID_GIZMO_AX_Z, COLOR_ID_GIZMO_AX_X};
    for(int i = 0; i < 3; ++i)
    {
        GtkRtgCanvas *canvas = editor->views[i];
        for(int dy : {16, -16})
        {
            GdkPoint hit = gizmo_hit(editor, i, rings[i]);
            auto before = rendered_transform(editor->sample);
            const point3f center = before * *PIVOT(editor->sample);
            const auto screen_center = screen_point(canvas, center);
            const point3f world = world_point(canvas, hit.x, hit.y, Z(screen_center));
            const point3f local = before.inverse() * world;
            const auto start = screen_point(canvas, before * local);
            g_assert_cmpfloat_with_epsilon(X(start), hit.x, .02);
            g_assert_cmpfloat_with_epsilon(Y(start), hit.y, .02);
            drag_canvas(canvas, hit, 0, dy);
            const auto after = rendered_transform(editor->sample);
            const auto end = screen_point(canvas, after * local);
            g_assert_cmpfloat((Y(end) - Y(start)) * dy, >, .1);
            g_assert_cmpfloat(fabs(Y(end) - Y(start)), <, 40.);
        }
    }
    g_print("PASS: object rotation follows clockwise and counterclockwise pointer motion\n");

    GtkRtgCanvas *canvas = editor->views[3];
    for(guint ring : {COLOR_ID_GIZMO_AX_X, COLOR_ID_GIZMO_AX_Y, COLOR_ID_GIZMO_AX_Z,
                      COLOR_ID_GIZMO_PLANE_CAMERA})
        for(float sign : {-1.f, 1.f})
        {
            const GdkPoint hit = gizmo_hit(editor, 3, ring);
            auto before = rendered_transform(editor->sample);
            const point3f center = before * *PIVOT(editor->sample);
            vector3f axis = ring == COLOR_ID_GIZMO_AX_X ? vector3f(0, 0, 1) :
                            ring == COLOR_ID_GIZMO_AX_Y ? vector3f(0, 1, 0) : vector3f(1, 0, 0);
            if(ring == COLOR_ID_GIZMO_PLANE_CAMERA)
                axis = cast3<float, double>(*gtk_rtg_camera_view_get_eye(editor->cameras[3])) - center;
            axis.norm();
            const point3f near = world_point(canvas, hit.x, hit.y, 0.);
            const vector3f ray = world_point(canvas, hit.x, hit.y, 1.) - near;
            g_assert_cmpfloat(fabs(axis.dot(ray)), >, .001);
            const point3f world = near + ray * (axis.dot(center - near) / axis.dot(ray));
            const point3f local = before.inverse() * world;
            const auto start = screen_point(canvas, before * local);
            const auto tangent = screen_point(canvas, world + axis.cross(world - center) * .01f);
            double dx = X(tangent) - X(start), dy = Y(tangent) - Y(start);
            const double length = sqrt(dx * dx + dy * dy);
            g_assert_cmpfloat(length, >, .001);
            dx *= sign * 16. / length;
            dy *= sign * 16. / length;
            drag_canvas(canvas, hit, lround(dx), lround(dy));
            const auto after = rendered_transform(editor->sample);
            const auto end = screen_point(canvas, after * local);
            g_assert_cmpfloat((X(end) - X(start)) * dx + (Y(end) - Y(start)) * dy, >, 5.);
            g_assert_cmpfloat((end - start).mag(), <, 50.);
            g_assert_cmpfloat((after * *PIVOT(editor->sample) - center).mag(), <, .001f);
        }
    g_print("PASS: all three perspective rotation rings and camera ring follow the pointer\n");

    for(int i = 0; i < 4; ++i)
        for(GdkPoint delta : {GdkPoint{12, 0}, GdkPoint{-12, 0}, GdkPoint{0, 12}, GdkPoint{0, -12}})
        {
            canvas = editor->views[i];
            const GdkPoint hit = sample_hit(editor, i);
            auto before = rendered_transform(editor->sample);
            const point3f center = before * *PIVOT(editor->sample);
            vector3f toward_eye = cast3<float, double>(*gtk_rtg_camera_view_get_eye(editor->cameras[i])) - center;
            toward_eye.norm();
            /* Follow a front-facing point on the virtual ball while dragging the object body. */
            const point3f local = before.inverse() * (center + toward_eye);
            const auto start = screen_point(canvas, before * local);
            drag_canvas(canvas, hit, delta.x, delta.y);
            const auto after = rendered_transform(editor->sample);
            const auto end = screen_point(canvas, after * local);
            g_assert_cmpfloat((X(end) - X(start)) * delta.x + (Y(end) - Y(start)) * delta.y, >, .1);
            g_assert_cmpfloat((after * *PIVOT(editor->sample) - center).mag(), <, .001f);
        }
    g_print("PASS: free object rotation follows horizontal and vertical drags in all four views\n");
}
