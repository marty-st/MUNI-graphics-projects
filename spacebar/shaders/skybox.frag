#version 450

layout(binding = 0) uniform samplerCube skybox;

layout(location = 0) in vec3 fs_texture_coordinate;

layout(location = 0) out vec4 final_color;

void main()
{    
    final_color = texture(skybox, fs_texture_coordinate);
}