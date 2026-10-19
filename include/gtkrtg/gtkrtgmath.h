#ifndef GTK_RTG_MATH_H
#define GTK_RTG_MATH_H

#include <cmath>
#include <gtkrtg/gtkrtgexception.h>

namespace gtkrtg
{

/* constants */
#define _PI	      (3.1415926535897932384626433832795)
#define _PI2      (1.57079632679489661923)
#define _2PI	  (6.283185307179586476925286766559)
#define _PIover2  (1.5707963267948966192313216916398)
#define _PIover4  (0.78539816339744830961566084581988)
#define _1over2PI (0.15915494309189533576888376337251)

/* converting back-and-forth from degrees to radians */
template<typename T> inline static T deg2rad(T angle){ return angle * (T(_PI) / T(180.)); }
template<typename T> inline static T rad2deg(T angle){ return angle * (T(180.) / T(_PI)); }

/* converting back-and-forth from polar to Cartesian coordinates */
template<typename T> inline static T polar_r(T x, T y){ return T(sqrt(double(x * x + y * y))); }
template<typename T> inline static T polar_theta(T x, T y){ return T(atan2(double(y), double(x))); }
template<typename T> inline static T cartesian_px(T polar_r, T polar_theta){ return polar_r * T(cos(double(polar_theta))); }
template<typename T> inline static T cartesian_py(T polar_r, T polar_theta){ return polar_r * T(sin(double(polar_theta))); }

/* converting back-and-forth from spherical to cartesian coordinates */
template<typename T> inline static T spherical_r(T x, T y, T z){ return T(sqrt(double(x * x + y * y + z * z))); }
template<typename T> inline static T spherical_theta(T x, T y, T z){ return T(atan2(sqrt(double((x * x)+(y * y))), double(z))); }
template<typename T> inline static T spherical_phi(T x, T y){ return T(atan2(double(y),double(x))); }
template<typename T> inline static T cartesian_sx(T spherical_r, T spherical_theta, T spherical_phi){ return spherical_r * T(sin(double(spherical_theta))) * T(cos(double(spherical_phi))); }
template<typename T> inline static T cartesian_sy(T spherical_r, T spherical_theta, T spherical_phi){ return spherical_r * T(sin(double(spherical_theta))) * T(sin(double(spherical_phi))); }
template<typename T> inline static T cartesian_sz(T spherical_r, T spherical_theta){ return spherical_r * T(cos(double(spherical_theta)));}

/* utils */
template<typename T> inline static T wrap_pi(T theta){ theta += T(_PI); theta -= T(floor(double(theta) * _1over2PI)) * T(_2PI); theta -= T(_PI); return theta; }
template<typename T> inline static void sin_cos(T &_sin, T &_cos, T theta){ _sin = T(sin(double(theta))); _cos = T(cos(double(theta))); }

template<typename T> inline static T safe_acos(T x)
{
	if(x <= T(-1.0)) return T(_PI);
	if(x >= T(1.0))	 return T(0.0);
	return T(acos(double(x)));
}


template<typename T, typename K>
inline T
spline_interpolate(K t, T &p0, T &p1, T &p2, T &p3)
{
	/* Catmull-Rom spline interpolation */

	K t2 = t*t;
	K t3 = t2*t;

	return ( ((p1 * K(2)) + (p0*(K(-1)) + p2) * t + (p0*K(2) - p1*K(5) + p2*K(4) - p3) * t2 + (p0*(K(-1)) + p1*K(3) - p2*K(3) + p3) * t3) * K(0.5) );
}

/* general macros */
#define MAX3(a,b,c) ((((a) > (b)) && ((a) > (c))) ? (a) : ((b) > (c)) ? (b) : (c))
#define MIN3(a,b,c) ((((a) < (b)) && ((a) < (c))) ? (a) : ((b) < (c)) ? (b) : (c))

/*
 * handy macros for accessing vector2 and vector3 value(s)
 */
enum{ _X, _Y, _Z, _W };

#define X(v)	  ((v).value[_X])
#define Y(v)	  ((v).value[_Y])
#define Z(v)	  ((v).value[_Z])
#define W(v)	  ((v).value[_W])
#define VALUES(v) ((v).value)

template<typename T>
class vector2
{
public:
	T value[2];

	vector2<T>(){ value[_X] = T(0.); value[_Y] = T(0.); }
	vector2<T>(T x, T y){ value[_X] = x; value[_Y] = y; }
	vector2<T>(T *v){ value[_X] = v[0]; value[_Y] = v[1]; }

