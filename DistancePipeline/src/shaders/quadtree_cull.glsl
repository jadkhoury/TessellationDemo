#line 2
/* ---- In ltree_common.glsl ----- *
 struct dj_vertex;

 uniform mat3 shear_mat;
 uniform float adaptive_factor;
 uniform int prim_type;
 uniform vec3 cam_pos;

 mat3 toLocal, toWorld;
 dj_vertex mesh_v[];
 uint mesh_q_idx[];
 uint  mesh_t_idx[];

 float distanceToLod(float x);
 void getMeshMapping(uint idx, out mat3 toUnit, out mat3 toOriginal);
*/

/* ---- In dj_frustum.glsl -----
bool dcf_cull(mat4 mvp, vec3 bmin, vec3 bmax);
*/

#ifdef COMPUTE_SHADER

layout (local_size_x = 256) in;

layout (std430, binding = 0) readonly buffer Data_In
{
	uvec4 old_nodes[];
};

layout (std430, binding = 1) buffer Data_Out
{
	uvec4 new_nodes[];
};

layout (std430, binding = 2) readonly buffer Indirect_Draw_Data_In
{
	uint  old_count;
	uint  old_primCount;
};

layout (std430, binding = 3) buffer Indirect_Draw_Data_Out
{
	uint  count;
	uint  primCount;
};

uniform int uniform_level;
uniform int uniform_subdiv;

uniform int firstFrame;
uniform float deltaT;
uniform vec3 prim_centroid;

uniform int num_mesh_tri;
uniform int num_mesh_quad;

uniform mat4 M, V, P, MVP;

uint invocation_idx;
uint current_lvl;
uvec2 key;

bool should_divide, should_merge;


uint getKey(uint idx)
{
	return old_nodes[idx][0];
}

uvec2 getKey30(uint idx)
{
	return old_nodes[idx].xy;
}

void writeKey(uint new_key)
{
	uint idx = atomicAdd(primCount, 1);
	new_nodes[idx][0] = new_key;
	new_nodes[idx][1] = old_nodes[invocation_idx][1];
}
void writeKey(uvec2 new_key)
{
	uint idx = atomicAdd(primCount, 1);
	new_nodes[idx].xy = new_key;
	new_nodes[idx][2] = old_nodes[invocation_idx][2];

}

void main(void)
{
	invocation_idx = int(gl_GlobalInvocationID.x);
	int active_nodes;
	if(prim_type == QUADS) {
		active_nodes = max(num_mesh_quad, int(old_primCount));
	} else if (prim_type == TRIANGLES) {
		active_nodes = max(num_mesh_tri, int(old_primCount));
	}
	if(invocation_idx >= active_nodes)
		return;


	uvec2 key = getKey30(invocation_idx);
	mat3 xform;
	vec3 b_min = vec3(1e5), b_max = vec3(-1e5);

	vec4 pos3d;
	vec3 node_v_pos;
	if(prim_type == QUADS)
	{
		lt_get_quad_xform_2_30(key, xform);
		vec3 prim_v_pos[4] = { vec3(0,0,1), vec3(0,1,1), vec3(1,0,1), vec3(1,1,1) };
		Quad q;
		getMeshQuad(old_nodes[invocation_idx][2], q);
		for(int i = 0; i < 4; ++i)
		{
			node_v_pos = xform * prim_v_pos[i];
			pos3d = mapToMeshQuad(q, node_v_pos.xy);
			b_min = min(b_min, pos3d.xyz);
			b_max = max(b_min, pos3d.xyz);
		}
	}
	else if (prim_type == TRIANGLES)
	{
		Triangle t;
		getMeshTriangle(old_nodes[invocation_idx][2], t);
		lt_get_triangle_xform_2_30(key, xform);
		vec3 prim_v_pos[3] = { vec3(0,0,1), vec3(0,1,1), vec3(1,0,1)};
		for(int i = 0; i < 3; ++i)
		{
			node_v_pos = xform * prim_v_pos[i];
			pos3d =  mapToMeshTriangle(t, node_v_pos.xy);
			b_min = min(b_min, pos3d.xyz);
			b_max = max(b_max, pos3d.xyz);
		}
	}

	// Breaking homogeneity: 3D points in 3 coords
	b_min.z = -0.5;
	b_max.z = 0.5;


	if(dcf_cull(MVP, b_min, b_max)) writeKey(key);
}


#endif
