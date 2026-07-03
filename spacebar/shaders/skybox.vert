#version 450

layout(binding = 0, std140) uniform Camera {
	mat4 projection;
	mat4 view;
	vec3 position;
} camera;

layout(location = 0) uniform mat4 rotation;

layout(location = 0) in vec3 position;

layout(location = 0) out vec3 fs_texture_coordinate;

void main()
{
    fs_texture_coordinate = vec3(rotation * vec4(position, 1.0));
	mat4 viewWithoutTranslation = mat4(mat3(camera.view));
    gl_Position = camera.projection * viewWithoutTranslation * vec4(position, 1.0);
}  