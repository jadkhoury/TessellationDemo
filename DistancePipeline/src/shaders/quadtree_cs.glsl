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

uniform float deltaT;
uniform vec3 prim_centroid;

uniform int num_mesh_tri;
uniform int num_mesh_quad;

uniform mat4 M, V, P, MVP;


uint invocation_idx;
uint current_lvl;
uvec2 current_key;

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

// Everything in this shader is computed in Homogeneous 2D
// points: (x,y,1)
// directions: (x,y,0)
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

	current_key = getKey30(invocation_idx);
	current_lvl = lt_level_2_30(current_key);

	bool should_divide, should_merge;
	if(uniform_subdiv > 0)
	{
		should_divide = current_lvl < uniform_level;
		should_merge = current_lvl > uniform_level;
	}
	else
	{
		mat3 xform, parent_xform;
		if(prim_type == QUADS)
			lt_get_quad_xform_2_30(current_key, xform, parent_xform);
		else if (prim_type == TRIANGLES)
			lt_get_triangle_xform_2_30(current_key, xform, parent_xform);

		vec3 node_center = xform * prim_centroid;
		vec3 parent_center = parent_xform * prim_centroid;

		vec4 node_center_3D, parent_center_3D;

		// Compute the node and parent center coordinates in world space
		// (ie place it on the mesh)
		if (prim_type == QUADS)
		{
			Quad q;
			getMeshQuad(old_nodes[invocation_idx][2], q);
			node_center_3D   = M * mapToMeshQuad(q, node_center.xy);
			parent_center_3D = M * mapToMeshQuad(q, parent_center.xy);
		}
		else if (prim_type == TRIANGLES)
		{
			Triangle t;
			getMeshTriangle(old_nodes[invocation_idx][2], t);
			node_center_3D   = M * mapToMeshTriangle(t, node_center.xy);
			parent_center_3D = M * mapToMeshTriangle(t, parent_center.xy);
		}
		float z1 = distance(node_center_3D.xyz, cam_pos);
		float z2 = distance(parent_center_3D.xyz, cam_pos);

		// Decide action based on LoD
		should_divide = float(current_lvl) < distanceToLod(z1);
		should_merge  = float(current_lvl) - 1.0 >= distanceToLod(z2);
	}

	if(should_divide && !lt_is_leaf_2_30(current_key)) {
		//Divide
		uvec2 children[4];
		lt_children_2_30(current_key, children);
		for(int i = 0; i < 4; ++i)
		{
			writeKey(children[i]);
		}
	} else if (should_merge && !lt_is_root_2_30(current_key)){
		//Merge
		if(lt_is_upper_left_2_30(current_key))
		{
			uvec2 parent = lt_parent_2_30(current_key);
			writeKey(parent);
		}
	} else {
		writeKey(current_key);
	}
}

#endif


