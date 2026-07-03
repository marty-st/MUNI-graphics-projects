#include "application.hpp"

#include <memory>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using std::make_shared;

GLuint load_texture_2d(const std::filesystem::path filename) {
    int width, height, channels;
    unsigned char* data = stbi_load(filename.generic_string().data(), &width, &height, &channels, 4);

    GLuint texture;
    glCreateTextures(GL_TEXTURE_2D, 1, &texture);

    glTextureStorage2D(texture, static_cast<GLsizei>(std::log2(width)), GL_RGBA8, width, height);

    glTextureSubImage2D(texture,
                        0,                         //
                        0, 0,                      //
                        width, height,             //
                        GL_RGBA, GL_UNSIGNED_BYTE, //
                        data);

    stbi_image_free(data);

    glGenerateTextureMipmap(texture);

    glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return texture;
}

GLuint load_texture_2d(const std::filesystem::path filename, bool invert_y)
{
    stbi_set_flip_vertically_on_load(invert_y);
    return load_texture_2d(filename);
}

unsigned int load_texture_cube_map(const std::vector<std::filesystem::path> paths)
{
    stbi_set_flip_vertically_on_load(false);

    GLuint texture;
    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &texture);

    glTextureStorage2D(texture, static_cast<GLsizei>(std::log2(2048)) + 1, GL_RGBA8, 2048, 2048);

    assert(paths.size() == 6);
    for (unsigned int i = 0; i < paths.size(); ++i)
    {
        int width, height, channels;
        unsigned char* data = stbi_load(paths[i].generic_string().data(), &width, &height, &channels, 4);
        if (data)
        {
            glTextureSubImage3D(texture,
                                0, // Mipmap level
                                0, 0, i, // (x, y, face) offset
                                2048, 2048, 1, // (w, h, depth)
                                GL_RGBA,
                                GL_UNSIGNED_BYTE,
                                data);

            stbi_image_free(data);
        }
        else
        {
            std::cout << "Cubemap tex failed to load at path: " << paths[i] << std::endl;
            stbi_image_free(data);
        }
    }
    glGenerateTextureMipmap(texture);

    // glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR); // Less flickering but looks not as sharp
    glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
    return texture;
}  

Application::Application(int initial_width, int initial_height, std::vector<std::string> arguments)
    : PV112Application(initial_width, initial_height, arguments) {
    this->width = initial_width;
    this->height = initial_height;

    // Configure fixed function pipeline
    glEnable(GL_MULTISAMPLE);

    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);

    glEnable(GL_DEPTH_TEST);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    // --------------------------------------------------------------------------
    // Initialize UBO Data
    // --------------------------------------------------------------------------
    camera_ubo.position = glm::vec4(camera.get_eye_position(), 1.0f);
    camera_ubo.projection = glm::perspective(glm::radians(45.0f), float(width) / float(height), 0.01f, 1000.0f);
    camera_ubo.view = glm::lookAt(camera.get_eye_position(), camera.getTarget(), glm::vec3(0.0f, 1.0f, 0.0f));

    // Lord have mercy
    createScene();
    createSkybox();

    // --------------------------------------------------------------------------
    // Create Buffers
    // --------------------------------------------------------------------------
    glCreateBuffers(1, &camera_buffer);
    glNamedBufferStorage(camera_buffer, sizeof(CameraUBO), &camera_ubo, GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &objects_buffer);
    glNamedBufferStorage(objects_buffer, sizeof(ObjectUBO) * objects_ubos.size(), objects_ubos.data(), GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &spotlight_objects_buffer);
    glNamedBufferStorage(spotlight_objects_buffer, sizeof(ObjectUBO) * spotlight_objects_ubos.size(), spotlight_objects_ubos.data(), GL_DYNAMIC_STORAGE_BIT);

    glCreateBuffers(1, &lights_buffer);
    glNamedBufferStorage(lights_buffer, lights_ubos.size() * sizeof(LightUBO), lights_ubos.data(), GL_DYNAMIC_STORAGE_BIT);

    compile_shaders();
}

Application::~Application() {
    delete_shaders();

    glDeleteBuffers(1, &camera_buffer);
    glDeleteBuffers(1, &objects_buffer);
    glDeleteBuffers(1, &spotlight_objects_buffer);
    glDeleteBuffers(1, &lights_buffer);
}

// ----------------------------------------------------------------------------
// Methods
// ----------------------------------------------------------------------------

std::shared_ptr<GeometryPlus> Application::createObject(geometry_ptr geometry, Frame frame, Material material, const std::filesystem::path texname, bool invert_y)
{
    // TO DO: use normal maps for normals
    bool has_texture = texname == "" ? false : true;
    geometries.emplace_back(std::make_shared<GeometryPlus>(geometry, frame, material));
    auto object = geometries.back();
    object->index = geometries.size() - 1;

    if (has_texture)
    {
        object->has_texture = true;
        textures.emplace_back(load_texture_2d(images_path / texname, invert_y));
        object->bound_texture = textures.back();
    }

    objects_ubos.push_back({.model_matrix = object->frame->getModelMat(),
                            .ambient_color = object->material.ambient_color,
                            .diffuse_color = object->material.diffuse_color,
                            .specular_color = object->material.specular_color});
    return object;
}

std::shared_ptr<GeometryPlus> Application::createObject(const std::filesystem::path objname, bool normalize, bool centralize, Frame frame, Material material, const std::filesystem::path texname, bool invert_y)
{
    return createObject(std::make_shared<Geometry>(GeometryPlus::from_file(objects_path / objname, normalize, centralize)), frame, material, texname, invert_y);
}

