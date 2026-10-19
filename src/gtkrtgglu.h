#ifndef GTK_RTG_GLU_H
#define GTK_RTG_GLU_H

#include <gtkrtg/gtkrtgbase.h>
#include <gtkrtg/gtkrtgmath.h>

/*
 * GLU 'missing' functions <float based>
 *
 */

inline void
__gluMultMatricesf(const GLfloat a[16], const GLfloat b[16], GLfloat r[16])
{
	int i, j;

	for(i = 0; i < 4; i++)
	{
		for(j = 0; j < 4; j++)
		{
			r[i*4+j] = a[i*4+0]*b[0*4+j] +
					   a[i*4+1]*b[1*4+j] +
					   a[i*4+2]*b[2*4+j] +
					   a[i*4+3]*b[3*4+j];
		}
	}
}


inline
void __gluMakeIdentityf(GLfloat m[16])
{
	m[0+4*0] = 1; m[0+4*1] = 0; m[0+4*2] = 0; m[0+4*3] = 0;
	m[1+4*0] = 0; m[1+4*1] = 1; m[1+4*2] = 0; m[1+4*3] = 0;
	m[2+4*0] = 0; m[2+4*1] = 0; m[2+4*2] = 1; m[2+4*3] = 0;
	m[3+4*0] = 0; m[3+4*1] = 0; m[3+4*2] = 0; m[3+4*3] = 1;
}


/*
** inverse = invert(src)
** New, faster implementation by Shan Hao Bo, April 2006.
*/
inline
int __gluInvertMatrixf(const GLfloat src[16], GLfloat inverse[16])
{
	/* Partial pivoting avoids dividing by the tiny components of axial views. */
	GLdouble augmented[4][8];
	for(int row = 0; row < 4; ++row)
		for(int column = 0; column < 4; ++column)
		{
			augmented[row][column] = src[row * 4 + column];
			augmented[row][column + 4] = row == column ? 1.0 : 0.0;
		}

	for(int column = 0; column < 4; ++column)
	{
		int pivot = column;
		for(int row = column + 1; row < 4; ++row)
			if(fabs(augmented[row][column]) > fabs(augmented[pivot][column]))
				pivot = row;
		if(augmented[pivot][column] == 0.0)
			return GL_FALSE;

		if(pivot != column)
			for(int k = 0; k < 8; ++k)
			{
				const GLdouble value = augmented[column][k];
				augmented[column][k] = augmented[pivot][k];
				augmented[pivot][k] = value;
			}

		const GLdouble divisor = augmented[column][column];
		for(int k = 0; k < 8; ++k)
			augmented[column][k] /= divisor;
		for(int row = 0; row < 4; ++row)
		{
			if(row == column)
				continue;
			const GLdouble factor = augmented[row][column];
			for(int k = 0; k < 8; ++k)
				augmented[row][k] -= factor * augmented[column][k];
		}
	}

	for(int row = 0; row < 4; ++row)
		for(int column = 0; column < 4; ++column)
			inverse[row * 4 + column] = GLfloat(augmented[row][column + 4]);
	return GL_TRUE;
}


inline void
__gluMultMatrixVecf(const GLfloat matrix[16],
                    const GLfloat in[4],
                          GLfloat out[4]       )
{
	int i;

	for (i=0; i<4; i++)
	{
		out[i] = in[0] * matrix[0*4+i] +
				 in[1] * matrix[1*4+i] +
				 in[2] * matrix[2*4+i] +
				 in[3] * matrix[3*4+i];
	}
}


inline GLint
gluUnProjectf(      GLfloat  winx,
                    GLfloat  winy,
                    GLfloat  winz,
              const GLfloat  modelMatrix[16],
              const GLfloat  projMatrix[16],
              const GLint    viewport[4],
	            	GLfloat *objx,
                    GLfloat *objy,
                    GLfloat *objz               )
{
	float finalMatrix[16];
	float in[4];
	float out[4];

	__gluMultMatricesf(modelMatrix, projMatrix, finalMatrix);
	if(!__gluInvertMatrixf(finalMatrix, finalMatrix)) return(GL_FALSE);

	in[0]=winx;
	in[1]=winy;
	in[2]=winz;
	in[3]=1.0;

	/* Map x and y from window coordinates */
	in[0] = (in[0] - (gfloat)viewport[0]) / (gfloat)viewport[2];
	in[1] = (in[1] - (gfloat)viewport[1]) / (gfloat)viewport[3];

	/* Map to range -1 to 1 */
	in[0] = in[0] * 2 - 1;
	in[1] = in[1] * 2 - 1;
	in[2] = in[2] * 2 - 1;

	__gluMultMatrixVecf(finalMatrix, in, out);
	if (out[3] == 0.0) return(GL_FALSE);
	out[0] /= out[3];
	out[1] /= out[3];
	out[2] /= out[3];
	*objx = out[0];
	*objy = out[1];
	*objz = out[2];

	return(GL_TRUE);
}

inline GLint
gluUnProjectf(GLfloat        winx,
              GLfloat        winy,
              GLfloat        winz,
              const GLfloat  finalMatrix[16],
              const GLint    viewport[4],
              GLfloat       *objx,
              GLfloat       *objy,
              GLfloat       *objz               )
{
	float in[4];
	float out[4];

	in[0]=winx;
	in[1]=winy;
	in[2]=winz;
	in[3]=1.0;

	/* Map x and y from window coordinates */
	in[0] = (in[0] - (gfloat)viewport[0]) / (gfloat)viewport[2];
	in[1] = (in[1] - (gfloat)viewport[1]) / (gfloat)viewport[3];

	/* Map to range -1 to 1 */
	in[0] = in[0] * 2 - 1;
	in[1] = in[1] * 2 - 1;
	in[2] = in[2] * 2 - 1;

	__gluMultMatrixVecf(finalMatrix, in, out);
	if (out[3] == 0.0) return(GL_FALSE);
	out[0] /= out[3];
	out[1] /= out[3];
	out[2] /= out[3];
	*objx = out[0];
	*objy = out[1];
	*objz = out[2];

	return(GL_TRUE);
}


