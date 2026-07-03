#version 450

layout(binding = 0, std140) uniform Camera {
    mat4 projection;
    mat4 view;
    vec3 position;
} camera;

layout(binding = 1, std140) uniform Object {
    mat4 model_matrix;
    vec4 ambient_color;
    vec4 diffuse_color;
    vec4 specular_color;
} object;

struct LightPlus {
	vec4 position;
	vec4 ambient_color;
	vec4 diffuse_color;
	vec4 specular_color;
    vec4 cone_direction;
};

layout(binding = 2, std430) buffer Lights {
	LightPlus lights[];
};

layout(binding = 3) uniform sampler2D albedo_texture;
layout(binding = 4) uniform samplerCube skybox_texture;

layout(location = 1) uniform bool has_texture = false;
layout(location = 2) uniform mat4 skybox_rotation;

layout(location = 0) in vec3 fs_position;
layout(location = 1) in vec3 fs_normal;
layout(location = 2) in vec2 fs_texture_coordinate;

layout(location = 0) out vec4 final_color;

void main() {
    vec3 color_sum = vec3(0.0);
    
    for (int i = 0; i < lights.length(); ++i)
    {
        LightPlus light = lights[i];

        vec3 light_vector = light.position.xyz - fs_position * light.position.w;
        vec3 L = normalize(light_vector);
        vec3 N = normalize(fs_normal);
        vec3 E = normalize(camera.position - fs_position);
        vec3 H = normalize(L + E);

        float NdotL = max(dot(N, L), 0.0);
        float NdotH = max(dot(N, H), 0.0001);

        vec3 ambient = object.ambient_color.rgb * light.ambient_color.rgb;
        vec3 diffuse = object.diffuse_color.rgb * (has_texture ? texture(albedo_texture, fs_texture_coordinate).rgb : vec3(1.0)) *
                    light.diffuse_color.rgb;
        vec3 specular = object.specular_color.rgb * light.specular_color.rgb;

        // Environment Mapping - NOTE: Avoid low shininess on complex geometry, looks poopoodoodoo
        if (object.ambient_color.w > 0.0)
            specular = specular * texture(skybox_texture, vec3(skybox_rotation * vec4(H, 1.0))).rgb;

        vec3 color = ambient.rgb + NdotL * diffuse.rgb + pow(NdotH, object.specular_color.w) * specular;

        // Attenuation or Cone
        if (light.position.w == 1.0) 
        {
            float attenuation = dot(light_vector, light_vector);
            
            if (length(light.cone_direction) > 0.0)
            {
                vec3 d = normalize(light.cone_direction).xyz;
                vec3 v = -L;
                color *= pow(max(dot(d, v), 0.0), attenuation);
            }
            else
            {
                color /= attenuation;
            }
        }

        color = color / (color + 1.0);       // tone mapping
        color = pow(color, vec3(1.0 / 2.2)); // gamma correction

        color_sum += color;
    }

    final_color = vec4(color_sum, object.diffuse_color.w);
}
