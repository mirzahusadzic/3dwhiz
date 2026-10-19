#ifndef GL_SPECS
#define GL_SPECS

#include <gtkrtg/gtkrtgbase.h>

inline void
check_error()
{
	const GLenum error = glGetError();

	if(error != GL_NO_ERROR)
	{
		const GLubyte* errorStr = gluErrorString(error);
		g_warning("Error: %s \n", errorStr);
	}

	g_return_if_fail(error == GL_NO_ERROR);
}

#define GL_CHECK_ERROR check_error();

#endif /* GL_SPECS */
