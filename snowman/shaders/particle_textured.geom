#version 450 core

// The input primitive specification.
layout (points) in;
// The ouput primitive specification.
layout (triangle_strip, max_vertices = 4) out;

// NOTE TO SELF: ^^ Means for every point, we generate 4 vertices

// ----------------------------------------------------------------------------
// Local Variables
// ----------------------------------------------------------------------------
// The texture coorinates for the emitted vertices.
const vec2 quad_tex_coords[4] = vec2[4](
	vec2(0.0, 1.0),
	vec2(0.0, 0.0),
	vec2(1.0, 1.0),
	vec2(1.0, 0.0)
);
// The position offsets for the emitted vertices.
const vec4 quad_offsets[4] = vec4[4](
	vec4(-0.5, +0.5, 0.0, 0.0),
	vec4(-0.5, -0.5, 0.0, 0.0),
	vec4(+0.5, +0.5, 0.0, 0.0),
	vec4(+0.5, -0.5, 0.0, 0.0)
);

// ----------------------------------------------------------------------------
// Input Variables
// ----------------------------------------------------------------------------
in VertexData
{
	vec3 particle_color;	      	// The particle color.
	vec4 particle_position_vs;  	// The particle position in view space.
} in_data[1];

// The UBO with camera data.	
layout (std140, binding = 0) uniform CameraBuffer
{
	mat4 projection;		// The projection matrix.
	mat4 projection_inv;	// The inverse of the projection matrix.
	mat4 view;				// The view matrix
	mat4 view_inv;			// The inverse of the view matrix.
	mat3 view_it;			// The inverse of the transpose of the top-left part 3x3 of the view matrix
	vec3 eye_position;		// The position of the eye in world space.
};

// The size of a particle in view space.
layout (location = 3) uniform float particle_size_vs;

// ----------------------------------------------------------------------------
// Output Variables
// ----------------------------------------------------------------------------
out VertexData
{
	vec3 particle_color;	    // The particle color.
	vec2 tex_coord;    			// The texture coordinates for the particle.
} out_data;

// ----------------------------------------------------------------------------
// Main Method
// ----------------------------------------------------------------------------
void main()
{
	// For each vertex of the quad
	for (int i = 0; i < 4; ++i)
	{
		// out_data
		out_data.particle_color = in_data[0].particle_color;
		out_data.tex_coord = quad_tex_coords[i];
		// gl_Position
		gl_Position = projection * (in_data[0].particle_position_vs + quad_offsets[i] * particle_size_vs);

		EmitVertex();
	}

	EndPrimitive();
}