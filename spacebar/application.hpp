// ################################################################################
// Common Framework for Computer Graphics Courses at FI MUNI.
//
// Copyright (c) 2021-2022 Visitlab (https://visitlab.fi.muni.cz)
// All rights reserved.
// ################################################################################

#pragma once

#include "camera.h"
#include "cube.hpp"
#include "pv112_application.hpp"
#include "sphere.hpp"
#include "teapot.hpp"

#include "frame.hpp"
#include "cameraplus.hpp"
#include "geometryplus.hpp"
#include "material.hpp"

// ----------------------------------------------------------------------------
// UNIFORM STRUCTS
// ----------------------------------------------------------------------------
struct CameraUBO {
    glm::mat4 projection;
    glm::mat4 view;
    glm::vec4 position;
};

struct LightUBO {
    /* Constructors default */
    LightUBO() = default; 
    /* Constructors - Point, Directional */
    LightUBO(glm::vec4 _position, glm::vec4 color)
    : position{_position}, ambient_color{color}, diffuse_color{color}, specular_color{color} {}
    LightUBO(glm::vec4 _position, glm::vec4 _ambient_color, glm::vec4 _diffuse_color, glm::vec4 _specular_color)
    : position{_position}, ambient_color{_ambient_color}, diffuse_color{_diffuse_color}, specular_color{_specular_color} {}
    /* Constructors - Cone */
    LightUBO(glm::vec4 _position, glm::vec4 color, glm::vec3 _cone_direction)
    : position{_position}, ambient_color{color}, diffuse_color{color}, specular_color{color}, cone_direction{glm::vec4(_cone_direction, 0.0f)} 
    {
        position.w = 1.0f;
    }
    LightUBO(glm::vec4 _position, glm::vec4 _ambient_color, glm::vec4 _diffuse_color, glm::vec4 _specular_color, glm::vec3 _cone_direction)
    : position{_position}, ambient_color{_ambient_color}, diffuse_color{_diffuse_color}, specular_color{_specular_color}, cone_direction{glm::vec4(_cone_direction, 0.0f)}
    {
        position.w = 1.0f;
    }

    /* Attributes */ 
    glm::vec4 position;
    glm::vec4 ambient_color;
    glm::vec4 diffuse_color;
    glm::vec4 specular_color;
    // If |cone_direction| = 0, then the light isn't considered a cone light
    glm::vec4 cone_direction = glm::vec4(0.0f);
};

struct alignas(256) ObjectUBO {
    glm::mat4 model_matrix;  // [  0 -  64) bytes
    glm::vec4 ambient_color; // [ 64 -  80) bytes
    glm::vec4 diffuse_color; // [ 80 -  96) bytes

    // Contains shininess in .w element
    glm::vec4 specular_color; // [ 96 - 112) bytes
};

// Constants
const float clear_color[4] = {0.0, 0.0, 0.0, 1.0};
const float clear_depth[1] = {1.0};

static const glm::vec4 ac = glm::vec4(0.1f, 0.1f, 0.2f, 0.0f);
static const glm::vec4 dc = glm::vec4(1.0f);
static const glm::vec4 sc = glm::vec4(0.0f);

class Application : public PV112Application {

    // ----------------------------------------------------------------------------
    // Variables
    // ----------------------------------------------------------------------------
    std::filesystem::path images_path;
    std::filesystem::path objects_path;

    // Main program
    GLuint main_program;
    // TODO: feel free to add as many as you need/like
    GLuint lightsource_program;
    GLuint skybox_program;

    // List of geometries used in the project
    std::vector<std::shared_ptr<GeometryPlus>> geometries;
    std::vector<std::shared_ptr<GeometryPlus>> spotlight_geometries;

    // Shared pointers are pointers that automatically count how many times they are used. When there are 0 pointers to the object pointed
    // by shared_ptrs, the object is automatically deallocated. Consequently, we gain 3 main properties:
    // 1. Objects are not unnecessarily copied
    // 2. We don't have to track pointers
    // 3. We don't have to deallocate these geometries
    // std::shared_ptr<GeometryPlus> default_sphere;
    // std::shared_ptr<GeometryPlus> bunny;

    std::shared_ptr<GeometryPlus> vented_floor;
    std::shared_ptr<GeometryPlus> windowed_wall;
    std::shared_ptr<GeometryPlus> edge_ceiling;
    std::shared_ptr<GeometryPlus> glass_window;
    std::shared_ptr<GeometryPlus> corridor_wall;
    std::shared_ptr<GeometryPlus> pillar;
    std::shared_ptr<GeometryPlus> door_floor;

    std::shared_ptr<GeometryPlus> spotlight;
    std::shared_ptr<GeometryPlus> sphere;
    GLuint spotlight_texture;

