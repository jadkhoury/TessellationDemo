#line 1002

#ifndef LTREE_GLSL
#define LTREE_GLSL

#if (__VERSION__ < 150) // check version
#	error Uncompatible GLSL version
#endif

// ************************ DECLARATIONS ************************ //

const float M_PI = 3.1415926535897932384626433832795;

struct Quad {
	vec4 p[4]; // vertices
	vec2 t[4]; // texcoords
};

struct Triangle {
	vec4 p[3]; // vertices
	vec2 t[3]; // texcoords
};

/* children nodes */
void lt_children_2_15 (in uint key, out uint children[4]);
/* parent node */
uint lt_parent_2_15 (in uint key);
/* query node level */
uint lt_level_2_15 (in uint key);
/* check if the node is a leaf */
bool lt_is_leaf_2_15 (in uint key);
/* check if the node is the root */
bool lt_is_root_2_15 (in uint key);
/* check node orientation (ctd) */
bool lt_is_left_2_15 (in uint key);
bool lt_is_right_2_15 (in uint key);
bool lt_is_upper_2_15 (in uint key);
bool lt_is_lower_2_15 (in uint key);
bool lt_is_upper_left_2_15 (in uint key);

/* get normalized position + optionally parent (in [0,1]) */
void lt_get_quad_xform_2_15 (in uint key, out vec2 cell, out float cell_size);
void lt_get_quad_xform_2_15 (in uint key, out vec2 cell, out float cell_size, out vec2 parent_cell);
void lt_get_quad_xform_2_15(in uint key, out mat3 xform);
void lt_get_quad_xform_2_15(in uint key, out mat3 xform, out mat3 parent_xform);

void lt_get_triangle_xform_2_15(in uint key, out vec2 translat, out float theta, out float scale);
void lt_get_triangle_xform_2_15(in uint key, out mat3 xform);
void lt_get_triangle_xform_2_15(in uint key, out mat3 xform, out float scale);
void lt_get_triangle_xform_2_15(in uint key, out mat3 xform, out mat3 parent_xform);

mat3 buildXformMatrix(in vec2 tr, in float scale, in int cosT = 1, in int sinT = 0);

// Utility functions for uvec2 representation
uvec2 leftShift(in uvec2 key, in uint shift);
uvec2 rightShift(in uvec2 key, in uint shift);
int findVec2MSB(uvec2 key);


// ************************ IMPLEMENTATIONS ************************ //

// ------------------ Utility functions ------------------ //

mat3 buildXformMatrix(in vec2 tr, in float scale, in int cosT = 1, in int sinT = 0)
{
	mat3 xform = mat3(1.0);
	if(cosT != 1 || sinT != 0){
		//translation * scale * rotation =>
		// | sx*cosT  -sx*sinT  tr_x |
		// | sy*sinT   sy*cosT  tr_y |
		// |   0         0       1   |
		xform[0][0] = scale * cosT; xform[1][0] =-scale * sinT; xform[2][0] = tr.x;
		xform[0][1] = scale * sinT; xform[1][1] = scale * cosT; xform[2][1] = tr.y;
	} else {
		xform[0][0] = scale;
		xform[1][1] = scale;
		xform[2][0] = tr.x;
		xform[2][1] = tr.y;
		xform[2][2] = 1.0;
	}
	return xform;
}


uvec2 leftShift(in uvec2 key, in uint shift)
{
	uvec2 result = key;
	//Extract the "shift" first bits of y and append them at the end of x
	result.x = result.x << shift;
	result.x |= result.y >> (32 - shift);
	result.y  = result.y << shift;
	return result;
}
uvec2 rightShift(uvec2 key, uint shift)
{
	uvec2 result = key;

	result.y = result.y >> shift;
	result.y |= result.x << (32 - shift);
	result.x = result.x >> shift;

	return result;
}

int findVec2MSB(uvec2 key)
{
	return (key.x == 0) ? findMSB(key.y) : (findMSB(key.x) + 32);
}

// ------------- Children and Parents --------------- //

void lt_children_2_15 (in uint key, out uint children[4])
{
	key = key << 2u;
    children[0] = key;
	children[1] = key | 0x1;
	children[2] = key | 0x2;
	children[3] = key | 0x3;
}