	/* operators */
    inline vector2<T>& operator=(const vector2& vec){ value[_X] = vec.value[_X]; value[_Y] = vec.value[_Y]; return *this; }
    inline void operator-=(const vector2<T>& vec){ value[_X] -= vec.value[_X]; value[_Y] -= vec.value[_Y]; }
    inline void operator+=(const vector2<T>& vec){ value[_X] += vec.value[_X]; value[_Y] += vec.value[_Y]; }
	inline const vector2<T> operator-(const vector2<T>& vec) const { return vector2<T>(value[_X] - vec.value[_X], value[_Y] - vec.value[_Y]); }
	inline const vector2<T> operator+(const vector2<T>& vec) const { return vector2<T>(value[_X] + vec.value[_X], value[_Y] + vec.value[_Y]); }
    inline const vector2<T> operator*(const T scalar) const { return vector2<T>(value[_X] * scalar, value[_Y] * scalar); }
    inline const void operator*=(const T scalar) { value[_X] *= scalar; value[_Y] *= scalar; }
    inline const vector2<T> operator/(const T scalar) const { return vector2<T>(value[_X] / scalar, value[_Y] / scalar); }
    inline const void operator/=(const T scalar) { value[_X] /= scalar; value[_Y] /= scalar; }
    inline const bool operator==(const vector2<T>& vec) const { return (value[_X] == vec.value[_X]) && (value[_Y] == vec.value[_Y]); }
    inline const bool operator!=(const vector2<T>& vec) const { return (!(*this == vec)); }

	inline T mag() const { return sqrt(value[_X] * value[_X] + value[_Y] * value[_Y]); }
};

template<typename T>
class vector3
{
public:
	T value[3];

    vector3<T>(const vector3 &vec){ value[_X] = X(vec); value[_Y] = Y(vec); value[_Z] = Z(vec); }
	vector3<T>(){ value[_X] = T(0.); value[_Y] = T(0.); value[_Z] = T(0.); }
	vector3<T>(T x, T y, T z){ value[_X] = x; value[_Y] = y; value[_Z] = z; }
	vector3<T>(T *v){ value[_X] = v[0]; value[_Y] = v[1]; value[_Z] = v[2]; }

	/* operators */
	inline vector3<T>& operator=(const vector3<T>& vec){ value[_X] = vec.value[_X]; value[_Y] = vec.value[_Y]; value[_Z] = vec.value[_Z]; return *this; }
    inline void operator-=(const vector3<T>& vec){ value[_X] -= vec.value[_X]; value[_Y] -= vec.value[_Y]; value[_Z] -= vec.value[_Z]; }
    inline void operator+=(const vector3<T>& vec){ value[_X] += vec.value[_X]; value[_Y] += vec.value[_Y]; value[_Z] += vec.value[_Z]; }
	inline const vector3<T> operator-(const vector3<T>& vec) const { return vector3<T>(value[_X] - vec.value[_X], value[_Y] - vec.value[_Y], value[_Z] - vec.value[_Z]); }
	inline const vector3<T> operator+(const vector3<T>& vec) const { return vector3<T>(value[_X] + vec.value[_X], value[_Y] + vec.value[_Y], value[_Z] + vec.value[_Z]); }
    inline const vector3<T> operator*(const T scalar) const { return vector3<T>(value[_X] * scalar, value[_Y] * scalar, value[_Z] * scalar); }
    inline const void operator*=(const T scalar) { value[_X] *= scalar; value[_Y] *= scalar; value[_Z] *= scalar; }
    inline const vector3<T> operator/(const T scalar) const { return vector3<T>(value[_X] / scalar, value[_Y] / scalar, value[_Z] / scalar); }
    inline const void operator/=(const T scalar) { value[_X] /= scalar; value[_Y] /= scalar; value[_Z] /= scalar; }

    inline const bool operator==(const vector3<T>& vec) const { return (value[_X] == vec.value[_X]) && (value[_Y] == vec.value[_Y]) && (value[_Z] == vec.value[_Z]); }
    inline const bool operator!=(const vector3<T>& vec) const { return (!(*this == vec)); }

    /* methods */
	inline T mag() const { return T(sqrt(T(value[_X] * value[_X] + value[_Y] * value[_Y] + value[_Z] * value[_Z]))); }
    inline vector3<T>& norm(){ T inv_mag = T(1.) / mag(); value[_X] *= inv_mag; value[_Y] *= inv_mag; value[_Z] *= inv_mag; return *this; }
    inline vector3<T> abs(){ return vector3(fabs(value[_X]), fabs(value[_Y]), fabs(value[_Z])); }
	inline T dot(const vector3<T>& vec) const { return (value[_X] * vec.value[_X] + value[_Y] * vec.value[_Y] + value[_Z] * vec.value[_Z]); }
    inline vector3<T> cross(const vector3<T>& vec) const { return vector3<T>(value[_Y] * vec.value[_Z] - value[_Z] * vec.value[_Y], value[_Z] * vec.value[_X] - value[_X] * vec.value[_Z], value[_X] * vec.value[_Y] - value[_Y] * vec.value[_X] ); }
	inline vector3<T> avg(const vector3<T>& vec) const {return vector3<T>((value[_X] + vec.value[_X]) * T(.5), (value[_Y] + vec.value[_Y]) * T(.5), (value[_Z] + vec.value[_Z]) * T(.5)); }

