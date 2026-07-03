#version 450 core

// ----------------------------------------------------------------------------
// Input Variables
// ----------------------------------------------------------------------------
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

// The structure holding the information about a single Phong light.
struct PhongLight
{
	vec4 position;                   // The position of the light. Note that position.w should be one for point lights and spot lights, and zero for directional lights.
	vec3 ambient;                    // The ambient part of the color of the light.
	vec3 diffuse;                    // The diffuse part of the color of the light.
	vec3 specular;                   // The specular part of the color of the light. 
	vec3 spot_direction;             // The direction of the spot light, irrelevant for point lights and directional lights.
	float spot_exponent;             // The spot exponent of the spot light, irrelevant for point lights and directional lights.
	float spot_cos_cutoff;           // The cosine of the spot light's cutoff angle, -1 point lights, irrelevant for directional lights.
	float atten_constant;            // The constant attenuation of spot lights and point lights, irrelevant for directional lights. For no attenuation, set this to 1.
	float atten_linear;              // The linear attenuation of spot lights and point lights, irrelevant for directional lights.  For no attenuation, set this to 0.
	float atten_quadratic;           // The quadratic attenuation of spot lights and point lights, irrelevant for directional lights. For no attenuation, set this to 0.
};

// The UBO with light data.
layout (std140, binding = 2) uniform PhongLightsBuffer
{
	vec3 global_ambient_color;		// The global ambient color.
	int lights_count;				// The number of lights in the buffer.
	PhongLight lights[3];			// The array with actual lights.
};

// The structure holding the information about a single Particle.
struct Particle
{
    vec3 position;
    vec3 velocity;
    vec3 color;
    int light_id;
    float delay;
};

// SSBO with the particles
layout (std430, binding = 3) buffer ParticlesBuffer
{
	Particle particles[];
};

// radius in which particles can appear
layout (location = 0) uniform float sphere_light_radius;
// time substracted from particle delay
layout (location = 1) uniform float time_delta;
// time used to update velocity
layout (location = 2) uniform float simulation_time_delta;

// ----------------------------------------------------------------------------
// Output Variables
// ----------------------------------------------------------------------------
out VertexData
{
	vec3 particle_color;	    // The particle color.
	vec4 particle_position_vs;  // The particle position in view space.
} out_data;

// ----------------------------------------------------------------------------
// Local Methods
// ----------------------------------------------------------------------------
// Function from: https://www.shadertoy.com/view/4djSRW
float hash11(float p)
{
    p = fract(p * .1031);
    p *= p + 33.33;
    p *= p + p;
    return fract(p);
}

// Function from: https://www.shadertoy.com/view/4djSRW
vec3 hash31(float p)
{
   vec3 p3 = fract(vec3(p) * vec3(.1031, .1030, .0973));
   p3 += dot(p3, p3.yzx+33.33);
   return fract((p3.xxy+p3.yzz)*p3.zyx); 
}

// ----------------------------------------------------------------------------
// Main Method
// ----------------------------------------------------------------------------
void main()
{
	// gl_VertexID holds the index of the vertex that is being processed
	Particle particle = particles[gl_VertexID]; // copy

	if (particle.delay < 0.0)
	{
		vec3 random_direction = hash31(gl_VertexID) * 2.0 - 1.0; // interval <-1, 1>
		random_direction = normalize(random_direction);
		particle.position = lights[particle.light_id].position.xyz + random_direction * sphere_light_radius;
		particle.velocity = random_direction * 1.5;
		particle.color = lights[particle.light_id].diffuse;
		particle.delay = hash11(gl_VertexID) * 20000.0; // 20 seconds
	}

	particle.delay -= time_delta;

	// Simulation
	vec3 acceleration = vec3(0.0, -9.81, 0.0);

	particle.position += particle.velocity * simulation_time_delta + 0.5 * acceleration * simulation_time_delta * simulation_time_delta;
	particle.velocity += acceleration * simulation_time_delta;

	// Ground hit check
	if (particle.position.y < 0.0)
		particle.delay = -1.0;

	// Write copy back to buffer
	particles[gl_VertexID] = particle;

	out_data.particle_color = particle.color;
	out_data.particle_position_vs = view * vec4(particle.position, 1.0);

	gl_Position = projection * out_data.particle_position_vs;
}
