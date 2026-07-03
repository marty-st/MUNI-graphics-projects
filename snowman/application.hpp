#pragma once
#include "camera_ubo.hpp"
#include "light_ubo.hpp"
#include "pbr_material_ubo.hpp"
#include "pv227_application.hpp"
#include "ubo_impl.hpp" // required for UBO with snowman

#include <limits>

/** The number of spheres forming the snowman. */
constexpr int snowman_size = 10;

/** The structure defining the snowman. */
struct Snowman {
    glm::vec4 spheres[snowman_size];         // The spheres defining the snowman.
    PBRMaterialData materials[snowman_size]; // The respective materials for each sphere.
    int sphere_count = snowman_size;         // The number of spheres making up the snowman.
};

/** The definition of a snowman ubo. */
class SnowmanUBO : public UBO<Snowman> {
    using UBO<Snowman>::UBO; // copies constructors from the parent class
};

/** The structure defining a single particle. Uses padding to align with the std430 format used on the GPU. */
struct Particle
{
    glm::vec3 position = glm::vec3(FLT_MAX);    // 0-12
    float padding1;                             // align to 16
    glm::vec3 velocity = glm::vec3(0.0f);       // 16-28
    float padding2;                             // align to 32
    glm::vec3 color = glm::vec3(0.0f);          // 32-44
    int light_id;                               // 44-48
    float delay;                                // 48-52
    float padding3;                             // align to 64
    float padding4;
    float padding5;

};

class Application : public PV227Application {
    // ----------------------------------------------------------------------------
    // Variables (Geometry)
    // ----------------------------------------------------------------------------
  protected:
    /** The definition of the snowman. */
    Snowman snowman;
    /** The buffer with the snowman. */
    SnowmanUBO snowman_ubo;

    /** Array with particles */
    std::vector<Particle> particles;
    /** The buffer object with particles */
    GLuint particles_bo;

    /** The maximum number of particles for which the memory is allocated. */
    int max_snow_count = 131072;

    /** Size of rendered points when viewing the non-textured scene */
    float current_particle_size = 2.0f;
    float desired_particle_size = 2.0f;

    /** Size of the particle texture */
    float current_snow_size = 0.25f;
    float desired_snow_size = 0.25f;

    /** Stores delta from update function */
    float time_delta;
    /** Depends on `time_delta` but is adjusted so that particles fall at reasonable speeds */
    float simulation_time_delta;

    // ----------------------------------------------------------------------------
    // Variables (Textures)
    // ----------------------------------------------------------------------------
    GLuint particle_texture;
    GLuint star_texture;
    GLuint snowflake_texture;
  protected:
    // ----------------------------------------------------------------------------
    // Variables (Light)
    // ----------------------------------------------------------------------------
  protected:
    /** The UBO storing the data about lights - positions, colors, etc. */
    PhongLightsUBO phong_lights_ubo;
    /** The UBO defining a material that is used for lights during rasterization. */
    PhongMaterialUBO light_material_ubo;

    // ----------------------------------------------------------------------------
    // Variables (Camera)
    // ----------------------------------------------------------------------------
  protected:
    /** The UBO storing the information about camera. */
    CameraUBO camera_ubo;

    // ----------------------------------------------------------------------------
    // Variables (Shaders)
    // ----------------------------------------------------------------------------
    ShaderProgram ray_tracing_hs_program;
    ShaderProgram ray_tracing_ss_program;
    GLuint SNOWMAN_BINDING = 3;
    GLuint SPHERE_LIGHT_RADIUS_LOCATION = 0;
    GLuint REFLECTIONS_LOCATION = 1;
    GLuint SHADOW_SAMPLES_LOCATION = 2;

    ShaderProgram point_particles_program;
    GLuint PARTICLES_BUFFER_BINDING = 3;
    GLuint TIME_DELTA_LOCATION = 1;
    GLuint SIMULATION_TIME_DELTA_LOCATION = 2;

    ShaderProgram snow_particles_program;
    GLuint PARTICLE_SIZE_VS_LOCATION = 3;
  protected:
    // ----------------------------------------------------------------------------
    // Variables (Frame Buffers)
    // ----------------------------------------------------------------------------
  protected:
    // ----------------------------------------------------------------------------
    // Variables (GUI)
    // ----------------------------------------------------------------------------
  protected:
    /** The number of iterations. */
    int reflections = 3;