	/* expecting normalized 'axis'! */
	inline void rot(const vector3<T>& axis, T angle_rad){ vector3<T> cross1 = cross(axis); vector3 cross2 = cross1.cross(axis); cross2 *= (T(1.) - cos(T(angle_rad))); cross1 *= sin(T(angle_rad)); *this += cross2 - cross1; }

	/* only normalized vector!*/
	inline T alpha(){ return T(_PI2) - T(acos(double(value[_X]))); }
	inline T beta(){ return T(_PI2) - T(acos(double(value[_Y]))); }
	inline T gamma(){ return T(_PI2) - T(acos(double(value[_Z]))); }
};

template<typename T, typename K> inline vector2<T> cast2(const vector2<K>& vec) { return vector2<T>(T(X(vec)), T(Y(vec)), T(Z(vec))); }
template<typename T, typename K> inline vector3<T> cast3(const vector3<K>& vec) { return vector3<T>(T(X(vec)), T(Y(vec)), T(Z(vec))); }

template<typename T>
inline vector3<T>
calc_face_normal(vector3<T> &p0, vector3<T> &p1, vector3<T> &p2)
{
	vector3<T> v0 = p1 - p0;
	vector3<T> v1 = p2 - p0;

	return v0.cross(v1);
}

template<typename T>
class quaternion
{
public:
	T value[4];

	inline void identity(){ value[_W] = T(1.0); value[_X] = value[_Y] = value[_Z] = T(0.0); }

	inline void rotate_x(T theta)
	{
		T theta_2 = theta * T(0.5);

		value[_W] = T(cos(theta_2));
		value[_X] = T(sin(theta_2));
		value[_Y] = T(0.0);
		value[_Z] = T(0.0);
	}

	inline void rotate_y(T theta)
	{
		T theta_2 = theta * T(0.5);

		value[_W] = T(cos(theta_2));
		value[_X] = T(0.0);
		value[_Y] = T(sin(theta_2));
		value[_Z] = T(0.0);
	}

	inline void rotate_z(T theta)
	{
		T theta_2 = theta * T(0.5);

		value[_W] = T(cos(theta_2));
		value[_X] = T(0.0);
		value[_Y] = T(0.0);
		value[_Z] = T(sin(theta_2));
	}

	inline void rotate_axis(const vector3<T> &axis, T theta)
	{
		/* must be normalized */
		g_throw_if_fail(T(fabs(axis.mag())) - T(1.0) < T(.01));

		T theta_2 = theta * T(0.5);
		T sin_theta_2 = T(sin(theta_2));

		value[_W] = T(cos(theta_2));
		value[_X] = X(axis) * sin_theta_2;
		value[_Y] = Y(axis) * sin_theta_2;
		value[_Z] = Z(axis) * sin_theta_2;
	}

	/* quaternion cross product, which concatenates multiple angular displacements. */
	inline const quaternion<T> operator *(const quaternion<T> &a) const
	{
		quaternion<T> result;

		W(result) = value[_W]*W(a) - value[_X]*X(a) - value[_Y]*Y(a) - value[_Z]*Z(a);
		X(result) = value[_W]*X(a) + value[_X]*W(a) + value[_Z]*Y(a) - value[_Y]*Z(a);
		Y(result) = value[_W]*Y(a) + value[_Y]*W(a) + value[_X]*Z(a) - value[_Z]*X(a);
		Z(result) = value[_W]*Z(a) + value[_Z]*W(a) + value[_Y]*X(a) - value[_X]*Y(a);

		return result;
	}

	inline const quaternion<T>& operator *=(const quaternion<T> &a)
	{
		*this = *this * a;
		return *this;
	}

	inline void	norm()
	{
		T mag = T(sqrt(value[_W]*value[_W] + value[_X]*value[_X] + value[_Y]*value[_Y] + value[_Z]*value[_Z]));

		if(mag > T(0.0))
		{
			T _1_mag = T(1.0) / mag;

			value[_W] *= _1_mag;
			value[_X] *= _1_mag;
			value[_Y] *= _1_mag;
			value[_Z] *= _1_mag;

		}else
			identity();
	}

	inline T angle() const
	{
		/* w = cos(theta / 2) */
		T theta_2 = safe_acos<T>(value[_W]);
		return theta_2 * T(2.0);
	}

	inline vector3<T> axis() const
	{
		/* compute sin^2(theta/2).  Remember that w = cos(theta/2), and sin^2(x) + cos^2(x) = 1 */
		T sinThetaOver2Sq = T(1.0) - value[_W]*value[_W];

		/* Protect against numerical imprecision */
		if (sinThetaOver2Sq <= T(0.0))
		{
			/* Identity quaternion, or numerical imprecision */
			return vector3<T>(T(1.0), T(0.0), T(0.0));
		}

		/* Compute 1 / sin(theta/2) */
		T oneOverSinThetaOver2 = T(1.0) / T(sqrt(sinThetaOver2Sq));

		/* return axis of rotation */
		return vector3<T>(value[_X]*oneOverSinThetaOver2, value[_Y]*oneOverSinThetaOver2, value[_Z]*oneOverSinThetaOver2);
	}