void Application::copyObject(Frame frame, std::shared_ptr<GeometryPlus> gp)
{
    geometries.emplace_back(std::make_shared<GeometryPlus>(gp->geometry, frame, gp->material));
    auto object = geometries.back();
    object->index = geometries.size() - 1;

    object->has_texture = gp->has_texture;
    if (object->has_texture)
        object->bound_texture = gp->bound_texture;

    objects_ubos.push_back({.model_matrix = object->frame->getModelMat(),
                            .ambient_color = object->material.ambient_color,
                            .diffuse_color = object->material.diffuse_color,
                            .specular_color = object->material.specular_color});
}

void Application::copyObject(Frame frame, int index)
{
    if (glm::abs(index) > geometries.size() || index >= static_cast<int>(geometries.size()) || geometries.empty())
    {
        std::cout << "Invalid copyObject index\n";
        return;
    }
    if (index < 0)
        copyObject(frame, geometries[geometries.size() + index]);
    else
        copyObject(frame, geometries[index]);
}

void Application::createLight(glm::vec4 position, glm::vec4 color, glm::vec3 cone_direction)
{
    if (glm::length(cone_direction) == 0.0f)
        lights_ubos.emplace_back(position, color);
    else
        lights_ubos.emplace_back(position, color, cone_direction);
}

void Application::createLight(glm::vec4 position, glm::vec4 ambient_color, glm::vec4 diffuse_color, glm::vec4 specular_color, glm::vec3 cone_direction)
{
    if (glm::length(cone_direction) == 0.0f)
        lights_ubos.emplace_back(position, ambient_color, diffuse_color, specular_color);
    else
        lights_ubos.emplace_back(position, ambient_color, diffuse_color, specular_color, cone_direction);
}

std::shared_ptr<GeometryPlus> Application::createSpotlightBase(Frame frame, bool rotates)
{
    geometry_ptr spot_geometry = std::make_shared<Geometry>(GeometryPlus::from_file(objects_path / "spotlight.obj", false, false));
    spotlight_geometries.emplace_back(std::make_shared<GeometryPlus>(spot_geometry, frame, mat::steel));

    auto spot = spotlight_geometries.back();
    spot->index = spotlight_geometries.size() - 1;

    spot->has_texture = true;
    spotlight_texture = load_texture_2d(images_path / "spotlight.png", true);
    spot->bound_texture = spotlight_texture;

    spot->animated = rotates;

    spotlight_objects_ubos.push_back({.model_matrix = spot->frame->getModelMat(),
                                      .ambient_color = spot->material.ambient_color,
                                      .diffuse_color = spot->material.diffuse_color,
                                      .specular_color = spot->material.specular_color});

    return spot;
}

std::shared_ptr<GeometryPlus> Application::createSphereLight(std::shared_ptr<GeometryPlus> spot, glm::vec4 color, glm::vec3 cone_direction)
{
    geometry_ptr sphere_geometry = std::make_shared<Geometry>(GeometryPlus::from_file(objects_path / "light_sphere.obj", false, false));
    spotlight_geometries.emplace_back(std::make_shared<GeometryPlus>(sphere_geometry, spot->frame, Material{color, glm::vec4(0.0f), glm::vec4(0.0f)}));
    auto sphere = spotlight_geometries.back();
    sphere->index = spotlight_geometries.size() - 1;

    sphere->animated = spot->animated;
    sphere->lightsource = true;

    spotlight_objects_ubos.push_back({.model_matrix = sphere->frame->getModelMat(),
                                      .ambient_color = sphere->material.ambient_color,
                                      .diffuse_color = sphere->material.diffuse_color,
                                      .specular_color = sphere->material.specular_color});

    glm::vec4 position = glm::vec4(translateLight(spot->frame->getPosition()), 1.0f);
    createLight(position, color, cone_direction);
    // Used to bind a corresponding light index for cone rotations
    sphere->bound_texture = static_cast<GLuint>(lights_ubos.size() - 1);

    return sphere;
}

std::pair<std::shared_ptr<GeometryPlus>, std::shared_ptr<GeometryPlus>> Application::createSpotlight(Frame frame, glm::vec4 color, glm::vec3 cone_direction, bool rotates)
{
    auto spot = createSpotlightBase(frame, rotates);
    auto sphere = createSphereLight(spot, color, cone_direction);
    
    return {spot, sphere};
}

void Application::copySpotlightBase(std::shared_ptr<GeometryPlus> &spot, Frame frame, bool rotates)
{
    spotlight_geometries.emplace_back(std::make_shared<GeometryPlus>(spotlight->geometry, frame, spotlight->material));
    spot = spotlight_geometries.back();
    spot->index = spotlight_geometries.size() - 1;

    spot->has_texture = spotlight->has_texture;
    if (spot->has_texture)
        spot->bound_texture = spotlight->bound_texture;

    spot->animated = rotates;

    spotlight_objects_ubos.push_back({.model_matrix = spot->frame->getModelMat(),
                                      .ambient_color = spot->material.ambient_color,
                                      .diffuse_color = spot->material.diffuse_color,
                                      .specular_color = spot->material.specular_color});
}

void Application::copySphereLight(std::shared_ptr<GeometryPlus> spot, glm::vec4 color, glm::vec3 cone_direction)
{
    spotlight_geometries.emplace_back(std::make_shared<GeometryPlus>(sphere->geometry, spot->frame, Material{color, color, glm::vec4(0.0f)}));
    sphere = spotlight_geometries.back();
    sphere->index = spotlight_geometries.size() - 1;

    sphere->animated = spot->animated;
    sphere->lightsource = true;

    spotlight_objects_ubos.push_back({.model_matrix = sphere->frame->getModelMat(),
                                      .ambient_color = sphere->material.ambient_color,
                                      .diffuse_color = sphere->material.diffuse_color,
                                      .specular_color = sphere->material.specular_color});

    glm::vec4 position = glm::vec4(translateLight(spot->frame->getPosition()), 1.0f);
    createLight(position, color, cone_direction);
    // Used to bind a corresponding light index for cone rotations
    sphere->bound_texture = static_cast<GLuint>(lights_ubos.size() - 1);
}