inline GLint
gluProjectf(GLfloat   objx,
            GLfloat   objy,
            GLfloat   objz,
            const     GLfloat     modelMatrix[16],
            const     GLfloat     projMatrix[16],
            const     GLint       viewport[4],
                      GLint      *winx,
                      GLint      *winy,
                      GLint      *winz = NULL)
{
	float in[4];
	float out[4];

	in[0]=objx;
	in[1]=objy;
	in[2]=objz;
	in[3]=1.0;
	__gluMultMatrixVecf(modelMatrix, in, out);
	__gluMultMatrixVecf(projMatrix, out, in);
	if (in[3] == 0.0) return(GL_FALSE);

	in[0] /= in[3];
	in[1] /= in[3];
	in[2] /= in[3];

	/* Map x, y and z to range 0-1 */
	in[0] = in[0] * .5f + .5f;
	in[1] = in[1] * .5f + .5f;
	in[2] = in[2] * .5f + .5f;

	/* Map x,y to viewport */
	in[0] = in[0] * (gfloat)viewport[2] + (gfloat)viewport[0];
	in[1] = in[1] * (gfloat)viewport[3] + (gfloat)viewport[1];

	*winx=GLint(in[0]);
	*winy=GLint(in[1]);

	if(winz) *winz=GLint(in[2]);

	return(GL_TRUE);
}


inline GLint
gluProjectf(gtkrtg::point3f     &point,
			const     GLfloat    finalMatrix[16],
			const     GLint      viewport[4],
                      GdkPoint  &screen )
{
	using namespace gtkrtg;

	float in[4];
	float out[4];

	in[0]=X(point);
	in[1]=Y(point);
	in[2]=Z(point);
	in[3]=1.0;
	__gluMultMatrixVecf(finalMatrix, in, out);
	if (out[3] == 0.0) return(GL_FALSE);

	X(point) = out[0];
	Y(point) = out[1];
	Z(point) = out[2];

	out[0] /= out[3];
	out[1] /= out[3];
	out[2] /= out[3];
	/* Map x, y and z to range 0-1 */
	out[0] = out[0] * .5f + .5f;
	out[1] = out[1] * .5f + .5f;
	out[2] = out[2] * .5f + .5f;

	/* Map x,y to viewport */
	out[0] = out[0] * (gfloat)viewport[2] + (gfloat)viewport[0];
	out[1] = out[1] * (gfloat)viewport[3] + (gfloat)viewport[1];

	screen.x=gint(out[0]);
	screen.y=gint(out[1]);

	return(GL_TRUE);
}


inline void
gluPerspectivef(GLfloat *m, GLfloat fovy, GLfloat aspect, GLfloat zNear, GLfloat zFar)
{
	using namespace gtkrtg;

	float sine, cotangent, deltaZ;
	float radians = fovy / 2.f * float(_PI) / 180.f;

	deltaZ = zFar - zNear;
	sine = sinf(radians);
	if ((deltaZ == 0) || (sine == 0) || (aspect == 0))
	return;

	cotangent = cosf(radians) / sine;

	__gluMakeIdentityf(m);

	m[_11] = cotangent / aspect;
	m[_22] = cotangent;
	m[_33] = -(zFar + zNear) / deltaZ;
	m[_34] = -1;
	m[_43] = -2 * zNear * zFar / deltaZ;
	m[_44] = 0;
}


static void
normalize(float v[3])
{
	float r;

	r = sqrtf( v[0]*v[0] + v[1]*v[1] + v[2]*v[2] );
	if (r == 0.0) return;

	v[0] /= r;
	v[1] /= r;
	v[2] /= r;
}


static void
cross(float v1[3], float v2[3], float result[3])
{
	result[0] = v1[1]*v2[2] - v1[2]*v2[1];
	result[1] = v1[2]*v2[0] - v1[0]*v2[2];
	result[2] = v1[0]*v2[1] - v1[1]*v2[0];
}

inline void
gluLookAtf(GLfloat *m, GLfloat eyex, GLfloat eyey, GLfloat eyez, GLfloat centerx,
		   GLfloat centery, GLfloat centerz, GLfloat upx, GLfloat upy, GLfloat upz)
{
	using namespace gtkrtg;

	float forward[3], side[3], up[3];

	forward[0] = centerx - eyex;
	forward[1] = centery - eyey;
	forward[2] = centerz - eyez;

	up[0] = upx;
	up[1] = upy;
	up[2] = upz;

	normalize(forward);

	/* Side = forward x up */
	cross(forward, up, side);
	normalize(side);

	/* Recompute up as: up = side x forward */
	cross(side, forward, up);

	__gluMakeIdentityf(m);
	m[_11] = side[0];
	m[_21] = side[1];
	m[_31] = side[2];

	m[_12] = up[0];
	m[_22] = up[1];
	m[_32] = up[2];

	m[_13] = -forward[0];
	m[_23] = -forward[1];
	m[_33] = -forward[2];

	/* translation */
	m[tx]  = -(eyex*m[_11] + eyey*m[_21] + eyez*m[_31]);
	m[ty]  = -(eyex*m[_12] + eyey*m[_22] + eyez*m[_32]);
	m[tz]  = -(eyex*m[_13] + eyey*m[_23] + eyez*m[_33]);
}

#endif /* GTK_RTG_GLU_H */