void lt_children_2_30 (in uvec2 key, out uvec2 children[4])
{
	key = leftShift(key, 2u);
	children[0] = key;
	children[1] = uvec2(key.x, key.y | 0x1);
	children[2] = uvec2(key.x, key.y | 0x2);
	children[3] = uvec2(key.x, key.y | 0x3);
}


uint lt_parent_2_15 (in uint key)
{
    return key >> 2u;
}

uvec2 lt_parent_2_30 (in uvec2 key)
{
	return rightShift(key, 2u);
}

// --------------------- Level ---------------------- //

uint lt_level_2_15 (in uint key)
{
    uint i = findMSB(key);
    return (i >> 1u);
}

uint lt_level_2_30 (in uvec2 key)
{
	uint i = findVec2MSB(key);
	return (i >> 1u);
}

// ------------------ Leaf & Root ------------------- //

bool lt_is_leaf_2_15 (in uint key)
{
	return (findMSB(key) == 30u);
	//Temporary, as the max lvl is 10
	//return (findMSB(key) == 20u);
}

bool lt_is_leaf_2_30 (in uvec2 key)
{
	return (findVec2MSB(key) == 32u);
	//Temporary, as the max lvl is 10
	//return (findVec2MSB(key) == 20u);
}

bool lt_is_root_2_15 (in uint key)
{
    return (findMSB(key) == 0u);
}

bool lt_is_root_2_30 (in uvec2 key)
{
	return (findVec2MSB(key) == 0u);
}

// -------------------- Position --------------------- //

bool lt_is_left_2_15 (in uint key)
{
    return !bool(key & 0x1);
}
bool lt_is_left_2_30 (in uvec2 key)
{
	return !bool(key.y & 0x1);
}

bool lt_is_right_2_15 (in uint key)
{
    return bool(key & 0x1);
}
bool lt_is_right_2_30 (in uvec2 key)
{
	return bool(key.y & 0x1);
}

bool lt_is_upper_2_15 (in uint key)
{
    return !bool(key & 0x2);
}
bool lt_is_upper_2_30 (in uvec2 key)
{
	return !bool(key.y & 0x2);
}

bool lt_is_lower_2_15 (in uint key)
{
	return bool(key & 0x2);
}
bool lt_is_lower_2_30 (in uvec2 key)
{
	return bool(key.y & 0x2);
}

bool lt_is_upper_left_2_15 (in uint key)
{
    return !(bool(key & 0x3));
}
bool lt_is_upper_left_2_30 (in uvec2 key)
{
	return !(bool(key.y & 0x3));
}

// key:  ---- ---- ---- --01 b1b2-- ---- ---- ----
// mask: ---- ---- ---- ----  11--  ---- ---- ----
// ------------------- Quad XForm ------------------- //

void lt_get_quad_xform_2_15(in uint key, out mat3 xform)
{
    vec2 tr = vec2(0.0);
    float scale = 1.0f;
    int shift = int(findMSB(key) - 2);
    uint mask, b1b2;
    uint b1, b2;
    while(shift >= 0)
    {
        mask = 0x3 << shift;
        b1b2 = (key & mask) >> shift;
        b1 = (b1b2 & 0x2) >> 1u;
        b2 = b1b2 & 0x1;

        scale *= 0.5;
        tr.x += scale * float(b2);
        tr.y += scale * float(b1 ^ 0x1);
        shift -= 2;
    }
    xform = buildXformMatrix(tr, scale);

}
void lt_get_quad_xform_2_30(in uvec2 key, out mat3 xform)
{
	if(key.x == 0){
		lt_get_quad_xform_2_15(key.y, xform);
		return;
	}
	uint current_half = key.x;
	bool round2 = false;

	vec2 tr = vec2(0.0);
	float scale = 1.0f;
	int shift = int(findMSB(current_half) - 2);
	uint mask, b1b2;
	uint b1, b2;

	if(key.x == 0x1)
	{
		round2 = true;
		shift = 30;
		current_half = key.y;
	}

	while(shift >= 0)
	{
		mask = 0x3 << shift;
		b1b2 = (current_half & mask) >> shift;
		b1 = (b1b2 & 0x2) >> 1u;
		b2 = b1b2 & 0x1;

		scale *= 0.5;
		tr.x += scale * float(b2);
		tr.y += scale * float(b1 ^ 0x1);
		shift -= 2;

		if(shift < 0 && !round2) {
			current_half = key.y;
			shift = 30;
			round2 = true;
		}
	}
	xform = buildXformMatrix(tr, scale);

}