void Application::copySpotlight(Frame frame, glm::vec4 color, glm::vec3 cone_direction, bool rotates)
{
    auto spot = std::make_shared<GeometryPlus>();
    copySpotlightBase(spot, frame, rotates);
    copySphereLight(spot, color, cone_direction);
}

glm::vec3 Application::translateLight(const glm::vec3 &coord)
{
    // coord.x += 0.178062f;
    // coord.y -= 0.52517f;

    return glm::vec3(coord.x + 0.178062f, coord.y - 0.52517f, coord.z);
}

void Application::linkObjectFrame(std::shared_ptr<GeometryPlus> source, std::shared_ptr<GeometryPlus> recipient)
{
    recipient->frame = source->frame;
}

void Application::linkObjectFrame()
{
    if (geometries.size() < 2)
        return;
    linkObjectFrame(geometries[geometries.size() - 2], geometries.back());
    std::cout << "Took source Frame pos: " << glm::to_string(geometries[geometries.size() - 2]->frame->getPosition());
    std::cout << " and gave it to recipient: " << glm::to_string(geometries.back()->frame->getPosition()) << std::endl;
}

void Application::createSkybox()
{
    skybox_cube = std::make_shared<GeometryPlus>(Geometry::from_file(objects_path / "cube.obj"), Frame{}, Material{});

    auto skybox_folder = images_path / "cube_map/blue";
    const std::vector<std::filesystem::path> skybox_paths =
    {
        skybox_folder / "bkg1_right.png",
        skybox_folder / "bkg1_left.png",
        skybox_folder / "bkg1_top.png",
        skybox_folder / "bkg1_bot.png",
        skybox_folder / "bkg1_front.png",
        skybox_folder / "bkg1_back.png"
    };
    skybox_texture = load_texture_cube_map(skybox_paths);

    skybox_cube->bound_texture = skybox_texture;
}