	inline T dot(const quaternion<T> &q) const { return value[_W]*W(q) + value[_X]*X(q) + value[_Y]*Y(q) + value[_Z]*Z(q); }

	inline quaternion<T> slerp(const quaternion<T> &q0, const quaternion<T> &q1, T t)
	{
		/* Check for out-of range parameter and return edge points if so */
		if (t <= T(0.0)) return q0;
		if (t >= T(1.0)) return q1;

		/* Compute "cosine of angle between quaternions" using dot product */
		T cosOmega = q0.dot(q1);

		/* If negative dot, use -q1. Two quaternions q and -q represent the same rotation,
		   but may produce different slerp.  We chose q or -q to rotate using the acute angle. */

		T q1w = W(q1);
		T q1x = X(q1);
		T q1y = Y(q1);
		T q1z = Z(q1);

		if(cosOmega < T(0.0))
		{
			q1w = -q1w;
			q1x = -q1x;
			q1y = -q1y;
			q1z = -q1z;
			cosOmega = -cosOmega;
		}

		/* We should have two unit quaternions, so dot should be <= 1.0 */
		g_throw_if_fail(cosOmega < T(1.1));

		// Compute interpolation fraction, checking for quaternions
		// almost exactly the same

		T k0, k1;
		if(cosOmega > T(0.9999))
		{
			// Very close - just use linear interpolation,
			// which will protect against a divide by zero

			k0 = T(1.0)-t;
			k1 = t;
		}else
		{
			// Compute the sin of the angle using the
			// trig identity sin^2(omega) + cos^2(omega) = 1

			T sinOmega = T(sqrt(T(1.0) - cosOmega*cosOmega));

			// Compute the angle from its sin and cosine
			T omega = T(atan2(sinOmega, cosOmega));

			// Compute inverse of denominator, so we only have
			// to divide once
			T oneOverSinOmega = T(1.0) / sinOmega;

			// Compute interpolation parameters
			k0 = sin((T(1.0) - t) * omega) * oneOverSinOmega;
			k1 = sin(t * omega) * oneOverSinOmega;
		}

		// Interpolate
		quaternion<T> result;
		X(result) = k0*X(q0) + k1*q1x;
		Y(result) = k0*Y(q0) + k1*q1y;
		Z(result) = k0*Z(q0) + k1*q1z;
		W(result) = k0*W(q0) + k1*q1w;

		return result;
	}

	/* Compute the quaternion conjugate.  This is the quaternion with the opposite
	   rotation as the original quaternion. */
	inline void conjugate()
	{
		/* opposite axis of rotation */
		value[_X] *= -1.f;
		value[_Y] *= -1.f;
		value[_Z] *= -1.f;
	}

	inline quaternion<T> pow(const quaternion<T> &q, T exponent)
	{
		/* check for the case of an identity quaternion.
		   this will protect against divide by zero */

		if(fabs(W(q)) > T(0.9999))
			return q;

		/* extract the half angle alpha (alpha = theta/2) */
		T alpha = T(acos(W(q)));

		/* compute new alpha value */
		T newAlpha = alpha * exponent;

		/* Compute new w value */
		quaternion<T> result;
		W(result) = T(cos(newAlpha));

		/* Compute new xyz values */
		T mult = T(sin(newAlpha)) / T(sin(alpha));
		X(result) = X(q) * mult;
		Y(result) = Y(q) * mult;
		Z(result) = Z(q) * mult;

		return result;
	}
};

/* row vector indices (matrix3x3) */
enum{ _11_, _12_, _13_,
      _21_, _22_, _23_,
      _31_, _32_, _33_, mn_ };

template<typename T>
class matrix3x3
{
public:
	T m[mn_];

	matrix3x3(){ identity(); }

	inline void identity()
	{
		m[_11_]=T(1.0); m[_21_]=T(0.0); m[_31_]=T(0.0);
		m[_12_]=T(0.0); m[_22_]=T(1.0); m[_32_]=T(0.0);
		m[_13_]=T(0.0); m[_23_]=T(0.0); m[_33_]=T(1.0);
	}

	inline T& operator[](unsigned int i) { return m[i]; }