    std::shared_ptr<GeometryPlus> skybox_cube;

    // Default camera that rotates around center.
    // Camera camera;

    // My camera with extra functionality
    CameraPlus camera;

    //frames
    frame_ptr skybox_frame = std::make_shared<Frame>();

    // UBOs
    GLuint camera_buffer = 0;
    CameraUBO camera_ubo;

    GLuint objects_buffer = 0;
    std::vector<ObjectUBO> objects_ubos;

    GLuint spotlight_objects_buffer = 0;
    std::vector<ObjectUBO> spotlight_objects_ubos;

    std::vector<LightUBO> lights_ubos;
    GLuint lights_buffer = 0;

    // Textures
    std::vector<GLuint> textures;
    GLuint skybox_texture = 0;

    // ----------------------------------------------------------------------------
    // Constructors & Destructors
    // ----------------------------------------------------------------------------
  public:
    /**
     * Constructs a new @link Application with a custom width and height.
     *
     * @param 	initial_width 	The initial width of the window.
     * @param 	initial_height	The initial height of the window.
     * @param 	arguments	  	The command line arguments used to obtain the application directory.
     */
    Application(int initial_width, int initial_height, std::vector<std::string> arguments = {});

    /** Destroys the {@link Application} and releases the allocated resources. */
    ~Application() override;

    // ----------------------------------------------------------------------------
    // Initialization Methods
    // ----------------------------------------------------------------------------
    std::shared_ptr<GeometryPlus> createObject(geometry_ptr geometry, 
                                               Frame frame, Material material, 
                                               const std::filesystem::path texname = "", bool invert_y = true);
    std::shared_ptr<GeometryPlus> createObject(const std::filesystem::path objname, bool normalize, bool centralize, 
                                               Frame frame, Material material, 
                                               const std::filesystem::path texname = "", bool invert_y = true);
    void copyObject(Frame frame, std::shared_ptr<GeometryPlus> gp);
    void copyObject(Frame frame, int index = -1);

    void createLight(glm::vec4 position, glm::vec4 color, glm::vec3 cone_direction = glm::vec3(0.0f));
    void createLight(glm::vec4 position, 
                     glm::vec4 ambient_color, glm::vec4 diffuse_color, glm::vec4 specular_color, 
                     glm::vec3 cone_direction = glm::vec3(0.0f));

    std::shared_ptr<GeometryPlus> createSpotlightBase(Frame frame, bool rotates);
    std::shared_ptr<GeometryPlus> createSphereLight(std::shared_ptr<GeometryPlus> spot, glm::vec4 color, glm::vec3 cone_direction);
    std::pair<std::shared_ptr<GeometryPlus>, std::shared_ptr<GeometryPlus>> createSpotlight(Frame frame, glm::vec4 color, glm::vec3 cone_direction, bool rotates);
    
    void copySpotlightBase(std::shared_ptr<GeometryPlus> &spot, Frame frame, bool rotates);
    void copySphereLight(std::shared_ptr<GeometryPlus> spot, glm::vec4 color, glm::vec3 cone_direction);
    void copySpotlight(Frame frame, glm::vec4 color, glm::vec3 cone_direction, bool rotates);

    glm::vec3 translateLight(const glm::vec3 &coord);
    void linkObjectFrame(std::shared_ptr<GeometryPlus> source, std::shared_ptr<GeometryPlus> recipient);
    void linkObjectFrame();

    void createSkybox();
    void createScene();

    // ----------------------------------------------------------------------------
    // Update Methods
    // ----------------------------------------------------------------------------
    void updateObjectsUBOs();
    void updateSpotlightUBOs();
    void rotateSpotlights(float delta);

    /** @copydoc PV112Application::compile_shaders */
    void compile_shaders() override;

    void recompile_shaders();

    /** @copydoc PV112Application::delete_shaders */
    void delete_shaders() override;

    /** @copydoc PV112Application::update */
    void update(float delta) override;

    void render_skybox();
    void render_lightsources();
    void render_opaque();
    void render_transparent();

    /** @copydoc PV112Application::render */
    void render() override;

    void ui_camera_type();
    void ui_step_change();
    /** @copydoc PV112Application::render_ui */
    void render_ui() override;

    // ----------------------------------------------------------------------------
    // Input Events
    // ----------------------------------------------------------------------------

    /** @copydoc PV112Application::on_resize */
    void on_resize(int width, int height) override;

    /** @copydoc PV112Application::on_mouse_move */
    void on_mouse_move(double x, double y) override;

    /** @copydoc PV112Application::on_mouse_button */
    void on_mouse_button(int button, int action, int mods) override;

    /** @copydoc PV112Application::on_key_pressed */
    void on_key_pressed(int key, int scancode, int action, int mods) override;
};