void lt_get_quad_xform_2_15(in uint key, out mat3 xform, out mat3 parent_xform)
{
	vec2 tr = vec2(0.0);
	float scale = 1.0f;
	int shift = int(findMSB(key) - 2);
	uint mask, b1b2;
	uint b1, b2;
	while(shift >= 0)
	{
		if(shift == 0)
			parent_xform = buildXformMatrix(tr, scale);
		mask = 0x3 << shift;
		b1b2 = (key & mask) >> shift;
		b1 = (b1b2 & 0x2) >> 1u;
		b2 = b1b2 & 0x1;

		scale *= 0.5;
		tr.x += scale * float(b2);
		tr.y += scale * float(b1 ^ 0x1);
		shift -= 2;
	}
	xform = buildXformMatrix(tr, scale);
}

void lt_get_quad_xform_2_30(in uvec2 key, out mat3 xform, out mat3 parent_xform)
{
	if(key.x == 0){
		lt_get_quad_xform_2_15(key.y, xform, parent_xform);
		return;
	}
	uint current_half = key.x;
	bool round2 = false;

	vec2 tr = vec2(0.0);
	float scale = 1.0f;
	int shift = int(findMSB(current_half) - 2);
	uint mask, b1b2;
	uint b1, b2;

	if(key.x == 0x1)
	{
		round2 = true;
		shift = 30;
		current_half = key.y;
	}

	while(shift >= 0)
	{
		if(round2 && shift == 0)
			parent_xform = buildXformMatrix(tr, scale);
		mask = 0x3 << shift;
		b1b2 = (current_half & mask) >> shift;
		b1 = (b1b2 & 0x2) >> 1u;
		b2 = b1b2 & 0x1;

		scale *= 0.5;
		tr.x += scale * float(b2);
		tr.y += scale * float(b1 ^ 0x1);
		shift -= 2;

		if(shift < 0 && !round2) {
			current_half = key.y;
			shift = 30;
			round2 = true;
		}
	}
	xform = buildXformMatrix(tr, scale);
}

// ----------------- Triangle XForm ------------------ //

void lt_get_triangle_xform_2_15(in uint key, out mat3 xform)
{
	vec2 tr = vec2(0.0);
	float scale = 1.0f;
	int shift = int(findMSB(key) - 2);
	uint mask, b1b2;
	uint b1, b2;
	float theta = 0;
	int cosT = 1, sinT = 0;
	int old_cosT = 1;
	float tmp_x, tmp_y;
	int a = 1, b = 0;

	while(shift >= 0)
	{
		mask = 0x3 << shift;
		b1b2 = (key & mask) >> shift;
		b1 = (b1b2 & 0x2) >> 1u;
		b2 = b1b2 & 0x1;

		scale *= 0.5;
		tmp_x = scale * float(b1 & 0x1);
		tmp_y = scale * float(b1 ^ 0x1);

		tr.x += cosT*tmp_x - sinT*tmp_y;
		tr.y += sinT*tmp_x + cosT*tmp_y;

		a = int((b1 ^ b2) ^ 0x1);
		b = a + int(((b1 ^ b2) & b1) << 1) - 1;
		cosT = a*cosT - b*sinT;
		sinT = b*old_cosT + a*sinT;
		old_cosT = cosT;

		shift -= 2;
	}
	xform = buildXformMatrix(tr, scale, cosT, sinT);
}

