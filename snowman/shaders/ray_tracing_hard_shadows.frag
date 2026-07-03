#version 450 core

// ----------------------------------------------------------------------------
// Input Variables
// ----------------------------------------------------------------------------
in VertexData
{
	vec2 tex_coord;
} in_data;

// The UBO with camera data.	
layout (std140, binding = 0) uniform CameraBuffer
{
	mat4 projection;	  // The projection matrix.
	mat4 projection_inv;  // The inverse of the projection matrix.
	mat4 view;			  // The view matrix
	mat4 view_inv;		  // The inverse of the view matrix.
	mat3 view_it;		  // The inverse of the transpose of the top-left part 3x3 of the view matrix
	vec3 eye_position;	  // The position of the eye in world space.
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

// The material data.
struct PBRMaterialData
{
   vec3 diffuse;     // The diffuse color of the material.
   float roughness;  // The roughness of the material.
   vec3 f0;          // The Fresnel reflection at 0�.
};

layout (std140, binding = 3) uniform SnowManBuffer
{
    vec4 spheres[10];         // The spheres defining the snowman.
    PBRMaterialData materials[10]; // The respective materials for each sphere.
    int sphere_count;          // The number of spheres making up the snowman.
};

// Size of the rotating sphere lights.
layout (location = 0) uniform float sphere_light_radius;
// Ray bounce depth
layout (location = 1) uniform int reflections;
// Number of random shadow samples per shadow ray
layout (location = 2) uniform int shadow_samples;

// ----------------------------------------------------------------------------
// Output Variables
// ----------------------------------------------------------------------------
// The final output color.
layout (location = 0) out vec4 final_color;

// ----------------------------------------------------------------------------
// Ray Tracing Structures
// ----------------------------------------------------------------------------
// The definition of a ray.
struct Ray 
{
    vec3 origin;     // The ray origin.
    vec3 direction;  // The ray direction.
};
// The definition of an intersection.
struct RayIntersectionData
{
	float t;
	vec3 intersection;
	vec3 normal;
};
// The definition of an intersection including information about the hit object.
struct Hit 
{
    float t;				  // The distance between the ray origin and the intersection points along the ray. 
	vec3 intersection;        // The intersection point.
    vec3 normal;              // The surface normal at the interesection point.
	PBRMaterialData material; // The material of the object at the intersection point.
	int object_id;            // The id of the object at the intersection point.
};

const RayIntersectionData no_intersection = RayIntersectionData(1e20, vec3(0.0), vec3(0.0));
const Hit miss = Hit(1e20, vec3(0.0), vec3(0.0), PBRMaterialData(vec3(0),0,vec3(0)), 0);

// ----------------------------------------------------------------------------
// Local Methods
// ----------------------------------------------------------------------------

// The FresnelSchlick approximation of the reflection.
vec3 FresnelSchlick(in vec3 f0, in vec3 V, in vec3 H)
{
	float VdotH = clamp(dot(V, H), 0.0, 1.0);
	return f0 + (1.0 - f0) * pow(1.0 - VdotH, 5.0);
}

// Computes an intersection between a ray and a plane defined by its normal and one point inside the plane.
// ray - the ray definition (contains ray.origin and ray.direction)
// normal - the plane normal
// point - a point laying in the plane
Hit RayPlaneIntersection(const Ray ray, const vec3 normal, const vec3 point) 
{
	float t = dot(normal, (point - ray.origin)) / dot(normal, ray.direction);
	if (t < 0)
		return miss;
		
	vec3 intersection = ray.origin + t * ray.direction;

	// Finite plane - square
	if (abs(intersection.x) + abs(intersection.z) > 70)
		return miss;

    return Hit(t, intersection, normal, materials[0], 0);
}

// Computes an intersection between a ray and a sphere defined by its center and radius.
// ray - the ray definition (contains ray.origin and ray.direction)
// center - the center of the sphere
// radius - the radius of the sphere
// i - the index of the sphere in the array, can be used to obtain the material from materials buffer
RayIntersectionData RaySphereIntersection(const Ray ray, const vec3 center, const float radius) 
{
	// Optimalized version.
	vec3 center_to_origin = ray.origin - center;
	float b = dot(ray.direction, center_to_origin);
	float c = dot(center_to_origin, center_to_origin) - (radius*radius);

	float det = b*b - c;
	if (det < 0.0) 
        return no_intersection;

	float t = -b - sqrt(det);
	if (t < 0.0) 
        t = -b + sqrt(det);
	if (t < 0.0) 
        return no_intersection;

	vec3 intersection = ray.origin + t * ray.direction;
	vec3 normal = normalize(intersection - center);

	return RayIntersectionData(t, intersection, normal);
}

// Evaluates the intersections of the ray with the scene objects and returns the closest hit.
Hit Evaluate(const Ray ray)
{	
	// Floor
	Hit closest_hit = RayPlaneIntersection(ray, vec3(0, 1, 0), vec3(0));

	// Snowman
    for(int i = 0; i < sphere_count; ++i){
		RayIntersectionData sphere_intersection = RaySphereIntersection(ray, spheres[i].xyz, spheres[i].w);
		Hit sphere_hit = Hit(sphere_intersection.t, sphere_intersection.intersection, sphere_intersection.normal, materials[i], i+1);

		if (sphere_hit.t < closest_hit.t)
			closest_hit = sphere_hit;
	}

	// Sphere lights
    for(int i = 0; i < lights_count + 1; ++i){
		RayIntersectionData sphere_intersection = RaySphereIntersection(ray, lights[i].position.xyz, sphere_light_radius);
		Hit sphere_hit = Hit(sphere_intersection.t, sphere_intersection.intersection, sphere_intersection.normal, PBRMaterialData(lights[i].diffuse, 0.0, vec3(0.0)), -i-1);

		if (sphere_hit.t < closest_hit.t)
			closest_hit = sphere_hit;
	}
	
    return closest_hit;
}

// Evaluates the intersections of the ray with shadow occluders and returns the closest hit.
Hit EvaluateShadow(const Ray ray)
{
	Hit closest_hit = miss;
	// Snowman
    for(int i = 0; i < sphere_count; ++i)
	{
		RayIntersectionData sphere_intersection = RaySphereIntersection(ray, spheres[i].xyz, spheres[i].w);
		Hit sphere_hit = Hit(sphere_intersection.t, sphere_intersection.intersection, sphere_intersection.normal, materials[i], i+1);

		if (sphere_hit.t < closest_hit.t)
			closest_hit = sphere_hit;
	}

	return closest_hit;
}

// Traces the ray trough the scene and accumulates the color.
vec3 Trace(Ray ray) 
{
    // The accumulated color and attenuation used when tracing the rays through the scene.
	vec3 color = vec3(0.0);
    vec3 attenuation = vec3(1.0);
	const float epsilon = 1e-2;

	for (int i = 0; i < reflections; ++i) 
	{
		Hit hit = Evaluate(ray);

		// First intersection depth, costs ~ 0.1 FPS on default settings.
		if (i == 0) 
			gl_FragDepth = (1.0 / hit.t - 1.0) / -0.999; // (1.0 / hit.t - 1.0 / near) / (1.0 / far - 1.0 / near)
		
		if (hit.t == miss.t)
			break;
    	
		// Fresnel
		vec3 fresnel = FresnelSchlick(hit.material.f0, -ray.direction, hit.normal);
		// Color and Shadow
		if (hit.object_id < 0) // Hit sphere light
			color += hit.material.diffuse * (1.0 - fresnel) * attenuation;
		else
		{
			for (int i = 0; i < lights_count; ++i) 
			{
				// The direction towards the light.
				vec3 L = normalize(lights[i].position.xyz - hit.intersection);
				Ray to_light = Ray(hit.intersection + L * epsilon, L); // epsilon in the direction of the light
				float distance_to_light = length(lights[i].position.xyz - hit.intersection);

				// Snowman shadows only
				Hit shadow = EvaluateShadow(to_light);
				if (shadow.t == miss.t || distance_to_light < shadow.t)
				{
					vec3 local = lights[i].diffuse * max(0.0, dot(hit.normal, L)) * hit.material.diffuse * (1.0 - fresnel) * attenuation;
					float atten_factor = 1.0 / (1.0 + 0.5 * distance_to_light);
					color += local * atten_factor;
				}
			}
		}
		// Reflect the ray.
		vec3 reflected = reflect(ray.direction, hit.normal);
		ray = Ray(hit.intersection + hit.normal * epsilon, reflected); // hit.normal or reflected?
		
		// Attenuation
		attenuation *= fresnel * hit.material.diffuse;
    }

    return color;
}

// ----------------------------------------------------------------------------
// Main Method
// ----------------------------------------------------------------------------
void main()
{
	vec3 ray_origin = (view_inv * projection_inv * vec4(in_data.tex_coord * 2.0 - 1.0, -1.0, 1.0)).xyz;
    vec3 ray_direction = normalize(ray_origin - eye_position);

	int unused = shadow_samples; // To avoid warnings about unused variables.
	vec3 color = Trace(Ray(ray_origin, ray_direction));

	final_color = vec4(color, 1.0);
}
