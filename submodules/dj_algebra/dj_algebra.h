/* dj_algebra.h - public domain algebra library
by Jonathan Dupuy

   Do this:
      #define DJ_ALGEBRA_IMPLEMENTATION 1
   before you include this file in *one* C++ file to create the implementation.

   INTERFACING

   define DJA_ASSERT(x) to avoid using assert.h.

   QUICK NOTES

*/

#ifndef DJA_INCLUDE_DJ_ALGEBRA_H
#define DJA_INCLUDE_DJ_ALGEBRA_H

namespace dja {

/* Floating point precision */
#if DJA_USE_DOUBLE_PRECISION
typedef double float_t;
#else
typedef float float_t;
#endif

/* Temporary Macros */
#define OP operator
#define V1 float_t
#define V2 vec2
#define V3 vec3
#define V4 vec4
#define M2 mat2
#define M3 mat3
#define M4 mat4
#define Z complex
#define Q quaternion

/* Forward Declarations */
struct vec2;
struct vec3;
struct vec4;
struct mat2;
struct mat3;
struct mat4;
struct complex;
struct quaternion;

// *****************************************************************************
/* vec2 API */
struct vec2 {
	vec2(V1 x, V1 y): x(x), y(y) {}
	explicit vec2(V1 x = V1(0)): x(x), y(x) {}
	explicit vec2(const Z& z);
	static vec2 memcpy(const V1 *v) {return V2(v[0], v[1]);}
	V1& operator[](int i) {return (&x)[i];}
	const V1& operator[](int i) const {return (&x)[i];}
	V1 x, y;
};
V2 OP*(const V1 a, const V2& b) {return V2(a * b.x, a * b.y);}
V2 OP*(const V2& a, const V1 b) {return V2(b * a.x, b * a.y);}
V2 OP/(const V2& a, const V1 b) {return (V1(1) / b) * a;}
V2 OP*(const V2& a, const V2& b) {return V2(a.x * b.x, a.y * b.y);}
V2 OP/(const V2& a, const V2& b) {return V2(a.x / b.x, a.y / b.y);}
V2 OP+(const V2& a, const V2& b) {return V2(a.x + b.x, a.y + b.y);}
V2 OP-(const V2& a, const V2& b) {return V2(a.x - b.x, a.y - b.y);}
V2 OP+(const V2& a) {return V2(+a.x, +a.y);}
V2 OP-(const V2& a) {return V2(-a.x, -a.y);}
V2& OP+=(V2& a, const V2& b) {a.x+= b.x; a.y+= b.y; return a;}
V2& OP-=(V2& a, const V2& b) {a.x-= b.x; a.y-= b.y; return a;}
V2& OP*=(V2& a, const V2& b) {a.x*= b.x; a.y*= b.y; return a;}
V2& OP*=(V2& a, V1 b) {a.x*= b; a.y*= b; return a;}
V2& OP/=(V2& a, const V2& b) {a.x/= b.x; a.y/= b.y; return a;}
V2& OP/=(V2& a, V1 b) {a*= V1(1) / b; return a;}
V1 dot(const V2& a, const V2& b) {return a.x * b.x + a.y * b.y;}
V1 norm(const V2& a) {return sqrt(dot(a, a));}
V2 normalize(const V2& a) {return a / norm(a);}
V2 reflect(const V2& a, const V2& n) {return a - V1(2) * dot(a, n) * n;}
V2 lerp(const V1 u, const V2& a, const V2& b) {return a + u * (b - a);}

// *****************************************************************************
/* vec3 API */
struct vec3 {
	vec3(V1 x, V1 y, V1 z): x(x), y(y), z(z) {}
	explicit vec3(V1 x = V1(0)): x(x), y(x), z(x) {}
	explicit vec3(const Q& q);
	static vec3 memcpy(const V1 *v) {return V3(v[0], v[1], v[2]);}
	V1& operator[](int i) {return (&x)[i];}
	const V1& operator[](int i) const {return (&x)[i];}
	V1 x, y, z;
};
V3 OP*(const V1 a, const V3& b) {return V3(a * b.x, a * b.y, a * b.z);}
V3 OP*(const V3& a, const V1 b) {return V3(b * a.x, b * a.y, b * a.z);}
V3 OP/(const V3& a, const V1 b) {return (V1(1) / b) * a;}
V3 OP*(const V3& a, const V3& b) {return V3(a.x * b.x, a.y * b.y, a.z * b.z);}
V3 OP/(const V3& a, const V3& b) {return V3(a.x / b.x, a.y / b.y, a.z / b.z);}
V3 OP+(const V3& a, const V3& b) {return V3(a.x + b.x, a.y + b.y, a.z + b.z);}
V3 OP-(const V3& a, const V3& b) {return V3(a.x - b.x, a.y - b.y, a.z - b.z);}
V3 OP+(const V3& a) {return V3(+a.x, +a.y, +a.z);}
V3 OP-(const V3& a) {return V3(-a.x, -a.y, -a.z);}
V3& OP+=(V3& a, const V3& b) {a.x+= b.x; a.y+= b.y; a.z+= b.z; return a;}
V3& OP-=(V3& a, const V3& b) {a.x-= b.x; a.y-= b.y; a.z-= b.z; return a;}
V3& OP*=(V3& a, const V3& b) {a.x*= b.x; a.y*= b.y; a.z*= b.z; return a;}
V3& OP*=(V3& a, const V1 b) {a.x*= b; a.y*= b; a.z*= b; return a;}
V3& OP/=(V3& a, const V3& b) {a.x/= b.x; a.y/= b.y; a.z/= b.z; return a;}
V3& OP/=(V3& a, const V1 b) {a*= V1(1) / b; return a;}
V1 dot(const V3& a, const V3& b) {return a.x * b.x + a.y * b.y + a.z * b.z;}
V1 norm(const V3& a) {return sqrt(dot(a, a));}
V3 normalize(const V3& a) {return a / norm(a);}
V3 reflect(const V3& a, const V3& n) {return a - V1(2) * dot(a, n) * n;}
V3 lerp(const V1 u, const V3& a, const V3& b) {return a + u * (b - a);}
V3 cross(const V3& a, const V3& b) {
	return vec3(a.y * b.z - a.z * b.y,
	            a.z * b.x - a.x * b.z,
	            a.x * b.y - a.y * b.x);
}
V3 rotate(const V3& r, const V3& axis, const V1 rad) {
	V1 c = cos(rad), s = sin(rad);
	V3 r1 = axis * dot(r, axis);
	V3 r2 = r - r1;
	V3 r3 = cross(r2, axis);

	return c * r2 - s * r3 + r1;
}

// *****************************************************************************
/* vec4 API */
struct vec4 {
	vec4(V1 x, V1 y, V1 z, V1 w): x(x), y(y), z(z), w(w) {}
	explicit vec4(V1 x = V1(0)): x(x), y(x), z(x), w(x) {}
	explicit vec4(const Q& q);
	static vec4 memcpy(const V1 *v) {return V4(v[0], v[1], v[2], v[3]);}
	V1& operator[](int i) {return (&x)[i];}
	const V1& operator[](int i) const {return (&x)[i];}
	V1 x, y, z, w;
};
V4 OP*(const V1 a, const V4& b) {return V4(a * b.x, a * b.y, a * b.z, a * b.w);}
V4 OP*(const V4& a, const V1 b) {return V4(b * a.x, b * a.y, b * a.z, b * a.w);}
V4 OP/(const V4& a, const V1 b) {return (V1(1) / b) * a;}
V4 OP*(const V4& a, const V4& b) {return V4(a.x * b.x, a.y * b.y, a.z * b.z, a.w * b.w);}
V4 OP/(const V4& a, const V4& b) {return V4(a.x / b.x, a.y / b.y, a.z / b.z, a.w / b.w);}
V4 OP+(const V4& a, const V4& b) {return V4(a.x + b.x, a.y + b.y, a.z + b.z, a.w * b.w);}
V4 OP-(const V4& a, const V4& b) {return V4(a.x - b.x, a.y - b.y, a.z - b.z, a.w * b.w);}
V4 OP+(const V4& a) {return V4(+a.x, +a.y, +a.z, +a.w);}
V4 OP-(const V4& a) {return V4(-a.x, -a.y, -a.z, -a.w);}
V4& OP+=(V4& a, const V4& b) {a.x+= b.x; a.y+= b.y; a.z+= b.z; a.w+= b.w; return a;}
V4& OP-=(V4& a, const V4& b) {a.x-= b.x; a.y-= b.y; a.z-= b.z; a.w-= b.w; return a;}
V4& OP*=(V4& a, const V4& b) {a.x*= b.x; a.y*= b.y; a.z*= b.z; a.w*= b.w; return a;}
V4& OP/=(V4& a, const V4& b) {a.x/= b.x; a.y/= b.y; a.z/= b.z; a.w/= b.w; return a;}
V4& OP*=(V4& a, const V1 b) {a.x*= b; a.y*= b; a.z*= b; a.w*= b; return a;}
V4& OP/=(V4& a, const V1 b) {a*= V1(1) / b; return a;}
V1 dph(const V4& a, const V4& b) {return a.x * b.x + a.y * b.y + a.z * b.z;}
V1 dot(const V4& a, const V4& b) {return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;}
V1 norm(const V4& a) {return sqrt(dot(a, a));}
V4 normalize(const V4& a) {return a / norm(a);}
V4 lerp(const V1 u, const V4& a, const V4& b) {return a + u * (b - a);}

// *****************************************************************************
/* mat2x2 API */
struct mat2 {
	mat2(V1 m00, V1 m01, V1 m10, V1 m11);
	mat2(const V2& m0, const V2& m1);
	explicit mat2(V1 diag = V1(1));
	explicit mat2(const Z& z);
	static mat2 memcpy(const V1 *v, bool rowmajor = true);
	static mat2 rotation(V1 rad);
	static mat2 scale(V1 value);
	static mat2 scale(const V2& values);
	V2& operator[](int i) {return m[i];}
	const V2& operator[](int i) const {return m[i];}
	private: V2 m[2];
};
V1 determinant(const M2& m);
M2 transpose(const M2& m);
M2 adjugate(const M2& m);
M2 inverse(const M2& m);
V2 OP*(const M2& m, const V2& a);
M2 OP*(const V1 s, const M2& m);
M2 OP*(const M2& a, const M2& b);

// *****************************************************************************
/* mat3x3 API */
struct mat3 {
	mat3(V1 m00, V1 m01, V1 m02,
	     V1 m10, V1 m11, V1 m12,
	     V1 m20, V1 m21, V1 m22);
	mat3(const V3& m0, const V3& m1, const V3& m2);
	explicit mat3(V1 diag = V1(1));
	static mat3 memcpy(const V1 *v, bool rowmajor = true);
	static mat3 rotation(const V3& axis, V1 rad);
	static mat3 rotation(const Q& quaternion);
	static mat3 scale(V1 value);
	static mat3 scale(const V3& values);
	V3& operator[](int i) {return m[i];}
	const V3& operator[](int i) const {return m[i];}
	private: V3 m[3];
};
V1 determinant(const M3& m);
M3 transpose(const M3& m);
M3 adjugate(const M3& m);
M3 inverse(const M3& m);
V3 OP*(const M3& m, const V3& a);
M3 OP*(const V1 s, const M3& m);
M3 OP*(const M3& a, const M3& b);

// *****************************************************************************
/* mat4x4 API */
struct mat4 {
	mat4(V1 m00, V1 m01, V1 m02, V1 m03,
	     V1 m10, V1 m11, V1 m12, V1 m13,
	     V1 m20, V1 m21, V1 m22, V1 m23,
	     V1 m30, V1 m31, V1 m32, V1 m33);
	mat4(const V4& m0, const V4& m1, const V4& m2, const V4& m3);
	explicit mat4(V1 diag = V1(1));
	static mat4 memcpy(const V1 *v, bool rowmajor = true);
	struct homogeneous {
		static mat4 from_mat3(const M3& m);
		static mat4 rotation(const V3& axis, V1 rad);
		static mat4 rotation(const Q& quaternion);
		static mat4 translation(const V3& dir);
		static mat4 scale(V1 value);
		static mat4 scale(const V3& values);
		static mat4 perspective(V1 fovy, V1 aspect, V1 zNear, V1 zFar);
		static mat4 orthographic(V1 left, V1 right,
		                         V1 bottom, V1 top,
		                         V1 near, V1 far);
		static mat4 tile(V1 left, V1 right, V1 bottom, V1 top);
	};
	V4& operator[](int i) {return m[i];}
	const V4& operator[](int i) const {return m[i];}
	private: V4 m[4];
};
V1 determinant(const M4& m);
M4 transpose(const M4& m);
M4 adjugate(const M4& m);
M4 inverse(const M4& m);
V4 OP*(const M4& m, const V4& a);
M4 OP*(const M4& a, const M4& b);
M4 OP*(const V1 s, const M4& m);

// *****************************************************************************
/* complex API */
struct complex {
	explicit complex(V1 re = 0, V1 im = 0): re(re), im(im) {}
	explicit complex(const V2& v): re(v.x), im(v.y) {}
	static complex memcpy(const V1 *v) {return complex(v[0], v[1]);}
	static complex polar(V1 angle, V1 norm) {
		return complex(norm * cos(angle), norm * sin(angle));
	}
	V1& operator[](int i) {return (&re)[i];}
	const V1& operator[](int i) const {return (&re)[i];}
	V1 re, im;
};
Z bar(const Z& z) {return Z(z.re, -z.im);}
V1 dot(const Z& a, const Z& b) {return dot(V2(a), V2(b));}
Z OP*(const V1 a, const Z& b) {return Z(a * b.re, a * b.im);}
Z OP*(const Z& a, const V1 b) {return b * a;}
Z OP*(const Z& a, const Z& b) {
	return Z(a.re * b.re - a.im * b.im, a.im * b.re + a.re * b.im);
}
Z OP/(const Z& a, const V1 b) {return (V1(1) / b) * a;}
Z OP/(const Z& a, const Z& b) {return (a * bar(b)) / dot(b, b);}
Z OP/(const V1 a, const Z& b) {return Z(a) / b;}
Z OP+(const Z& a, const Z& b) {return Z(V2(a) + V2(b));}
Z OP-(const Z& a, const Z& b) {return Z(V2(a) - V2(b));}
Z OP+(const Z& a) {return Z(+a.re, +a.im);}
Z OP-(const Z& a) {return Z(-a.re, -a.im);}
Z& OP+=(Z& a, const Z& b) {a.re+= b.re; a.im+= b.im; return a;}
Z& OP+=(Z& a, const V1& b) {a.re+= b; return a;}
Z& OP-=(Z& a, const Z& b) {a.re-= b.re; a.im-= b.im; return a;}
Z& OP-=(Z& a, const V1& b) {a.re-= b; return a;}
Z& OP*=(Z& a, const Z& b) {a = a * b; return a;}
Z& OP*=(Z& a, const V1& b) {a = a * b; return a;}
V1 norm(const Z& a) {return sqrt(dot(a, a));}
V1 angle(const Z& z) {return atan2(z.im, z.re);}
Z normalize(const Z& z) {return Z(normalize(V2(z)));}
Z reflect(const Z& z, const Z& n) {return -n * bar(z) * n;}
Z lerp(const V1 u, const Z& a, const Z& b) {return a + u * (b - a);}
V2::V2(const Z& z): x(z.re), y(z.im) {}
M2::M2(const Z& z) {m[0] = V2(z.re, z.im); m[1] = V2(-z.im, z.re);}
Z zeta(const Z& z); // Riemann Zeta Function

// *****************************************************************************
/* Standalone quaternion API */
struct quaternion {
	quaternion(V1 re, V1 i, V1 j, V1 k): re(re), im(V3(i, j, k)) {}
	explicit quaternion(V1 re = 0, const V3& im = V3(0)): re(re), im(im) {}
	explicit quaternion(const V3& im): re(0), im(im) {} 
	explicit quaternion(const V4& v): re(v.x), im(v.y, v.z, v.w) {}
	static quaternion rotation(const V3& axis, V1 angle) {
		V1 psi = angle / V1(2);
		return quaternion(cos(psi), sin(psi) * axis);
	}
	V1& operator[](int i) {return (&re)[i];}
	const V1& operator[](int i) const {return (&re)[i];}
	V1 re;
	V3 im;
};
Q bar(const Q& q) {return Q(q.re, -q.im);}
V1 dot(const Q& a, const Q& b) {return dot(V4(a), V4(b));}
Q OP*(const V1 a, const Q& b) {return Q(a * b.re, a * b.im);}
Q OP*(const Q& a, const V1 b) {return b * a;}
Q OP*(const Q& a, const Q& b) {
	V1 re = a.re * b.re - dot(a.im, b.im);
	V3 im = a.re * b.im + a.im * b.re + cross(a.im, b.im);
	return Q(re, im);
}
Q OP/(const Q& a, const V1 b) {return (V1(1) / b) * a;}
Q OP/(const Q& a, const Q& b) {return (a * bar(b)) / dot(b, b);}
Q OP/(const V1 a, const Q& b) {return Q(a) / b;}
Q OP+(const Q& a, const Q& b) {return Q(V4(a) + V4(b));}
Q OP-(const Q& a, const Q& b) {return Q(V4(a) - V4(b));}
Q OP+(const Q& a) {return Q(+V4(a));}
Q OP-(const Q& a) {return Q(-V4(a));}
Q& OP+=(Q& a, const Q& b) {a.re+= b.re; a.im+= b.im; return a;}
Q& OP+=(Q& a, const V1& b) {a.re+= b; return a;}
Q& OP-=(Q& a, const Q& b) {a.re-= b.re; a.im-= b.im; return a;}
Q& OP-=(Q& a, const V1& b) {a.re-= b; return a;}
Q& OP*=(Q& a, const Q& b) {a = a * b; return a;}
Q& OP*=(Q& a, const V1& b) {a = a * b; return a;}
V1 norm(const Q& q) {return sqrt(dot(q, q));}
Q normalize(const Q& q) {return Q(normalize(V4(q)));}
V3::V3(const Q& q): x(q.im.x), y(q.im.y), z(q.im.z) {}
V4::V4(const Q& q): x(q.re), y(q.im.x), z(q.im.y), w(q.im.z) {}


// *****************************************************************************
/* Temporary Macros Cleanup */
#undef OP
#undef V1
#undef V2
#undef V3
#undef V4
#undef M2
#undef M3
#undef M4
#undef Z
#undef Q

} // namespace dja