void Application::createScene()
{
    images_path = lecture_folder_path / "images";
    objects_path = lecture_folder_path / "objects";

    // --------------------------------------------------------------------------
    //  Load/Create Objects
    // --------------------------------------------------------------------------
    // geometries.push_back(std::make_shared<GeometryPlus>(std::make_shared<Geometry>(Sphere()), Frame{}, false));
    // // You can use from_file function to load a Geometry from .obj file
    // geometries.push_back(make_shared<GeometryPlus>(Geometry::from_file(objects_path / "bunny.obj"), Frame{}, true));

    // Light Sphere
    // createObject(std::make_shared<Geometry>(Sphere()),
    //                       Frame{glm::vec3(0.0f, 3.0f, 2.0f), glm::quat(), glm::vec3(0.1f)}, 
    //                       Material{glm::vec4(1.0f, 1.0f, 0.0f, 1.0f), glm::vec4(0.0f), sc});
    // Bunny
    // createObject("bunny.obj", true, true,
    //              Frame{}, 
    //              Material{ac, dc, glm::vec4(1.0f, 1.0f, 1.0f, 32.0f)}, 
    //              "bunny.jpg", false);

    // Wall Right Side
    auto coord = glm::vec3(14.987f, 2.3365f, 11.138f);
    auto rot = glm::vec3(0.0f);
    auto scale = glm::vec3(1.0f);
    auto block_dist = 7.4f;
    windowed_wall = createObject("wall_block/wall.obj", false, true,
                                 Frame{coord}, mat::dark_steel);
    
    for (int i = 1; i <= 3; ++i)
        copyObject(Frame{glm::vec3(coord.x, coord.y, coord.z - i * block_dist)});

    coord = glm::vec3(11.0522f, 0.0f, 11.138f);
    vented_floor = createObject("wall_block/floor.obj", false, true,
                                Frame{(coord)}, mat::dark_steel);
    for (int i = 1; i <= 3; ++i)
        copyObject(Frame{glm::vec3(coord.x, coord.y, coord.z - i * block_dist)});

    coord = glm::vec3(10.974f, 4.516f, 11.0f);
    scale = glm::vec3(1.0f, 1.0f, 1.05f);
    edge_ceiling = createObject("wall_block/ceiling.obj", false, true,
                                Frame{coord, rot, scale}, mat::dark_steel);
    for (int i = 1; i <= 3; ++i)
        copyObject(Frame{glm::vec3(coord.x, coord.y, coord.z - i * block_dist), rot, scale});

    coord = glm::vec3(15.5f, 2.3706f, 11.138f);
    glass_window = createObject("wall_block/window.obj", false, true,
                                Frame{coord}, mat::window);
    for (int i = 1; i <= 3; ++i)
        copyObject(Frame{glm::vec3(coord.x, coord.y, coord.z - i * block_dist)});

    // Wall Left Side
    coord = glm::vec3(-14.987f, 2.3365f, 11.138f);
    rot = glm::vec3(0.0f, 180.0f, 0.0f);
    block_dist = 7.4f;
    for (int i = 0; i <= 3; ++i)
        copyObject(Frame{glm::vec3(coord.x, coord.y, coord.z - i * block_dist), rot}, windowed_wall);

    coord = glm::vec3(-11.0522f, 0.0f, 11.138f);
    for (int i = 0; i <= 3; ++i)
        copyObject(Frame{glm::vec3(coord.x, coord.y, coord.z - i * block_dist), rot}, vented_floor);

    coord = glm::vec3(-10.974f, 4.516f, 11.354f);
    scale = glm::vec3(1.0f, 1.0f, 1.05f);
    for (int i = 0; i <= 3; ++i)
        copyObject(Frame{glm::vec3(coord.x, coord.y, coord.z - i * block_dist), rot, scale}, edge_ceiling);

    coord = glm::vec3(-15.5f, 2.3706f, 11.138f);
    for (int i = 0; i <= 3; ++i)
        copyObject(Frame{glm::vec3(coord.x, coord.y, coord.z - i * block_dist), rot}, glass_window);

    // Opposite Floor and Wall
    coord = glm::vec3(3.67773f, 0.0f, -11.189f);
    rot = glm::vec3(0.0f, 90.0f, 0.0f);
    block_dist = 7.4f;
    copyObject(Frame{coord, rot}, vented_floor);
    copyObject(Frame{glm::vec3(coord.x - block_dist, coord.y, coord.z), rot}, vented_floor);

    coord = glm::vec3(3.6773f, 2.3365f, -15.0f);
    copyObject(Frame{coord, rot}, windowed_wall);
    copyObject(Frame{glm::vec3(coord.x - block_dist, coord.y, coord.z), rot}, windowed_wall);
    copyObject(Frame{glm::vec3(coord.x - 2 * block_dist, coord.y, coord.z), rot}, windowed_wall);
    copyObject(Frame{glm::vec3(coord.x + block_dist, coord.y, coord.z), rot}, windowed_wall);

    coord = glm::vec3(3.6773f, 2.3706f, -15.476f);
    copyObject(Frame{coord, rot}, glass_window);
    copyObject(Frame{glm::vec3(coord.x - block_dist, coord.y, coord.z), rot}, glass_window);
    copyObject(Frame{glm::vec3(coord.x - 2 * block_dist, coord.y, coord.z), rot}, glass_window);
    copyObject(Frame{glm::vec3(coord.x + block_dist, coord.y, coord.z), rot}, glass_window);

    // Floor and Walls At Entrance
    coord = glm::vec3(3.6585f, 0.0f, 11.099f);
    rot = glm::vec3(0.0f, -90.0f, 0.0f);
    block_dist = 7.4f;
    
    copyObject(Frame{coord, rot}, vented_floor);
    copyObject(Frame{glm::vec3(coord.x - block_dist, coord.y, coord.z), rot}, vented_floor);

    coord = glm::vec3(11.0522f, 2.3365f, 15.0f);
    copyObject(Frame{coord, rot}, windowed_wall);
    copyObject(Frame{glm::vec3(-coord.x, coord.y, coord.z), rot}, windowed_wall);

    coord = glm::vec3(11.0522f, 2.3706f, 15.476f);
    copyObject(Frame{coord, rot}, glass_window);
    copyObject(Frame{glm::vec3(-coord.x, coord.y, coord.z), rot}, glass_window);

    // Corners
    coord = glm::vec3(14.913f, 2.4104f, 15.18f);
    pillar = createObject("wall_block/pillar.obj", false, true,
                          Frame{coord}, mat::dark_steel);

    copyObject(Frame{glm::vec3(coord.x, coord.y, -coord.z)});
    rot = glm::vec3(0.0f, 180.0f, 0.0f);
    copyObject(Frame{glm::vec3(-coord.x, coord.y, coord.z), rot});
    copyObject(Frame{glm::vec3(-coord.x, coord.y, -coord.z), rot});

    // Door
    coord = glm::vec3(0.0f, 1.7473f, 14.903f);
    createObject("corridor/frame_door.obj", false, true,
                 Frame{coord}, mat::steel, 
                 "corridor/frame_door_BaseColor.png", true);

    coord = glm::vec3(1.0458f, 1.5209f, 15.166f);
    createObject("corridor/R_door.obj", false, true,
                 Frame{coord}, mat::steel, 
                 "corridor/R_door_BaseColor.png", true);

    coord = glm::vec3(-0.92898f, 1.521f, 15.166f);
    createObject("corridor/L_door.obj", false, true,
                 Frame{coord}, mat::steel, 
                 "corridor/L_door_BaseColor.png", true);

    coord = glm::vec3(0.0f, -0.05f, 14.861f);
    scale = glm::vec3(3.0f, 1.0f, 1.0f);
    door_floor = createObject("corridor/door floor.obj", false, true,
                              Frame{coord, glm::vec3(0.0f), scale}, mat::steel, 
                              "corridor/door floor_BaseColor.png", true);

    // Fix Holes
    coord = glm::vec3(11.0522f, 4.7f, 15.0f);
    rot = glm::vec3(-90.0f, 0.0f, 0.0f);
    scale = glm::vec3(1.3f);
    copyObject(Frame{coord, rot, scale}, door_floor);
    copyObject(Frame{glm::vec3(-coord.x, coord.y, coord.z), rot, scale}, door_floor);
    rot = glm::vec3(90.0f, 0.0f, 0.0f);
    copyObject(Frame{glm::vec3(coord.x, coord.y, -coord.z), rot, scale}, door_floor);
    copyObject(Frame{glm::vec3(-coord.x, coord.y, -coord.z), rot, scale}, door_floor);

    coord = glm::vec3(0.0f, 4.15f, 14.903f);
    rot = glm::vec3(-90.0f, 0.0f, 0.0f);
    scale = glm::vec3(1.1f, 1.0f, 1.0f);
    copyObject(Frame{coord, rot, scale}, door_floor);

    coord = glm::vec3(6.2f, 2.3176f, 14.95f);
    createObject("wall_block/plane.obj", false, true,
                 Frame{coord}, mat::dark_steel);

    copyObject(Frame{glm::vec3(-coord.x + 1.8f, coord.y, coord.z)}); // WTF

    coord = glm::vec3(7.3311, 2.4104f, 15.18f);
    rot = glm::vec3(0.0f, -90.0f, 0.0f);
    copyObject(Frame{coord, rot}, pillar);
    copyObject(Frame{glm::vec3(-coord.x, coord.y, coord.z), rot}, pillar);
    coord.x = 3.0f;
    copyObject(Frame{coord, rot}, pillar);
    copyObject(Frame{glm::vec3(-coord.x, coord.y, coord.z), rot}, pillar);
    
    // Center Floor and Underside
    coord = glm::vec3(0.0f, -0.5f, 0.0f);
    createObject("bar/ground_module.obj", false, true,
                 Frame{coord, glm::vec3(0.0f), glm::vec3(6.0f)}, mat::black_rubber, 
                 "ground_module.png");

    coord = glm::vec3(4.9f, 0.0f, 5.3891f);
    for (int z = 0; z <= 3; ++z)
    {
        float z_dist = 4.08f;
        for (int x = 0; x <= 2; ++x)
        {
            float x_dist = 4.88f;
            copyObject(Frame{glm::vec3(coord.x - x * x_dist, coord.y, coord.z - z * z_dist)});
        }
    }

    // Bar Furniture
    coord = glm::vec3(0.0f);
    // ABSOLUTE COORDS
    createObject("bar/big/T_BarDesk.obj", false, false,
                 Frame{coord}, Material{}, 
                 "bar/big/T_BarDesk_Base.png");

    createObject("bar/big/BarDeskSide.obj", false, false,
                 Frame{coord}, Material{}, 
                 "bar/big/Bar_Corner_Base.png");

    createObject("bar/big/BarVendingMachine.obj", false, false,
                 Frame{coord}, mat::iron, 
                 "bar/big/BarVendingMachine_Base.png");

    createObject("bar/big/VendingMachine01.obj", false, false,
                 Frame{coord}, mat::iron, 
                 "bar/big/VendingMachine01_Base.png");

    // CENTERED COORDS
    coord = glm::vec3(-1.1059f, 0.05f, 0.45598f);
    createObject("bar/big/SM_BarGround.obj", false, true,
                 Frame{coord}, Material{}, 
                 "bar/big/SM_BarGround_Base.png");
                
    coord = glm::vec3(0.0f, 0.69606f, 1.3339f);
    createObject("bar/big/SM_Bardesk.obj", false, true,
                 Frame{coord}, Material{}, 
                 "bar/big/SM_Bardesk_Base.png");

    block_dist = 4.05f;
    copyObject(Frame{glm::vec3(coord.x, coord.y, coord.z - block_dist)});

    coord = glm::vec3(-0.12316f, 0.7081f, 3.6781f);
    createObject("bar/big/Bar_Corner.obj", false, true,
                 Frame{coord}, mat::iron, 
                 "bar/big/Bar_Corner_Base.png");

    coord.x = -2.0588f;
    rot = glm::vec3(0.0f, -90.f, 0.0f);
    copyObject(Frame{coord, rot});

    coord = glm::vec3(1.0f, 0.7875f, 2.9462f);
    createObject("bar/big/CyberChair.obj", false, true,
                 Frame{coord}, mat::matte_iron, 
                 "bar/big/CyberChair_Base.png");

    block_dist = 0.95f;
    for (int i = 1; i <= 6; ++i)
        copyObject(Frame{glm::vec3(coord.x, coord.y, coord.z - i * block_dist)});

    coord.z -= block_dist / 2.0f;
    coord.x = -3.05f;
    rot = glm::vec3(0.0f, 180.0f, 0.0f);
    for (int i = 0; i <= 3; ++i)
        copyObject(Frame{glm::vec3(coord.x, coord.y, coord.z - i * block_dist), rot});

    // Bar Ceiling - ABSOLUTE COORDS
    coord = glm::vec3(0.0f);
    createObject("bar/top/Bar_Ceiling.obj", false, false,
                 Frame{coord}, mat::iron, 
                 "bar/top/Bar_Ceiling_and_Hang_Base.png");
    
    createObject("bar/top/Bar_Hang.obj", false, false,
                 Frame{coord}, mat::iron, 
                 "bar/top/Bar_Ceiling_and_Hang_Base.png");

    createObject("bar/top/Bar_Pillars.obj", false, false,
                 Frame{coord}, mat::iron, 
                 "bar/top/Bar_Pillars_Base.png");

    createObject("bar/top/Bar_Shroud.obj", false, false,
                 Frame{coord}, mat::iron, 
                 "bar/top/Bar_Shroud_Base.png");

    createObject("bar/top/ChineseBaloon.obj", false, false,
                 Frame{coord}, Material{}, 
                 "bar/top/ChineseBaloon02_Base.png");
    geometries.back()->lightsource = true;

    block_dist = 0.97f;
    for (int i = 1; i <= 2; ++i)
    {
        copyObject(Frame{glm::vec3(coord.x, coord.y, coord.z - i * block_dist)});
        geometries.back()->lightsource = true;
    }

    // Bar Items
    // ABSOLUTE COORDINATES
    createObject("bar/small/Coffee_Machine.obj", false, false,
                 Frame{coord}, mat::matte_iron, 
                 "bar/small/Coffee_Machine_Base.png");

    createObject("bar/small/Oven.obj", false, false,
                 Frame{coord}, mat::matte_iron, 
                 "bar/small/Oven_Base.png");

    createObject("bar/small/Toaster.obj", false, false,
                 Frame{coord}, mat::iron, 
                 "bar/small/Toaster_Base.png");

    // CENTRALIZED COORDINATES
    coord = glm::vec3(-1.1872f, 1.37f, 3.28f);
    createObject("bar/small/SM_BeerBarrel.obj", false, true,
                 Frame{coord}, mat::iron, 
                 "bar/small/SM_BeerBarrel_Base.png");

    block_dist = 0.27f;
    for (int i = 1; i <= 2; ++i)
        copyObject(Frame{glm::vec3(coord.x, coord.y, coord.z - i * block_dist)});
    
    coord = glm::vec3(-0.03f, 1.37f, 0.98f);
    createObject("bar/small/SM_BMalibuBottle.obj", false, true,
                 Frame{coord}, mat::bottle, 
                 "bar/small/SM_All_Bottles_Base.png");

    coord = glm::vec3(-0.15416f, 1.37f, -0.3593f);
    createObject("bar/small/SM_BoChampagneBottle.obj", false, true,
                 Frame{coord}, mat::bottle, 
                 "bar/small/SM_All_Bottles_Base.png");

    coord = glm::vec3(-0.103f, 1.3f, 0.4556f);
    createObject("bar/small/SM_Bucket1.obj", false, true,
                 Frame{coord}, mat::gold, 
                 "bar/small/SM_Buckets_and_Tray_Base.png");
    coord = glm::vec3(-2.7557f, 1.3f, 2.2f);
    rot = glm::vec3(0.0f, 150.0f, 0.0f);
    copyObject(Frame{coord, rot});

    coord = glm::vec3(-0.07267f, 1.3f, -2.1163);
    createObject("bar/small/SM_Bucket2.obj", false, true,
                 Frame{coord}, mat::gold, 
                 "bar/small/SM_Buckets_and_Tray_Base.png");

    coord = glm::vec3(-0.052672f, 1.37f, 0.86051f);
    createObject("bar/small/SM_BVodkaBottle01.obj", false, true,
                 Frame{coord}, mat::bottle, 
                 "bar/small/SM_All_Bottles_Base.png");

    coord = glm::vec3(-0.094162f, 1.37f, -0.4398f);
    createObject("bar/small/SM_BWineBottle2.obj", false, true,
                 Frame{coord}, mat::bottle, 
                 "bar/small/SM_All_Bottles_Base.png");

    coord = glm::vec3(-0.12267f, 1.37f, -2.7405f);
    createObject("bar/small/SM_BWineBottle9.obj", false, true,
                 Frame{coord}, mat::bottle, 
                 "bar/small/SM_All_Bottles_Base.png");

    coord = glm::vec3(-3.6f, 1.37f, 3.2f);
    rot = glm::vec3(0.0f, -30.0f, 0.0f);
    copyObject(Frame{coord, rot});

    coord = glm::vec3(-1.5057f, 1.24f, 0.68652f);
    createObject("bar/small/SM_Dish.obj", false, true,
                 Frame{coord}, mat::porcelain, 
                 "bar/small/SM_All_Dishes_Base.png");

    coord = glm::vec3(-1.5057f, 1.24f, 1.1643f);
    createObject("bar/small/SM_Dish_Egg.obj", false, true,
                 Frame{coord}, mat::porcelain, 
                 "bar/small/SM_All_Dishes_Base.png");

    coord = glm::vec3(0.0135f, 1.3f, 2.4027f);
    createObject("bar/small/SM_Ketchup.obj", false, true,
                 Frame{coord}, mat::red_plastic, 
                 "bar/small/SM_Kethchup3_Base.png");

    coord = glm::vec3(-0.15267f, 1.22f, 0.049028f);
    createObject("bar/small/SM_Tray.obj", false, true,
                 Frame{coord}, mat::white_plastic, 
                 "bar/small/SM_Buckets_and_Tray_Base.png");

    coord = glm::vec3(0.00733f, 1.32f, 1.0558f);
    createObject("bar/small/SM_Wine_Glass.obj", false, true,
                 Frame{coord}, mat::glass);

    // Central Ceiling
    coord = glm::vec3(0.0f, -0.05f, 0.0f);
    createObject("corridor/floor_flipped.obj", false, false,
                 Frame{coord}, mat::steel,
                 "corridor/floor_BaseColor.png");
    // Lights
    // Chinese Baloons
    coord = glm::vec3(-0.3f, 3.0f, 2.2);
    createLight(glm::vec4(coord, 1.0f), glm::vec4(0.1f), glm::vec4(color::light_blue / 2.0f, 1.0f), glm::vec4(color::light_blue / 2.0f, 1.0f));

    block_dist = 0.97f;
    for (int i = 1; i <= 2; ++i)
        createLight(glm::vec4(coord.x, coord.y, coord.z - i * block_dist, 1.0f),
                    glm::vec4(0.1f), glm::vec4(color::light_blue / 2.0f, 1.0f), glm::vec4(color::light_blue / 2.0f, 1.0f));

    // Spotlights
    // Bar Lit from the Right
    coord = glm::vec3(4.0f, 4.7f, 0.0f);
    rot = glm::vec3(0.0f, 180.0f, 0.0f);
    std::tie(spotlight, sphere) = createSpotlight(Frame{coord, rot}, glm::vec4(color::pink, 1.0f), glm::vec3(-1.0f, -1.0f, 0.0f), true);

    coord.z = -3.0f;
    copySpotlight(Frame{coord, rot}, glm::vec4(color::pink, 1.0f), glm::vec3(-1.0f, -1.0f, 0.0f), false);
    coord.z = 3.0f;
    copySpotlight(Frame{coord, rot}, glm::vec4(color::pink, 1.0f), glm::vec3(-1.0f, -1.0f, 0.0f), false);
    
    // Bar Lit from the Left
    coord = glm::vec3(-5.0f, 4.7f, 0.0f);
    rot = glm::vec3(0.0f, 0.0f, 0.0f);
    copySpotlight(Frame{coord, rot}, glm::vec4(color::green / 2.0f, 1.0f), glm::vec3(1.0f, -1.0f, 0.0f), true);
    coord.z = 2.0f;
    copySpotlight(Frame{coord, rot}, glm::vec4(color::green / 2.0f, 1.0f), glm::vec3(1.0f, -1.0f, 0.0f), false);
    coord.z = 4.0f;
    copySpotlight(Frame{coord, rot}, glm::vec4(color::green / 2.0f, 1.0f), glm::vec3(1.0f, -1.0f, 0.0f), false);

    // Outer Perimeter Lights
    coord = glm::vec3(11.0f, 4.5f, 14.7f);
    rot = glm::vec3(0.0f);
    block_dist = 7.4f;
    for (int i = 0; i <= 2; ++i)
    {
        coord.z -= block_dist;
        copySpotlight(Frame{coord, rot}, glm::vec4(color::blue * 2, 1.0f), glm::vec3(1.0f, -1.0f, 0.0f), i % 2 == 0);
    }

    coord = glm::vec3(-11.0f, 4.5f, 14.7f);
    rot = glm::vec3(0.0f, 180.0f, 0.0f);

    for (int i = 0; i <= 2; ++i)
    {
        coord.z -= block_dist;
        copySpotlight(Frame{coord, rot}, glm::vec4(color::blue * 2, 1.0f), glm::vec3(-1.0f, -1.0f, 0.0f), i % 2 == 1);
    }

    coord = glm::vec3(-7.3f, 4.5f, -11.2f);
    rot = glm::vec3(0.0f, 90.0f, 0.0f);
    for (int i = 0; i <= 2; ++i)
    {
        copySpotlight(Frame{coord, rot}, glm::vec4(color::blue * 2, 1.0f), glm::vec3(0.0f, -1.0f, -1.0f), i % 2 == 1);
        coord.x += block_dist;
    }

    // Door Light
    coord = glm::vec3(0.0f, 2.6f, 14.6f);
    createLight(glm::vec4(coord, 1.0f), glm::vec4(color::light_blue, 1.0f), glm::vec3(0.0f, -0.5f, -1.0f));

    // Light X
    createLight(glm::vec4(1.0f, -1.0f, 1.0f, 0.0f), glm::vec4(color::light_blue * 0.2, 1.0f), glm::vec4(color::light_blue * 0.2, 1.0f), glm::vec4(color::light_blue * 0.7, 1.0f)); 
    createLight(glm::vec4(-1.0f, -1.0f, -1.0f, 0.0f), glm::vec4(color::light_blue * 0.2, 1.0f), glm::vec4(color::light_blue * 0.2, 1.0f), glm::vec4(color::light_blue * 0.7, 1.0f)); 

    createLight(glm::vec4(1.0f, -1.0f, -1.0f, 0.0f), glm::vec4(0.0f), glm::vec4(0.0f), glm::vec4(color::light_blue * 0.2, 1.0f)); 
    createLight(glm::vec4(-1.0f, -1.0f, 1.0f, 0.0f), glm::vec4(0.0f), glm::vec4(0.0f), glm::vec4(color::light_blue * 0.2, 1.0f)); 
}