	inline void from_inertial2object(quaternion<T> &q)
	{
		m[_11_] = T(1.0) - T(2.0) * (Y(q) * Y(q) + Z(q) * Z(q));
		m[_12_] = T(2.0) * (X(q) * Y(q) + Z(q) * W(q));
		m[_13_] = T(2.0) * (Z(q) * X(q) - Y(q) * W(q));

		m[_21_] = T(2.0) * (X(q) * Y(q) - Z(q) * W(q));
		m[_22_] = T(1.0) - T(2.0) * (Z(q) * Z(q) + X(q) * X(q));
		m[_23_] = T(2.0) * (Y(q) * Z(q) + X(q) * W(q));

		m[_31_] = T(2.0) * (Z(q) * X(q) + Y(q) * W(q));
		m[_32_] = T(2.0) * (Y(q) * Z(q) - X(q) * W(q));
		m[_33_] = T(1.0) - T(2.0) * (Y(q) * Y(q) + X(q) * X(q));
	}

	inline void from_object2inertial(quaternion<T> &q)
	{
		m[_11_] = T(1.0) - T(2.0) * (Y(q) * Y(q) + Z(q) * Z(q));
		m[_12_] = T(2.0) * (X(q) * Y(q) - Z(q) * W(q));
		m[_13_] = T(2.0) * (Z(q) * X(q) + Y(q) * W(q));

		m[_21_] = T(2.0) * (X(q) * Y(q) + Z(q) * W(q));
		m[_22_] = T(1.0) - T(2.0) * (Z(q) * Z(q) + X(q) * X(q));
		m[_23_] = T(2.0) * (Y(q) * Z(q) - X(q) * W(q));

		m[_31_] = T(2.0) * (Z(q) * X(q) - Y(q) * W(q));
		m[_32_] = T(2.0) * (Y(q) * Z(q) + X(q) * W(q));
		m[_33_] = T(1.0) - T(2.0) * (Y(q) * Y(q) + X(q) * X(q));
	}

	inline vector3<T> inertial2object(const vector3<T> &v)
	{
		return vector3<T>( m[_11_]*X(v) + m[_21_]*Y(v) + m[_31_]*Z(v),
				   	   	   m[_12_]*X(v) + m[_22_]*Y(v) + m[_32_]*Z(v),
				   	   	   m[_13_]*X(v) + m[_23_]*Y(v) + m[_33_]*Z(v) );
	}

	inline vector3<T> object2inertial(const vector3<T> &v)
	{
		return vector3<T>( m[_11_]*X(v) + m[_12_]*Y(v) + m[_13_]*Z(v),
				   	   	   m[_21_]*X(v) + m[_22_]*Y(v) + m[_23_]*Z(v),
				   	   	   m[_31_]*X(v) + m[_32_]*Y(v) + m[_33_]*Z(v) );
	}

	inline quaternion<T> to_quaternion()
	{
		T fourWSquaredMinus1 = m[_11_] + m[_22_] + m[_33_];
		T fourXSquaredMinus1 = m[_11_] - m[_22_] - m[_33_];
		T fourYSquaredMinus1 = m[_22_] - m[_11_] - m[_33_];
		T fourZSquaredMinus1 = m[_33_] - m[_11_] - m[_22_];

		int biggestIndex = 0;

		T fourBiggestSquaredMinus1 = fourWSquaredMinus1;
		if(fourXSquaredMinus1 > fourBiggestSquaredMinus1)
		{
			fourBiggestSquaredMinus1 = fourXSquaredMinus1;
			biggestIndex = 1;
		}

		if(fourYSquaredMinus1 > fourBiggestSquaredMinus1)
		{
			fourBiggestSquaredMinus1 = fourYSquaredMinus1;
			biggestIndex = 2;
		}

		if(fourZSquaredMinus1 > fourBiggestSquaredMinus1)
		{
			fourBiggestSquaredMinus1 = fourZSquaredMinus1;
			biggestIndex = 3;
		}

		T biggestVal = T(sqrt(double(fourBiggestSquaredMinus1) + 1.0)) * T(0.5);
		T mult = T(0.25) / biggestVal;

		quaternion<T> q;

		switch(biggestIndex)
		{
			case 0: W(q) = biggestVal;
					X(q) = (m[_23_] - m[_32_]) * mult;
					Y(q) = (m[_31_] - m[_13_]) * mult;
					Z(q) = (m[_12_] - m[_21_]) * mult;
					break;

			case 1: X(q) = biggestVal;
					W(q) = (m[_23_] - m[_32_]) * mult;
					Y(q) = (m[_12_] + m[_21_]) * mult;
					Z(q) = (m[_31_] + m[_13_]) * mult;
					break;

			case 2: Y(q) = biggestVal;
					W(q) = (m[_31_] - m[_13_]) * mult;
					X(q) = (m[_12_] + m[_21_]) * mult;
					Z(q) = (m[_23_] + m[_32_]) * mult;
					break;

			case 3: Z(q) = biggestVal;
					W(q) = (m[_12_] - m[_21_]) * mult;
					X(q) = (m[_31_] + m[_13_]) * mult;
					Y(q) = (m[_23_] + m[_32_]) * mult;
					break;
		}

		return q;
	}
};

/* row vector indices (matrix4x4) */
enum{ _11, _12, _13, _14,
      _21, _22, _23, _24,
      _31, _32, _33, _34,
      _41, _42, _43, _44, mn };

/* translation part (matrix4x4) */
enum{ tx = _41,
      ty = _42,
      tz = _43,
      tw = _44 };

template<typename T>
class matrix4x4
{
public:
	T m[mn];

