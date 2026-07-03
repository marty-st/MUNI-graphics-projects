#pragma once
#include "glm_headers.hpp"

struct Material
{
    glm::vec4 ambient_color;
    glm::vec4 diffuse_color;
    // Contains shininess in .w element
    glm::vec4 specular_color;

    Material(glm::vec4 _ambient_color = glm::vec4(0.0f), glm::vec4 _diffuse_color = glm::vec4(1.0f), glm::vec4 _specular_color = glm::vec4(0.0f))
        : ambient_color{_ambient_color}, diffuse_color{_diffuse_color}, specular_color{_specular_color} {}
};

namespace mat
{
    static const Material chrome = Material(glm::vec4(0.25f), 
                                            glm::vec4(0.4f, 0.4f, 0.4f, 1.0f), 
                                            glm::vec4(0.774597f, 0.774597f, 0.774597f, 10.0f));

    static const Material dark_steel = Material(glm::vec4(0.0025f, 0.0025f, 0.0025f, 0.0f), 
                                                glm::vec4(0.05f, 0.05f, 0.05f, 1.0f), 
                                                glm::vec4(0.5f, 0.5f, 0.5f, 64.0f));

    static const Material window = Material(glm::vec4(0.05f),
                                            glm::vec4(0.25f, 0.25f, 0.25f, 0.2f),
                                            glm::vec4(1.0f, 1.0f, 1.0f, 10.0f));

    static const Material glass = Material(glm::vec4(0.0f),
                                           glm::vec4(0.15f, 0.15f, 0.15f, 0.1f),
                                           glm::vec4(1.0f, 1.0f, 1.0f, 128.0f));

    static const Material bottle = Material(glm::vec4(0.0f),
                                            glm::vec4(1.0f, 1.0f, 1.0f, 0.9f),
                                            glm::vec4(1.0f, 1.0f, 1.0f, 128.0f));

    static const Material steel = Material(glm::vec4(0.0f),
                                           glm::vec4(1.0f),
                                           glm::vec4(0.1f, 0.1f, 0.1f, 32.0f));

    static const Material iron = Material(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
                                          glm::vec4(1.0f),
                                          glm::vec4(0.4f, 0.4f, 0.4f, 256.0f));

    static const Material matte_iron = Material(glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),
                                                glm::vec4(1.0f),
                                                glm::vec4(0.1f, 0.1f, 0.1f, 32.0f));

    static const Material gold = Material(glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
                                          glm::vec4(1.0f),
                                          glm::vec4(0.62f, 0.55f, 0.36f, 51.2f));

    static const Material white_plastic = Material(glm::vec4(0.0f),
                                                   glm::vec4(1.0f),
                                                   glm::vec4(0.7f, 0.7f, 0.7f, 32.0f));

    static const Material red_plastic = Material(glm::vec4(0.0f),
                                                 glm::vec4(1.0f),
                                                 glm::vec4(0.7f, 0.6f, 0.6f, 32.0f));

    static const Material porcelain = Material(glm::vec4(0.0f),
                                               glm::vec4(1.0f),
                                               glm::vec4(0.9f, 0.9f, 0.9f, 256.0f));
                                               
    static const Material black_rubber = Material(glm::vec4(0.0f),
                                                  glm::vec4(1.0f),
                                                  glm::vec4(0.0f));
};

namespace color
{
    static const glm::vec3 white = glm::vec3(1.0f, 1.0f, 1.0f);
    static const glm::vec3 red = glm::vec3(1.0f, 0.0f, 0.0f);
    static const glm::vec3 green = glm::vec3(0.0f, 1.0f, 0.0f);
    static const glm::vec3 blue = glm::vec3(0.0f, 0.0f, 1.0f);
    static const glm::vec3 orange = glm::vec3(1.0f, 0.75f, 0.0f);
    static const glm::vec3 pink = glm::vec3(1.0f, 0.1f, 0.2f);
    static const glm::vec3 swamp = glm::vec3(0.0f, 0.78f, 0.561f);
    static const glm::vec3 light_blue = glm::vec3(0.255f, 0.514f, 0.961f);
};