void Application::updateObjectsUBOs()
{
    for (std::size_t i = 0; i < glm::min(geometries.size(), objects_ubos.size()); ++i)
    {
        if (geometries[i]->animated)
            objects_ubos[i].model_matrix = geometries[i]->frame->getModelMat();
    }
    glNamedBufferSubData(objects_buffer, 0, sizeof(ObjectUBO) * objects_ubos.size(), objects_ubos.data());
}

void Application::updateSpotlightUBOs()
{
    for (std::size_t i = 0; i < glm::min(spotlight_geometries.size(), spotlight_objects_ubos.size()); ++i)
    {
        if (spotlight_geometries[i]->animated)
            spotlight_objects_ubos[i].model_matrix = spotlight_geometries[i]->frame->getModelMat();
    }
    glNamedBufferSubData(spotlight_objects_buffer, 0, sizeof(ObjectUBO) * spotlight_objects_ubos.size(), spotlight_objects_ubos.data());
    glNamedBufferSubData(lights_buffer, 0, sizeof(LightUBO) * lights_ubos.size(), lights_ubos.data());
}

void Application::rotateSpotlights(float delta)
{
    // Light spheres share a frame from spotlight, so we only have to rotate the spotlight
    for (int i = 0; i < spotlight_geometries.size(); i += 2)
    {
        if (!spotlight_geometries[i]->animated)
            continue;

        auto &spot = spotlight_geometries[i];
        spot->frame->rotateRotationQuat(glm::vec3(0.0f, 0.02f * delta, 0.0f));
        auto &sphere = spotlight_geometries[i + 1];
        auto &cone = lights_ubos[sphere->bound_texture].cone_direction;
        cone = glm::tquat(glm::radians(glm::vec3(0.0f, 0.02f * delta, 0.0f))) * cone;
    }
}

