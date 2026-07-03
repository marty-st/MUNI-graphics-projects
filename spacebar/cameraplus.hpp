#pragma once

#include "camera.h"
#include "frame.hpp"
#include <glm/glm.hpp>
#include <limits>
#include <memory>

class CameraPlus : public Camera
{
    frame_ptr frame;
    glm::vec3 target = glm::vec3(0.0f, 0.0f, 0.0f);
    float step = 0.4f;
    long double step_change_time = std::numeric_limits<double>::lowest();

    bool state_short_distance = false;
    float long_distance;
    long double toggle_time = std::numeric_limits<double>::lowest();

public:
    using Camera::Camera;
    CameraPlus() : Camera()
    {
        frame = std::make_shared<Frame>();
        setDefaultState();
        update_eye_pos();
    }
    CameraPlus(bool switch_axes) : Camera(switch_axes) 
    {
        frame = std::make_shared<Frame>();
        setDefaultState();
        update_eye_pos();
    }

    void setDefaultState();

    void toggleCameraType(long double elapsed_time);

    const long double &getToggleTime() const { return toggle_time; }
    bool getStateShortDistance() const { return state_short_distance; }

    const long double &getStepTime() const { return step_change_time; }
    float getStep() const { return step; }

    frame_ptr &getFrame() { return frame; }
    const glm::vec3 &getTarget() const { return target; }
    const glm::vec3 getFrontVec() const { return glm::normalize(target - frame->getPosition()); }
    const glm::vec3 getUpVec() const { return glm::vec3(0.0f, 1.0f, 0.0f); }

    glm::vec3 get_eye_position() const { return frame->getPosition(); }
    glm::mat4x4 get_view_matrix() const { return lookAt(frame->getPosition(), target, glm::vec3(0.0f, 1.0f, 0.0f)); }

    void translateTarget(glm::vec3 direction) { target += direction; }
    void setTarget(glm::vec3 position) { target = position; }

    void update_eye_pos();

    /* SAME AS IN CAMERA CLASS BUT FOR SOME REASON HAS TO BE HERE 
       (probably due to use of update_pos_vec from this class) */
    void on_mouse_move(double x, double y);
    void on_key_pressed(int key, int scancode, int action, int mods, long double elapsed_time);
    void forward();
    void backward();
    void left();
    void right();
    void up();
    void down();

};