	matrix4x4<T>(){ identity(); }

	inline void identity()
	{
		m[_11]=T(1.0); m[_21]=T(0.0); m[_31]=T(0.0); m[tx]=T(0.0);
		m[_12]=T(0.0); m[_22]=T(1.0); m[_32]=T(0.0); m[ty]=T(0.0);
		m[_13]=T(0.0); m[_23]=T(0.0); m[_33]=T(1.0); m[tz]=T(0.0);
		m[_14]=T(0.0); m[_24]=T(0.0); m[_34]=T(0.0); m[tw]=T(1.0);
	}
	inline const vector3<T> operator*(const vector3<T> &p) const
	{
		return vector3<T>(X(p)*m[_11] + Y(p)*m[_21] + Z(p)*m[_31] + m[tx],
			          	  X(p)*m[_12] + Y(p)*m[_22] + Z(p)*m[_32] + m[ty],
			          	  X(p)*m[_13] + Y(p)*m[_23] + Z(p)*m[_33] + m[tz]);
	}

	inline vector3<T>& operator*=(vector3<T> &p){ p = p*m; return p; }
	inline T& operator[](unsigned int i) { return m[i]; }

	inline const matrix4x4<T> operator*(matrix4x4<T> &a) const
	{
		matrix4x4<T> r;
		int i, j;

		for (i = 0; i < 4; i++)
			for (j = 0; j < 4; j++)
				r[i*4+j] = m[i*4+0]*a[0*4+j] +
					   m[i*4+1]*a[1*4+j] +
					   m[i*4+2]*a[2*4+j] +
					   m[i*4+3]*a[3*4+j];
		return r;
	}

	inline matrix4x4<T>& operator*=(const matrix4x4<T> &a) const { this = this * a; return this; }

	inline T determinant()
	{
		return m[_11] * (m[_22]*m[_33] - m[_23]*m[_32])
			 + m[_12] * (m[_23]*m[_31] - m[_21]*m[_33])
			 + m[_13] * (m[_21]*m[_32] - m[_22]*m[_31]);
	}

	inline matrix4x4<T> inverse() throw (GtkRtgException)
	{
		T det = determinant();

		g_throw_if_fail(T(fabs(det)) > T(0.000001));

		T det_1 = T(1.0) / det;

		matrix4x4<T> r;

		r[_11] = (m[_22]*m[_33] - m[_23]*m[_32]) * det_1;
		r[_12] = (m[_13]*m[_32] - m[_12]*m[_33]) * det_1;
		r[_13] = (m[_12]*m[_23] - m[_13]*m[_22]) * det_1;

		r[_21] = (m[_23]*m[_31] - m[_21]*m[_33]) * det_1;
		r[_22] = (m[_11]*m[_33] - m[_13]*m[_31]) * det_1;
		r[_23] = (m[_13]*m[_21] - m[_11]*m[_23]) * det_1;

		r[_31] = (m[_21]*m[_32] - m[_22]*m[_31]) * det_1;
		r[_32] = (m[_12]*m[_31] - m[_11]*m[_32]) * det_1;
		r[_33] = (m[_11]*m[_22] - m[_12]*m[_21]) * det_1;


		r[tx] = -(m[tx]*r[_11] + m[ty]*r[_21] + m[tz]*r[_31]);
		r[ty] = -(m[tx]*r[_12] + m[ty]*r[_22] + m[tz]*r[_32]);
		r[tz] = -(m[tx]*r[_13] + m[ty]*r[_23] + m[tz]*r[_33]);

		return r;
	}

	inline matrix4x4<T> transpose() throw (GtkRtgException)
	{
		matrix4x4<T> r;

		r[_11] = m[_11]; r[_12] = m[_21]; r[_13] = m[_31];
		r[_21] = m[_12]; r[_22] = m[_22]; r[_23] = m[_32];
		r[_31] = m[_13]; r[_32] = m[_23]; r[_33] = m[_33];

		r[tx] = r[ty] = r[tz] = 0.f;
		r[tw] = 1.f;

		return r;
	}

	inline void set_translation(const vector3<T> &v){ m[tx] = X(v); m[ty] = Y(v); m[tz] = Z(v); }
	inline const vector3<T> get_translation() const { return vector3<T>(m[tx], m[ty], m[tz]); }
	inline void setup_translation(const vector3<T> &v)
	{
		m[_11]=T(1.0); m[_21]=T(0.0); m[_31]=T(0.0); m[tx]=X(v);
		m[_12]=T(0.0); m[_22]=T(1.0); m[_32]=T(0.0); m[ty]=Y(v);
		m[_13]=T(0.0); m[_23]=T(0.0); m[_33]=T(1.0); m[tz]=Z(v);
		m[_14]=T(0.0); m[_24]=T(0.0); m[_34]=T(0.0); m[tw]=T(1.0);
	}