void Application::delete_shaders()
{
    glDeleteProgram(main_program);
    glDeleteProgram(lightsource_program);
    glDeleteProgram(skybox_program);
}

void Application::compile_shaders() {
    main_program = create_program(lecture_shaders_path / "main.vert", lecture_shaders_path / "main.frag");
    lightsource_program = create_program(lecture_shaders_path / "light.vert", lecture_shaders_path / "light.frag");
    skybox_program = create_program(lecture_shaders_path / "skybox.vert", lecture_shaders_path / "skybox.frag");

    std::cout << "Shaders recompiled" << std::endl;
}

void Application::recompile_shaders() {
    delete_shaders();
    compile_shaders();
}

void Application::update(float delta) 
{
    elapsed_time += delta;
    skybox_frame->rotateRotationQuat(delta * glm::vec3(0.0f, 0.001f, 0.001f));
    // updateObjectsUBOs();

    rotateSpotlights(delta);
    updateSpotlightUBOs();
}

void Application::render_skybox()
{
    glUseProgram(skybox_program);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, camera_buffer);
    glProgramUniformMatrix4fv(skybox_program, 0, 1, GL_FALSE, glm::value_ptr(skybox_frame->getRotationMat()));
    glBindTextureUnit(0, skybox_cube->bound_texture);
    skybox_cube->draw();
}
void Application::render_lightsources()
{
    glUseProgram(lightsource_program);
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, camera_buffer);

    for (auto &object : spotlight_geometries)
    {
        if (!object->lightsource)
            continue;
        glProgramUniform1i(lightsource_program, 0, object->has_texture);
        glBindTextureUnit(2, object->bound_texture);
        glBindBufferRange(GL_UNIFORM_BUFFER, 1, spotlight_objects_buffer, object->index * 256, sizeof(ObjectUBO));
        object->draw();
    }

    for (auto &object : geometries)
    {
        if (!object->lightsource)
            continue;
        glProgramUniform1i(lightsource_program, 0, object->has_texture);
        glBindTextureUnit(2, object->bound_texture);
        glBindBufferRange(GL_UNIFORM_BUFFER, 1, objects_buffer, object->index * 256, sizeof(ObjectUBO));
        object->draw();
    }
}
void Application::render_opaque()
{
    glUseProgram(main_program);
    glProgramUniformMatrix4fv(main_program, 2, 1, GL_FALSE, glm::value_ptr(skybox_frame->getRotationMat()));
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, camera_buffer);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, lights_buffer);
    glBindTextureUnit(4, skybox_texture);

    for (auto &object : spotlight_geometries)
    {
        if (object->lightsource)
            continue;
        glBindBufferRange(GL_UNIFORM_BUFFER, 1, spotlight_objects_buffer, object->index * 256, sizeof(ObjectUBO));
        glProgramUniformMatrix3fv(main_program, 0, 1, GL_FALSE, glm::value_ptr(object->getNormalMatrix()));
        glProgramUniform1i(main_program, 1, object->has_texture);
        glBindTextureUnit(3, object->bound_texture);
        object->draw();
    }

    for (auto &object : geometries)
    {
        if (object->material.diffuse_color.w < 1.0f || object->lightsource)
            continue;
        
        glBindBufferRange(GL_UNIFORM_BUFFER, 1, objects_buffer, object->index * 256, sizeof(ObjectUBO));
        glProgramUniformMatrix3fv(main_program, 0, 1, GL_FALSE, glm::value_ptr(object->getNormalMatrix()));
        glProgramUniform1i(main_program, 1, object->has_texture);
        glBindTextureUnit(3, object->bound_texture);
        object->draw();
    }
}
void Application::render_transparent()
{
    for (auto &object : geometries)
    {
        if (object->material.diffuse_color.w >= 1.0f)
            continue;
        
        glBindBufferRange(GL_UNIFORM_BUFFER, 1, objects_buffer, object->index * 256, sizeof(ObjectUBO));
        glProgramUniformMatrix3fv(main_program, 0, 1, GL_FALSE, glm::value_ptr(object->getNormalMatrix()));
        glProgramUniform1i(main_program, 1, object->has_texture);
        glBindTextureUnit(3, object->bound_texture);
        object->draw();
    }
}

