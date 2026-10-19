#ifndef GTK_RTG_COLOR_ID_H
#define GTK_RTG_COLOR_ID_H

/*
 * Color id's used for color picking.
 *
 * Supports up to approx. 2^24 primitives (fcon: support alpha channel).
 */
enum
{
	COLOR_ID_RESERVED_START,

	/* gizmo */
	COLOR_ID_GIZMO_AX_X,
	COLOR_ID_GIZMO_AX_Y,
	COLOR_ID_GIZMO_AX_Z,
	COLOR_ID_GIZMO_PLANE_XY,
	COLOR_ID_GIZMO_PLANE_XZ,
	COLOR_ID_GIZMO_PLANE_YZ,
	COLOR_ID_GIZMO_AXES,
	COLOR_ID_GIZMO_PLANE_CAMERA,
    COLOR_ID_GIZMO_CIRCLE,

	/* prims */
	COLOR_ID_PRIMITIVE,

	COLOR_ID_RESERVED_END = 0xFFFFFE,
	COLOR_ID_END
};

#endif /* GTK_RTG_COLOR_ID_H */