    /** The desired snow particle count. */
    int desired_snow_count = 4096;

    /** The current snow particle count. */
    int current_snow_count = 256;

    /** The flag determining if a snow should be visible. */
    bool show_particles = true;

    /** The number of shadow samples. */
    int shadow_samples = 16;

    /** The flag determining if an area light should be present. */
    float sphere_light_radius = 0.5f;

    /** The flag determining if the ambient occlusion should be used. */
    bool corrective_use_ambient_occlusion = true;

    /** Decides whether we update lights and particles */
    bool time_running = true;

    /** Decides whether we update lights */
    bool sphere_rotation = true;

    /** Decides whether we update particles */
    bool particles_falling = true;

    /** Particle falling speed */
    int simulation_speed = 30;

    /** The constants identifying what particle texure will displayed on the screen. */
    static const int DISPLAY_STAR_TEXTURE = 0;
    static const int DISPLAY_SNOWFLAKE_TEXTURE = 1;
    /** The GUI labels for the constants above. */
    const char* TEXTURE_LABELS[2] = {"Star", "Snowflake"};
    /** The flag determining what texture will be displayed on the screen right now. */
    int texture_to_display = DISPLAY_SNOWFLAKE_TEXTURE;

    /** The constants identifying what can be displayed on the screen. */
    static const int DISPLAY_INITIAL_SCENE = 0;
    static const int DISPLAY_RT_HS_SCENE = 1;
    static const int DISPLAY_RT_SCENE = 2;
    static const int DISPLAY_RT_PTS_SCENE = 3;
    static const int DISPLAY_FINAL_SCENE = 4;
    /** The GUI labels for the constants above. */
    const char* DISPLAY_LABELS[5] = {"Initial Scene", "RT Hard Shadows", "RT Scene", "RT + Points", "Final Scene"};

    /** The flag determining what will be displayed on the screen right now. */
    int what_to_display = DISPLAY_FINAL_SCENE;

    // ----------------------------------------------------------------------------
    // Constructors
    // ----------------------------------------------------------------------------
  public:
    Application(int initial_width, int initial_height, std::vector<std::string> arguments = {});

    /** Destroys the {@link Application} and releases the allocated resources. */
    virtual ~Application();

    // ----------------------------------------------------------------------------
    // Shaders
    // ----------------------------------------------------------------------------
    /**
     * {@copydoc PV227Application::compile_shaders}
     */
    void compile_shaders() override;

    // ----------------------------------------------------------------------------
    // Initialize Scene
    // ----------------------------------------------------------------------------
  public:
    /** Prepares the required cameras. */
    void prepare_cameras();

    /** Prepares the required materials. */
    void prepare_materials();

    /** Prepares the required textures. */
    void prepare_textures();

    /** Prepares the lights. */
    void prepare_lights();

    /** Builds a snowman from individual parts. */
    void prepare_snowman();

    /** Prepares the scene objects. */
    void prepare_scene();

    /** Prepares the frame buffer objects. */
    void prepare_framebuffers();

    /** Resizes the full screen textures match the window. */
    void resize_fullscreen_textures();

    // ----------------------------------------------------------------------------
    // Update
    // ----------------------------------------------------------------------------
    /**
     * Converts color in HSV to RGB.
     *
     * @param 	h	The hue [0-360].
     * @param 	s	The saturation [0-1].
     * @param 	v	The value [0-1].
     *
     * @return	The converted RGC color.
     */
    glm::vec3 HSVtoRGB(float h, float s, float v);

    /**
     * {@copydoc PV227Application::update}
     */
    void update(float delta) override;

    // ----------------------------------------------------------------------------
    // Render
    // ----------------------------------------------------------------------------
  public:
    /** @copydoc PV227Application::render */
    void render() override;

    /** Renders the snowman using rasterization. */
    void raster_snowman();
    /** Renders the snowman using ray tracing. */
    void raytrace_snowman(const ShaderProgram &rt_program);
    /** Renders and simulates the snow particles. */
    void raster_particles(const ShaderProgram &particles_program);
    // ----------------------------------------------------------------------------
    // GUI
    // ----------------------------------------------------------------------------
  public:
    /** @copydoc PV227Application::render_ui */
    void render_ui() override;

    // ----------------------------------------------------------------------------
    // Input Events
    // ----------------------------------------------------------------------------
  public:
    /** @copydoc PV227Application::on_resize */
    void on_resize(int width, int height) override;
};