void Application::render() {
    // --------------------------------------------------------------------------
    // Update UBOs
    // --------------------------------------------------------------------------
    // Camera
    camera_ubo.position = glm::vec4(camera.get_eye_position(), 1.0f);
    camera_ubo.projection = glm::perspective(glm::radians(45.0f), static_cast<float>(width) / static_cast<float>(height), 0.01f, 1000.0f);
    camera_ubo.view = glm::lookAt(camera.get_eye_position(), 
                                  camera.getTarget(),
                                  glm::vec3(0.0f, 1.0f, 0.0f));
    glNamedBufferSubData(camera_buffer, 0, sizeof(CameraUBO), &camera_ubo);

    // --------------------------------------------------------------------------
    // Draw scene
    // --------------------------------------------------------------------------
    glViewport(0, 0, (GLsizei)width, (GLsizei)height);

    // Clear
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    /* SKYBOX */
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);

    render_skybox();

    /* OBJECTS */
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    /* OPAQUE */
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    render_lightsources();

    render_opaque();

    /* TRANSPARENT */
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    // This assumes main_program is in use
    render_transparent();

    glDepthMask(GL_TRUE);
}

void Application::ui_camera_type()
{
    auto message_name = "Camera Distance: ";
    auto value = camera.getStateShortDistance() ? "Short" : "Long";

    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    auto text_size = ImGui::CalcTextSize(message_name);
    text_size.x += ImGui::CalcTextSize(value).x;
    ImGui::SetNextWindowSize(ImVec2(text_size.x + 30, text_size.y), ImGuiCond_Always);

    ImGui::Begin("Camera Type", nullptr, ImGuiWindowFlags_NoDecoration);
    ImGui::Text(message_name);
    ImGui::SameLine();
    ImGui::Text(value);
    ImGui::End();
}