//
//
//// end header file ///////////////////////////////////////////////////////////
#endif // DJA_INCLUDE_DJ_ALGEBRA_H

#if DJ_ALGEBRA_IMPLEMENTATION

#include <cstring> // std::memcpy

#ifndef DJA_ASSERT
#	include <assert.h>
#	define DJA_ASSERT(x) assert(x)
#endif

namespace dja {

// *****************************************************************************
/* Temporary Macros */
#define M00(m) m[0][0]
#define M01(m) m[0][1]
#define M02(m) m[0][2]
#define M03(m) m[0][3]
#define M10(m) m[1][0]
#define M11(m) m[1][1]
#define M12(m) m[1][2]
#define M13(m) m[1][3]
#define M20(m) m[2][0]
#define M21(m) m[2][1]
#define M22(m) m[2][2]
#define M23(m) m[2][3]
#define M30(m) m[3][0]
#define M31(m) m[3][1]
#define M32(m) m[3][2]
#define M33(m) m[3][3]

// *****************************************************************************
// Matrix API Implementation

//------------------------------------------------------------------------------
// Constructors
mat2::mat2(float_t m00, float_t m01, float_t m10, float_t m11)
{
	m[0] = vec2(m00, m01);
	m[1] = vec2(m10, m11);
}

mat2::mat2(const vec2& m0, const vec2& m1)
{
	m[0] = m0;
	m[1] = m1;
}

mat2::mat2(float_t diag)
{
	m[0] = vec2(diag, 0);
	m[1] = vec2(0, diag);
}

mat3::mat3(
	float_t m00, float_t m01, float_t m02,
	float_t m10, float_t m11, float_t m12,
	float_t m20, float_t m21, float_t m22
) {
	m[0] = vec3(m00, m01, m02);
	m[1] = vec3(m10, m11, m12);
	m[2] = vec3(m20, m21, m22);
}

mat3::mat3(const vec3& m0, const vec3& m1, const vec3& m2)
{
	m[0] = m0;
	m[1] = m1;
	m[2] = m2;
}

mat3::mat3(float_t diag)
{
	m[0] = vec3(diag, 0, 0);
	m[1] = vec3(0, diag, 0);
	m[2] = vec3(0, 0, diag);
}

mat4::mat4(
	float_t m00, float_t m01, float_t m02, float_t m03,
	float_t m10, float_t m11, float_t m12, float_t m13,
	float_t m20, float_t m21, float_t m22, float_t m23,
	float_t m30, float_t m31, float_t m32, float_t m33
) {
	m[0] = vec4(m00, m01, m02, m03);
	m[1] = vec4(m10, m11, m12, m13);
	m[2] = vec4(m20, m21, m22, m23);
	m[3] = vec4(m30, m31, m32, m33);
}

mat4::mat4(const vec4& m0, const vec4& m1, const vec4& m2, const vec4& m3)
{
	m[0] = m0;
	m[1] = m1;
	m[2] = m2;
	m[3] = m3;
}

mat4::mat4(float_t diag)
{
	m[0] = vec4(diag, 0, 0, 0);
	m[1] = vec4(0, diag, 0, 0);
	m[2] = vec4(0, 0, diag, 0);
	m[3] = vec4(0, 0, 0, diag);
}

//------------------------------------------------------------------------------
// Determinant
float_t determinant(const mat2& m)
{
	return M00(m) * M11(m) - M10(m) * M01(m);
}

float_t determinant(const mat3& m)
{
	const float_t d1 = M11(m) * M22(m) - M21(m) * M12(m);
	const float_t d2 = M21(m) * M02(m) - M01(m) * M22(m);
	const float_t d3 = M01(m) * M12(m) - M11(m) * M02(m);

	return M00(m) * d1 - M10(m) * d2 + M20(m) * d3;
}

float_t determinant(const mat4& m)
{
	const float_t s0 = M00(m) * M11(m) - M10(m) * M01(m);
	const float_t s1 = M00(m) * M12(m) - M10(m) * M02(m);
	const float_t s2 = M00(m) * M13(m) - M10(m) * M03(m);
	const float_t s3 = M01(m) * M12(m) - M11(m) * M02(m);
	const float_t s4 = M01(m) * M13(m) - M11(m) * M03(m);
	const float_t s5 = M02(m) * M13(m) - M12(m) * M03(m);
	const float_t c5 = M22(m) * M33(m) - M32(m) * M23(m);
	const float_t c4 = M21(m) * M33(m) - M31(m) * M23(m);
	const float_t c3 = M21(m) * M32(m) - M31(m) * M22(m);
	const float_t c2 = M20(m) * M33(m) - M30(m) * M23(m);
	const float_t c1 = M20(m) * M32(m) - M30(m) * M22(m);
	const float_t c0 = M20(m) * M31(m) - M30(m) * M21(m);
	const float_t det = s0 * c5 - s1 * c4 + s2 * c3 
	                  + s3 * c2 - s4 * c1 + s5 * c0;

	return det;
}

//------------------------------------------------------------------------------
// Transpose
mat2 transpose(const mat2& m)
{
	return mat2(M00(m), M10(m), M01(m), M11(m));
}

mat3 transpose(const mat3& m)
{
	return mat3(M00(m), M10(m), M20(m),
	            M01(m), M11(m), M21(m),
	            M02(m), M12(m), M22(m));
}

mat4 transpose(const mat4& m)
{
	return mat4(M00(m), M10(m), M20(m), M30(m),
	            M01(m), M11(m), M21(m), M31(m),
	            M02(m), M12(m), M22(m), M32(m),
	            M03(m), M13(m), M23(m), M33(m));
}

//------------------------------------------------------------------------------
// Adjugate
mat2 adjugate(const mat2& m)
{
	return mat2( M11(m), -M01(m),
	            -M10(m),  M00(m));
}

mat3 adjugate(const mat3& m)
{
	mat3 a;

	M00(a) = M11(m) * M22(m) - M12(m) * M21(m);
	M01(a) =-M01(m) * M22(m) + M02(m) * M21(m);
	M02(a) = M01(m) * M12(m) - M02(m) * M11(m);

	M10(a) =-M10(m) * M22(m) + M12(m) * M20(m);
	M11(a) = M00(m) * M22(m) - M02(m) * M20(m);
	M12(a) =-M00(m) * M12(m) + M02(m) * M10(m);

	M20(a) = M10(m) * M21(m) - M11(m) * M20(m);
	M21(a) =-M00(m) * M21(m) + M01(m) * M20(m);
	M22(a) = M00(m) * M11(m) - M01(m) * M10(m);

	return a;
}

mat4 adjugate(const mat4& m)
{
	/* based on Laplace expansion theorem */
	const float_t s0 = M00(m) * M11(m) - M10(m) * M01(m);
	const float_t s1 = M00(m) * M12(m) - M10(m) * M02(m);
	const float_t s2 = M00(m) * M13(m) - M10(m) * M03(m);
	const float_t s3 = M01(m) * M12(m) - M11(m) * M02(m);
	const float_t s4 = M01(m) * M13(m) - M11(m) * M03(m);
	const float_t s5 = M02(m) * M13(m) - M12(m) * M03(m);
	const float_t c5 = M22(m) * M33(m) - M32(m) * M23(m);
	const float_t c4 = M21(m) * M33(m) - M31(m) * M23(m);
	const float_t c3 = M21(m) * M32(m) - M31(m) * M22(m);
	const float_t c2 = M20(m) * M33(m) - M30(m) * M23(m);
	const float_t c1 = M20(m) * M32(m) - M30(m) * M22(m);
	const float_t c0 = M20(m) * M31(m) - M30(m) * M21(m);
	mat4 a;

	M00(a) = M11(m)*c5 - M12(m)*c4 + M13(m)*c3;
	M01(a) =-M01(m)*c5 + M02(m)*c4 - M03(m)*c3;
	M02(a) = M31(m)*s5 - M32(m)*s4 + M33(m)*s3;
	M03(a) =-M21(m)*s5 + M22(m)*s4 - M23(m)*s3;

	M10(a) =-M10(m)*c5 + M12(m)*c2 - M13(m)*c1;
	M11(a) = M00(m)*c5 - M02(m)*c2 + M03(m)*c1;
	M12(a) =-M30(m)*s5 + M32(m)*s2 - M33(m)*s1;
	M13(a) = M20(m)*s5 - M22(m)*s2 + M23(m)*s1;

	M20(a) = M10(m)*c4 - M11(m)*c2 + M13(m)*c0;
	M21(a) =-M00(m)*c4 + M01(m)*c2 - M03(m)*c0;
	M22(a) = M30(m)*s4 - M31(m)*s2 + M33(m)*s0;
	M23(a) =-M20(m)*s4 + M21(m)*s2 - M23(m)*s0;

	M30(a) =-M10(m)*c3 + M11(m)*c1 - M12(m)*c0;
	M31(a) = M00(m)*c3 - M01(m)*c1 + M02(m)*c0;
	M32(a) =-M30(m)*s3 + M31(m)*s1 - M32(m)*s0;
	M33(a) = M20(m)*s3 - M21(m)*s1 + M22(m)*s0;

	return a;
}

//------------------------------------------------------------------------------
// Inverse
mat2 inverse(const mat2& m)
{
	float_t det = determinant(m);
	DJA_ASSERT(det != float_t(0));

	return (float_t(1) / det) * adjugate(m);
}

mat3 inverse(const mat3& m)
{
	float_t det = determinant(m);
	DJA_ASSERT(det != float_t(0));

	return (float_t(1) / det) * adjugate(m);
}

mat4 inverse(const mat4& m)
{
	/* based on Laplace expansion theorem */
	const float_t s0 = M00(m) * M11(m) - M10(m) * M01(m);
	const float_t s1 = M00(m) * M12(m) - M10(m) * M02(m);
	const float_t s2 = M00(m) * M13(m) - M10(m) * M03(m);
	const float_t s3 = M01(m) * M12(m) - M11(m) * M02(m);
	const float_t s4 = M01(m) * M13(m) - M11(m) * M03(m);
	const float_t s5 = M02(m) * M13(m) - M12(m) * M03(m);
	const float_t c5 = M22(m) * M33(m) - M32(m) * M23(m);
	const float_t c4 = M21(m) * M33(m) - M31(m) * M23(m);
	const float_t c3 = M21(m) * M32(m) - M31(m) * M22(m);
	const float_t c2 = M20(m) * M33(m) - M30(m) * M23(m);
	const float_t c1 = M20(m) * M32(m) - M30(m) * M22(m);
	const float_t c0 = M20(m) * M31(m) - M30(m) * M21(m);
	const float_t det = s0 * c5 - s1 * c4 + s2 * c3 
	                  + s3 * c2 - s4 * c1 + s5 * c0;
	DJA_ASSERT(det != float_t(0));
	mat4 a;

	M00(a) = M11(m)*c5 - M12(m)*c4 + M13(m)*c3;
	M01(a) =-M01(m)*c5 + M02(m)*c4 - M03(m)*c3;
	M02(a) = M31(m)*s5 - M32(m)*s4 + M33(m)*s3;
	M03(a) =-M21(m)*s5 + M22(m)*s4 - M23(m)*s3;

	M10(a) =-M10(m)*c5 + M12(m)*c2 - M13(m)*c1;
	M11(a) = M00(m)*c5 - M02(m)*c2 + M03(m)*c1;
	M12(a) =-M30(m)*s5 + M32(m)*s2 - M33(m)*s1;
	M13(a) = M20(m)*s5 - M22(m)*s2 + M23(m)*s1;

	M20(a) = M10(m)*c4 - M11(m)*c2 + M13(m)*c0;
	M21(a) =-M00(m)*c4 + M01(m)*c2 - M03(m)*c0;
	M22(a) = M30(m)*s4 - M31(m)*s2 + M33(m)*s0;
	M23(a) =-M20(m)*s4 + M21(m)*s2 - M23(m)*s0;

	M30(a) =-M10(m)*c3 + M11(m)*c1 - M12(m)*c0;
	M31(a) = M00(m)*c3 - M01(m)*c1 + M02(m)*c0;
	M32(a) =-M30(m)*s3 + M31(m)*s1 - M32(m)*s0;
	M33(a) = M20(m)*s3 - M21(m)*s1 + M22(m)*s0;

	return (float_t(1) / det) * a;
}

//------------------------------------------------------------------------------
// Matrix-Scalar Muliplication
mat2 operator*(const float_t s, const mat2& m)
{
	return mat2(s * m[0], s * m[1]);
}

mat3 operator*(const float_t s, const mat3& m)
{
	return mat3(s * m[0], s * m[1], s * m[2]);
}

mat4 operator*(const float_t s, const mat4& m)
{
	return mat4(s * m[0], s * m[1], s * m[2], s * m[3]);
}

//------------------------------------------------------------------------------
// Matrix-Vector Muliplication
vec2 operator*(const mat2& m, const vec2& a)
{
	return vec2(dot(m[0], a), dot(m[1], a));
}

vec3 operator*(const mat3& m, const vec3& a)
{
	return vec3(dot(m[0], a), dot(m[1], a), dot(m[2], a));
}

vec4 operator*(const mat4& m, const vec4& a)
{
	return vec4(dot(m[0], a), dot(m[1], a), dot(m[2], a), dot(m[3], a));
}


//------------------------------------------------------------------------------
// Matrix-Matrix Muliplication
mat2 operator*(const mat2& a, const mat2& b)
{
	mat2 t = transpose(b), r;

	for (int j = 0; j < 2; ++j)
	for (int i = 0; i < 2; ++i)
		r[j][i] = dot(a[j], t[i]);

	return r;
}

mat3 operator*(const mat3& a, const mat3& b)
{
	mat3 t = transpose(b), r;

	for (int j = 0; j < 3; ++j)
	for (int i = 0; i < 3; ++i)
		r[j][i] = dot(a[j], t[i]);

	return r;
}

mat4 operator*(const mat4& a, const mat4& b)
{
	mat4 t = transpose(b), r;

	for (int j = 0; j < 4; ++j)
	for (int i = 0; i < 4; ++i)
		r[j][i] = dot(a[j], t[i]);

	return r;
}

//------------------------------------------------------------------------------
// Matrix Factories
mat2 mat2::memcpy(const float_t *v, bool rowmajor)
{
	mat2 m;
	std::memcpy(&m[0][0], v, sizeof(mat2));
	return rowmajor ? m : transpose(m);
}

mat3 mat3::memcpy(const float_t *v, bool rowmajor)
{
	mat3 m;
	std::memcpy(&m[0][0], v, sizeof(mat3));
	return rowmajor ? m : transpose(m);
}

mat4 mat4::memcpy(const float_t *v, bool rowmajor)
{
	mat4 m;
	std::memcpy(&m[0][0], v, sizeof(mat4));
	return rowmajor ? m : transpose(m);
}

/* Scale */

mat2 mat2::scale(float_t value) { return mat2(value); }
mat2 mat2::scale(const vec2& values)
{
	return mat2(values[0], 0,
	            0, values[1]);
}

mat3 mat3::scale(float_t value) { return mat3(value); }
mat3 mat3::scale(const vec3& values)
{
	return mat3(values[0], 0, 0,
	            0, values[1], 0,
	            0, 0, values[2]);
}

mat4 mat4::homogeneous::scale(float_t value)
{
	return mat4::homogeneous::from_mat3(mat3::scale(value));
}
mat4 mat4::homogeneous::scale(const vec3& values)
{
	return mat4::homogeneous::from_mat3(mat3::scale(values));
}

/* Rotation */

mat2 mat2::rotation(float_t rad)
{
	return mat2(complex::polar(rad, 1));
}

mat3 mat3::rotation(const vec3& axis, float_t rad)
{
	return rotation(quaternion::rotation(axis, rad));
}
mat3 mat3::rotation(const quaternion& q)
{
	float_t jj2 = float_t(2) * q[2] * q[2];
	float_t ij2 = float_t(2) * q[1] * q[2];
	float_t ik2 = float_t(2) * q[1] * q[3];
	float_t jk2 = float_t(2) * q[2] * q[3];
	float_t kk2 = float_t(2) * q[3] * q[3];
	float_t rk2 = float_t(2) * q[0] * q[3];
	float_t rj2 = float_t(2) * q[0] * q[2];
	float_t ri2 = float_t(2) * q[0] * q[1];
	float_t ii2 = float_t(2) * q[1] * q[1];

	return mat3(float_t(1) - jj2 - kk2, ij2 + rk2, ik2 - rj2,
	            ij2 - rk2, float_t(1) - ii2 - kk2, jk2 + ri2,
	            ik2 + rj2, jk2 - ri2, float_t(1) - ii2 - jj2);
}

mat4 mat4::homogeneous::rotation(const vec3& axis, float_t rad)
{
	return mat4::homogeneous::from_mat3(mat3::rotation(axis, rad));
}
mat4 mat4::homogeneous::rotation(const quaternion& q)
{
	return mat4::homogeneous::from_mat3(mat3::rotation(q));
}

/* Homogeneous Transformations */
mat4 mat4::homogeneous::from_mat3(const mat3& m)
{
	return mat4(M00(m), M01(m), M02(m), 0,
	            M10(m), M11(m), M12(m), 0,
	            M20(m), M21(m), M22(m), 0,
	            0     , 0     , 0     , 1);
}

mat4 mat4::homogeneous::translation(const vec3& dir)
{
	return mat4(1, 0, 0, dir.x,
	            0, 1, 0, dir.y,
	            0, 0, 1, dir.z,
	            0, 0, 0, 1     );
}

/* Projections */
mat4
mat4::homogeneous::perspective(
	float_t fovy,
	float_t aspect,
	float_t zNear,
	float_t zFar
) {
	DJA_ASSERT(fovy > float_t(0));
	DJA_ASSERT(aspect > float_t(0));
	DJA_ASSERT(zNear > float_t(0));
	DJA_ASSERT(zNear < zFar);
	float_t f = float_t(1) / tan(fovy / float_t(2));
	float_t c = float_t(1) / (zNear - zFar);
	float_t a = (zFar + zNear) * c;
	float_t b = float_t(2) * zNear * zFar * c;
#if 0 // Standard OpenGL
	return mat4(f / aspect, 0, 0 , 0,
	            0         , f, 0 , 0,
	            0         , 0, a , b,
	            0         , 0, -1, 0);
#else // XYZ -> OpenGL
	return mat4( 0, f / aspect, 0, 0,
	             0, 0         , f, 0,
	             a, 0         , 0, b,
	            -1, 0         , 0, 0);
#endif
}

mat4
mat4::homogeneous::orthographic(
	float_t left, float_t right,
	float_t bottom, float_t top,
	float_t near, float_t far
) {
	DJA_ASSERT(left != right && bottom != top && near != far);
	float_t c1 = float_t(1) / (right - left);
	float_t c2 = float_t(1) / (top - bottom);
	float_t c3 = float_t(1) / (far - near);
	float_t d1 = float_t(2) * c1;
	float_t d2 = float_t(2) * c2;
	float_t d3 = -float_t(2) * c3;
	float_t tx = -(right + left) * c1;
	float_t ty = -(top + bottom) * c2;
	float_t tz = -(far + near) * c3;

#if 0 // Standard OpenGL
	return mat4(d1, 0 , 0 , tx,
	            0 , d2, 0 , ty,
	            0 , 0 , d3, tz,
	            0 , 0 , 0 ,  1);
#else // XYZ -> OpenGL
	return mat4(0 , d1, 0 , tx,
	            0 , 0 , d2, ty,
	            d3, 0 , 0 , tz,
	            0 , 0 , 0 ,  1);
#endif
}

mat4
mat4::homogeneous::tile(
	float_t left, float_t right,
	float_t bottom, float_t top
) {
	DJA_ASSERT(left != right && bottom != top);
	float_t c1 = float_t(1) / (right - left);
	float_t c2 = float_t(1) / (top - bottom);
	float_t d1 = float_t(2) * c1;
	float_t d2 = float_t(2) * c2;
	float_t tx = -(right + left) * c1;
	float_t ty = -(top + bottom) * c2;

	return mat4(d1, 0 , 0, tx,
	            0 , d2, 0, ty,
	            0 , 0 , 1,  0,
	            0 , 0 , 0,  1);
}


// *****************************************************************************
/* Temporary Macros Cleanup */
#undef M00
#undef M01
#undef M02
#undef M03
#undef M10
#undef M11
#undef M12
#undef M13
#undef M20
#undef M21
#undef M22
#undef M23
#undef M30
#undef M31
#undef M32
#undef M33

} // namespace dja

#endif // DJ_ALGEBRA_IMPLEMENTATION