void lt_get_triangle_xform_2_30(in uvec2 key, out mat3 xform)
{
	if(key.x == 0){
		lt_get_triangle_xform_2_15(key.y, xform);
		return;
	}
	uint current_half = key.x;
	bool round2 = false;

	vec2 tr = vec2(0.0);
	float scale = 1.0f;
	int shift = int(findMSB(current_half) - 2);
	uint mask, b1b2;
	uint b1, b2;
	float theta = 0;
	int cosT = 1, sinT = 0;
	int old_cosT = 1;
	float tmp_x, tmp_y;
	int a = 1, b = 0;

	if(key.x == 0x1)
	{
		round2 = true;
		shift = 30;
		current_half = key.y;
	}

	while(shift >= 0)
	{
		mask = 0x3 << shift;
		b1b2 = (current_half & mask) >> shift;
		b1 = (b1b2 & 0x2) >> 1u;
		b2 = b1b2 & 0x1;

		scale *= 0.5;
		tmp_x = scale * float(b1 & 0x1);
		tmp_y = scale * float(b1 ^ 0x1);

		tr.x += cosT*tmp_x - sinT*tmp_y;
		tr.y += sinT*tmp_x + cosT*tmp_y;

		a = int((b1 ^ b2) ^ 0x1);
		b = a + int(((b1 ^ b2) & b1) << 1) - 1;
		cosT = a*cosT - b*sinT;
		sinT = b*old_cosT + a*sinT;
		old_cosT = cosT;

		shift -= 2;
		if(shift < 0 && !round2) {
			current_half = key.y;
			shift = 30;
			round2 = true;
		}

	}
	xform = buildXformMatrix(tr, scale, cosT, sinT);
}


void lt_get_triangle_xform_2_15(in uint key, out mat3 xform, out mat3 parent_xform)
{
	vec2 tr = vec2(0.0);
	float scale = 1.0f;
	int shift = int(findMSB(key) - 2);
	uint mask, b1b2;
	uint b1, b2;
	float theta = 0;
	int cosT = 1, sinT = 0;
	int old_cosT = 1;
	float tmp_x, tmp_y;
	int a = 1, b = 0;

	while(shift >= 0)
	{
		if(shift == 0)
			parent_xform = buildXformMatrix(tr, scale, cosT, sinT);
		mask = 0x3 << shift;
		b1b2 = (key & mask) >> shift;
		b1 = (b1b2 & 0x2) >> 1u;
		b2 = b1b2 & 0x1;

		scale *= 0.5;
		tmp_x = scale * float(b1 & 0x1);
		tmp_y = scale * float(b1 ^ 0x1);

		tr.x += cosT*tmp_x - sinT*tmp_y;
		tr.y += sinT*tmp_x + cosT*tmp_y;

		a = int((b1 ^ b2) ^ 0x1);
		b = a + int(((b1 ^ b2) & b1) << 1) - 1;
		cosT = a*cosT - b*sinT;
		sinT = b*old_cosT + a*sinT;
		old_cosT = cosT;

		shift -= 2;
	}
	xform = buildXformMatrix(tr, scale, cosT, sinT);
}

void lt_get_triangle_xform_2_30(in uvec2 key, out mat3 xform, out mat3 parent_xform)
{
	if(key.x == 0){
		lt_get_triangle_xform_2_15(key.y, xform, parent_xform);
		return;
	}
	uint current_half = key.x;
	bool round2 = false;

	vec2 tr = vec2(0.0);
	float scale = 1.0f;
	int shift = int(findMSB(current_half) - 2);
	uint mask, b1b2;
	uint b1, b2;
	float theta = 0;
	int cosT = 1, sinT = 0;
	int old_cosT = 1;
	float tmp_x, tmp_y;
	int a = 1, b = 0;

	if(key.x == 0x1)
	{
		round2 = true;
		shift = 30;
		current_half = key.y;
	}

	while(shift >= 0)
	{
		if(round2 && shift == 0)
			parent_xform = buildXformMatrix(tr, scale, cosT, sinT);
		mask = 0x3 << shift;
		b1b2 = (current_half & mask) >> shift;
		b1 = (b1b2 & 0x2) >> 1u;
		b2 = b1b2 & 0x1;

		scale *= 0.5;
		tmp_x = scale * float(b1 & 0x1);
		tmp_y = scale * float(b1 ^ 0x1);

		tr.x += cosT*tmp_x - sinT*tmp_y;
		tr.y += sinT*tmp_x + cosT*tmp_y;

		a = int((b1 ^ b2) ^ 0x1);
		b = a + int(((b1 ^ b2) & b1) << 1) - 1;
		cosT = a*cosT - b*sinT;
		sinT = b*old_cosT + a*sinT;
		old_cosT = cosT;

		shift -= 2;

		if(shift < 0 && !round2) {
			current_half = key.y;
			shift = 30;
			round2 = true;
		}
	}
	xform = buildXformMatrix(tr, scale, cosT, sinT);
}

// ----------------------- Mappings ----------------------- //