void Application::ui_step_change()
{
    auto message_name = "Step Size: ";
    auto value = camera.getStep();

    auto text_size = ImGui::CalcTextSize(message_name);
    text_size.x += 100;
    ImGui::SetNextWindowPos(ImVec2(width - (text_size.x + 30), 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(text_size.x + 30, text_size.y), ImGuiCond_Always);

    ImGui::Begin("Camera Step", nullptr, ImGuiWindowFlags_NoDecoration);
    ImGui::Text(message_name);
    ImGui::SameLine();
    ImGui::Text("%.2f", value);
    ImGui::End();
}

void Application::render_ui() 
{
    const float unit = ImGui::GetFontSize(); 

    if (elapsed_time - camera.getToggleTime() < 2000.0)
        ui_camera_type();

    if (elapsed_time - camera.getStepTime() < 2000.0)
        ui_step_change();
}

// ----------------------------------------------------------------------------
// Input Events
// ----------------------------------------------------------------------------

void Application::on_resize(int width, int height) {
    // Calls the default implementation to set the class variables.
    PV112Application::on_resize(width, height);
}

void Application::on_mouse_move(double x, double y) { camera.on_mouse_move(x, y); }
void Application::on_mouse_button(int button, int action, int mods) { camera.on_mouse_button(button, action, mods); }
void Application::on_key_pressed(int key, int scancode, int action, int mods) {
    // Calls default implementation that invokes compile_shaders when 'R key is hit.
    // PV112Application::on_key_pressed(key, scancode, action, mods);
    if (!ImGui::GetIO().WantCaptureMouse && action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_T:
            recompile_shaders();
            break;
        }
    }
    camera.on_key_pressed(key, scancode, action, mods, elapsed_time);
}