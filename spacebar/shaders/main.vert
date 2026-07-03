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

layout(location = 0) uniform mat3 normal_matrix;

layout(location = 0) in vec3 position;
layout(location = 1) in vec3 normal;
layout(location = 2) in vec2 texture_coordinate;

layout(location = 0) out vec3 fs_position;
layout(location = 1) out vec3 fs_normal;
layout(location = 2) out vec2 fs_texture_coordinate;

void main()
{
	fs_position = vec3(object.model_matrix * vec4(position, 1.0));
	fs_normal = normal_matrix * normal;
	fs_texture_coordinate = texture_coordinate;

    gl_Position = camera.projection * camera.view * object.model_matrix * vec4(position, 1.0);
}