	inline void setup_rotation(const vector3<T> &axis, const T theta) throw (GtkRtgException)
	{
		/* axis must be a unit vector (normalized) */
		g_throw_if_fail(fabs(axis.dot(axis) - T(1.0)) < T(.01));

		T s,c; sin_cos(s, c, theta);
		T a=1.0f - c; T ax=a*X(axis); T ay=a*Y(axis); T az=a*Z(axis);

		m[_11]=ax*X(axis) + c;	       m[_21]=ay*X(axis) - Z(axis)*s; m[_31]=az*X(axis) + Y(axis)*s; m[tx]=T(0.0);
		m[_12]=ax*Y(axis) + Z(axis)*s; m[_22]=ay*Y(axis) + c;         m[_32]=az*Y(axis) - X(axis)*s; m[ty]=T(0.0);
		m[_13]=ax*Z(axis) - Y(axis)*s; m[_23]=ay*Z(axis) + X(axis)*s; m[_33]=az*Z(axis) + c;	     m[tz]=T(0.0);
	}

	inline void setup_scale(const vector3<T> &v)
	{
		m[_11]=X(v);   m[_21]=T(0.0); m[_31]=T(0.0); m[tx]=T(0.0);
		m[_12]=T(0.0); m[_22]=Y(v);   m[_32]=T(0.0); m[ty]=T(0.0);
		m[_13]=T(0.0); m[_23]=T(0.0); m[_33]=Z(v);   m[tz]=T(0.0);
		m[_14]=T(0.0); m[_24]=T(0.0); m[_34]=T(0.0); m[tw]=T(1.0);
	}

	inline void from_quaternion(const quaternion<T> &q, bool clear_trans = false)
	{
		T ww = T(2.0) * W(q);
		T xx = T(2.0) * X(q);
		T yy = T(2.0) * Y(q);
		T zz = T(2.0) * Z(q);

		m[_11] = T(1.0) - yy*Y(q) - zz*Z(q);
		m[_12] = xx*Y(q) + ww*Z(q);
		m[_13] = xx*Z(q) - ww*Y(q);

		m[_21] = xx*Y(q) - ww*Z(q);
		m[_22] = T(1.0) - xx*X(q) - zz*Z(q);
		m[_23] = yy*Z(q) + ww*X(q);

		m[_31] = xx*Z(q) + ww*Y(q);
		m[_32] = yy*Z(q) - ww*X(q);
		m[_33] = T(1.0) - xx*X(q) - yy*Y(q);

		if(clear_trans) m[tx] = m[ty] = m[tz] = T(0.0);
	}

	inline void local2parent(const vector3<T> &pos, matrix3x3<T> &orient)
	{
		/* Copy the rotation portion of the matrix.
		   The rotation matrix is "normally" an inertial->object matrix, which is
		   parent->local.  We want a local->parent rotation, so we
		   must transpose while copying. */

		m[_11] = orient[_11_]; m[_12] = orient[_21_]; m[_13] = orient[_31_];
		m[_21] = orient[_12_]; m[_22] = orient[_22_]; m[_23] = orient[_32_];
		m[_31] = orient[_13_]; m[_32] = orient[_23_]; m[_33] = orient[_33_];

		/* Now set the translation portion.  Translation happens "after"
		   the 3x3 portion, so we can simply copy the position
		   field directly. */

		m[tx] = X(pos); m[ty] = Y(pos); m[tz] = Z(pos);
	}

	inline void parent2local(const vector3<T> &pos, matrix3x3<T> &orient)
	{
		/* Copy the rotation portion of the matrix.  We can copy the
		   elements directly (without transposing) */

		m[_11] = orient[_11_]; m[_12] = orient[_12_]; m[_13] = orient[_13_];
		m[_21] = orient[_21_]; m[_22] = orient[_22_]; m[_23] = orient[_23_];
		m[_31] = orient[_31_]; m[_32] = orient[_32_]; m[_33] = orient[_33_];

		/* Now set the translation portion.  Normally, we would
		   translate by the negative of the position to translate
		   from world to inertial space.  However, we must correct
		   for the fact that the rotation occurs "first."  So we
		   must rotate the translation portion.  This is the same
		   as create a translation matrix T to translate by -pos,
		   and a rotation matrix R, and then creating the matrix
		   as the concatenation of TR. */

		m[tx] = -(X(pos)*m[_11] + Y(pos)*m[_21] + Z(pos)*m[_31]);
		m[ty] = -(X(pos)*m[_12] + Y(pos)*m[_22] + Z(pos)*m[_32]);
		m[tz] = -(X(pos)*m[_13] + Y(pos)*m[_23] + Z(pos)*m[_33]);
	}