mat3 adjugate(in mat3 m)
{
	float a,b,c,d,e,f,g,h,i;
	a = m[0][0]; b = m[1][0]; c = m[2][0];
	d = m[0][1]; e = m[1][1]; f = m[2][1];
	g = m[0][2]; h = m[1][2]; i = m[2][2];

	mat3 r = mat3(1.0);
	r[0][0] =  (e*i-f*h); r[1][0] = -(b*i-c*h); r[2][0] =  (b*f-c*e);
	r[0][1] = -(d*i-f*g); r[1][1] =  (a*i-c*g); r[2][1] = -(a*f-c*d);
	r[0][2] =  (d*h-e*g); r[1][2] = -(a*h-b*g); r[2][2] =  (a*e-b*d);

//	//Geometric method
//	vec3 x0 = m[0], x1 = m[1], x2 = m[2];
//	vec3 r0 = cross(x1,x2);
//	vec3 r1 = cross(x2, x0);
//	vec3 r2 = cross(x0, x1);
//	return glm::transpose(mat3(r0,r1,r2));

	return r ;

}

//Following notations from http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.26.6875&rep=rep1&type=pdf
void getQuadMappings(in vec3 p0, in vec3 p1, in vec3 p2, in vec3 p3, out mat3 toUnit, out mat3 toOriginal)
{
	float dx1 = p1.x - p2.x;
	float dx2 = p3.x - p2.x;
	float dy1 = p1.y - p2.y;
	float dy2 = p3.y - p2.y;
	float sx = p0.x - p1.x + p2.x - p3.x;
	float sy = p0.y - p1.y + p2.y - p3.y;
	float denom = (dx1 * dy2 - dx2 * dy1);
	float g = (sx * dy2 - dx2 * sy) / denom;
	float h = (dx1 * sy - sx * dy1) / denom;
	toOriginal = mat3(1.0);
	toOriginal[0][0] = p1.x - p0.x + g * p1.x;
	toOriginal[1][0] = p3.x - p0.x + h * p3.x;
	toOriginal[2][0] = p0.x;

	toOriginal[0][1] = p1.y - p0.y + g * p1.y;
	toOriginal[1][1] = p3.y - p0.y + h * p3.y;
	toOriginal[2][1] = p0.y;

	toOriginal[0][2] = g;
	toOriginal[1][2] = h;
	toOriginal[2][2] = 1.0;

	//hardcode adjugate instead
	toUnit = adjugate(toOriginal);
}

void getTriangleMappings(in vec3 p0, in vec3 p1, in vec3 p2, out mat3 toUnit, out mat3 toOriginal)
{
	toUnit = mat3(0);
	toOriginal = mat3(0);
	vec3 t = p0; // translation from origin
	vec3 x = p1 - p0; // x axis
	vec3 y = p2 - p0; // y axis

	toOriginal[0][0] = x.x; toOriginal[1][0] = y.x; toOriginal[2][0] = t.x;
	toOriginal[0][1] = x.y; toOriginal[1][1] = y.y; toOriginal[2][1] = t.y;
	toOriginal[0][2] = 0;   toOriginal[1][2] = 0;   toOriginal[2][2] = 1.0;

	float det = x.x * y.y - y.x * x.y;
	float detInv = 1.f / det;
	//first row
	toUnit[0][0] =  y.y * detInv;
	toUnit[1][0] = -y.x * detInv;
	toUnit[2][0] = -toUnit[0][0] * t.x - toUnit[1][0] * t.y;
	//second row
	toUnit[0][1] = -x.y * detInv;
	toUnit[1][1] =  x.x * detInv;
	toUnit[2][1] = -toUnit[0][1] * t.x - toUnit[1][1] * t.y;
	//third row
	toUnit[0][2] = 0;
	toUnit[1][2] = 0;
	toUnit[2][2] = 1;
}

vec4 mapToMeshTriangle(Triangle t, vec2 uv)
{
	return (1.0 - uv.x - uv.y) * t.p[0] + uv.x * t.p[1] + uv.y * t.p[2];
}

vec4 mapToMeshQuad(Quad q, vec2 uv)
{
	vec4 p01 = mix(q.p[0], q.p[1], uv.x);
	vec4 p32 = mix(q.p[3], q.p[2], uv.x);
	return mix(p01, p32, uv.y);
}

#endif

