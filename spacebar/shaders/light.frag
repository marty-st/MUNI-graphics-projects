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

layout(binding = 2) uniform sampler2D albedo_texture;
layout(location = 0) uniform bool has_texture = false;

layout(location = 0) in vec2 fs_texture_coordinate;

layout(location = 0) out vec4 final_color;

void main() 
{
    final_color = vec4(has_texture ? texture(albedo_texture, fs_texture_coordinate).rgb * 2 : object.ambient_color.rgb * 2, 1.0);
}