	inline quaternion<T> to_quaternion()
	{
		T fourWSquaredMinus1 = m[_11] + m[_22] + m[_33];
		T fourXSquaredMinus1 = m[_11] - m[_22] - m[_33];
		T fourYSquaredMinus1 = m[_22] - m[_11] - m[_33];
		T fourZSquaredMinus1 = m[_33] - m[_11] - m[_22];

		int biggestIndex = 0;

		T fourBiggestSquaredMinus1 = fourWSquaredMinus1;
		if(fourXSquaredMinus1 > fourBiggestSquaredMinus1)
		{
			fourBiggestSquaredMinus1 = fourXSquaredMinus1;
			biggestIndex = 1;
		}

		if(fourYSquaredMinus1 > fourBiggestSquaredMinus1)
		{
			fourBiggestSquaredMinus1 = fourYSquaredMinus1;
			biggestIndex = 2;
		}

		if (fourZSquaredMinus1 > fourBiggestSquaredMinus1)
		{
			fourBiggestSquaredMinus1 = fourZSquaredMinus1;
			biggestIndex = 3;
		}

		T biggestVal = T(sqrt(double(fourBiggestSquaredMinus1) + 1.0)) * T(0.5);
		T mult       = T(0.25) / biggestVal;

		quaternion<T> q;

		switch (biggestIndex)
		{
			case 0: W(q) = biggestVal;
					X(q) = (m[_23] - m[_32]) * mult;
					Y(q) = (m[_31] - m[_13]) * mult;
					Z(q) = (m[_12] - m[_21]) * mult;
					break;

			case 1: X(q) = biggestVal;
					W(q) = (m[_23] - m[_32]) * mult;
					Y(q) = (m[_12] + m[_21]) * mult;
					Z(q) = (m[_31] + m[_13]) * mult;
					break;

			case 2: Y(q) = biggestVal;
					W(q) = (m[_31] - m[_13]) * mult;
					X(q) = (m[_12] + m[_21]) * mult;
					Z(q) = (m[_23] + m[_32]) * mult;
					break;

			case 3: Z(q) = biggestVal;
					W(q) = (m[_12] - m[_21]) * mult;
					X(q) = (m[_31] + m[_13]) * mult;
					Y(q) = (m[_23] + m[_32]) * mult;
					break;
		}

		return q;
	}


	inline vector3<T>	get_pos_from_parent2local()
	{
		/* Multiply negative translation value by the
		   transpose of the 3x3 portion.  By using the transpose,
		   we assume that the matrix is orthogonal.  (This function
		   doesn't really make sense for non-rigid transformations...) */

		return vector3<T>( -(m[tx]*m[_11] + m[ty]*m[_12] + m[tz]*m[_13]),
				   	   	   -(m[tx]*m[_21] + m[ty]*m[_22] + m[tz]*m[_23]),
				   	   	   -(m[tx]*m[_31] + m[ty]*m[_32] + m[tz]*m[_33]) );
	}

	inline vector3<T> get_pos_from_local2parent()
	{
		return vector3<T>(m[tx], m[ty], m[tz]);
	}
};

template<typename T, typename K>
inline matrix4x4<T>
cast4x4(matrix4x4<K> &n)
{
	matrix4x4<T> m;

	m[_11]=T(n[_11]); m[_21]=T(n[_21]); m[_31]=T(n[_31]); m[tx]=T(n[tx]);
	m[_12]=T(n[_12]); m[_22]=T(n[_22]); m[_32]=T(n[_32]); m[ty]=T(n[ty]);
	m[_13]=T(n[_13]); m[_23]=T(n[_23]); m[_33]=T(n[_33]); m[tz]=T(n[tz]);
	m[_14]=T(n[_14]); m[_24]=T(n[_24]); m[_34]=T(n[_34]); m[tw]=T(n[tw]);

	return m;
}

template<typename T>
class plane
{
public:
        vector3<T> normal;
        T d;
};

/* custom types */
typedef vector2<float>     vector2f;
typedef vector2<double>	   vector2d;
typedef vector3<float>     vector3f;
typedef vector3<double>    vector3d;
typedef matrix3x3<float>   matrix3x3f;
typedef matrix3x3<double>  matrix3x3d;
typedef matrix4x4<float>   matrix4x4f;
typedef matrix4x4<double>  matrix4x4d;
typedef quaternion<float>  quat_f;
typedef quaternion<double> quat_d;
typedef plane<float>       plane_f;
typedef plane<double>      plane_d;

/* convenient typedefs */
typedef vector3f point3f;
typedef vector3d point3d;
typedef vector2f point2f;
typedef vector2d point2d;

} /* namespace gtkrtg */

#endif /* GTK_RTG_MATH_H */
