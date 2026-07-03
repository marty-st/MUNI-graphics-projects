#include "cameraplus.hpp"

#include "imgui.h"
#include "iapplication.h"

void CameraPlus::setDefaultState()
{
    angle_direction = -0.559999943f;
    angle_elevation = 0.208000064f;
    distance = 14.9283714f;
}

void CameraPlus::toggleCameraType(long double elapsed_time)
{
    if (state_short_distance)
    {
        distance = long_distance;
        setTarget(get_eye_position() + getFrontVec() * distance);
    }
    else
    {
        setTarget(get_eye_position() + getFrontVec());
        long_distance = distance;
        distance = 1.0f;
    }
    state_short_distance = !state_short_distance;
    toggle_time = elapsed_time;
}

void CameraPlus::update_eye_pos()
{
    if (switch_axes) {
    eye_position.x = target.x - distance * cosf(angle_elevation) * sinf(angle_direction);
    eye_position.z = target.y - distance * sinf(angle_elevation);
    eye_position.y = target.z - distance * cosf(angle_elevation) * cosf(angle_direction);
    } else {
    eye_position.x = target.x + distance * cosf(angle_elevation) * -sinf(angle_direction);
    eye_position.y = target.y + distance * sinf(angle_elevation);
    eye_position.z = target.z + distance * cosf(angle_elevation) * cosf(angle_direction);
    }
    frame->setPosVec(eye_position);
}

/* SAME AS IN CAMERA CLASS BUT FOR SOME REASON HAS TO BE HERE 
    (probably due to use of update_pos_vec from this class) */
void CameraPlus::on_mouse_move(double x, double y)
{
    float dx = static_cast<float>(x - last_x);
    float dy = static_cast<float>(y - last_y);
    last_x = static_cast<int>(x);
    last_y = static_cast<int>(y);

    if (is_rotating) {
        angle_direction += dx * angle_sensitivity;
        angle_elevation += dy * angle_sensitivity;

        // Clamps the results.
        angle_elevation = glm::clamp(angle_elevation, min_elevation, max_elevation);
    }
    if (is_zooming) {
        distance *= (1.0f + dy * zoom_sensitivity);

        // Clamps the results.
        if (distance < min_distance)
            distance = min_distance;
    }

    update_eye_pos();
}

void CameraPlus::on_key_pressed(int key, int scancode, int action, int mods, long double elapsed_time)
{
    if (!ImGui::GetIO().WantCaptureMouse && (action == GLFW_REPEAT || action == GLFW_PRESS)) {
        switch (key) {
        case GLFW_KEY_W:
            forward();
            break;
        case GLFW_KEY_A:
            left();
            break;
        case GLFW_KEY_S:
            backward();
            break;
        case GLFW_KEY_D:
            right();
            break;
        case GLFW_KEY_R:
            up();
            break;
        case GLFW_KEY_F:
            down();
            break;
        }
    }

    /* Reset camera to (0, 0, 0), change camera 'type' or increase / decrease WASD movement step */
    if (!ImGui::GetIO().WantCaptureMouse && action == GLFW_PRESS) {
        switch (key) {
        case GLFW_KEY_C:
            setTarget(glm::vec3(0.0f, 0.0f, 0.0f));
            break;
        case GLFW_KEY_V:
            toggleCameraType(elapsed_time);
            break;
        case GLFW_KEY_E:
            step *= 2.0f;
            step_change_time = elapsed_time;
            break;
        case GLFW_KEY_Q:
            step /= 2.0f;
            step_change_time = elapsed_time;
            break;
        }
    }
    update_eye_pos();
}

void CameraPlus::forward()
{
    auto front_vec = getFrontVec();
    auto length_orig = glm::length(front_vec);

    front_vec.y = 0.0f;
    front_vec *= length_orig / glm::length(front_vec);;

    translateTarget(step * front_vec);
}
void CameraPlus::backward()
{
    auto front_vec = getFrontVec();
    auto length_orig = glm::length(front_vec);

    front_vec.y = 0.0f;
    front_vec *= length_orig / glm::length(front_vec);

    translateTarget(-step * front_vec);
}
void CameraPlus::left()
{
    translateTarget(-step * glm::normalize(glm::cross(getFrontVec(), getUpVec())));
}
void CameraPlus::right()
{
    translateTarget(step * glm::normalize(glm::cross(getFrontVec(), getUpVec())));
}
void CameraPlus::up()
{
    translateTarget(step * getUpVec());
}
void CameraPlus::down()
{
    translateTarget(-step * getUpVec());
}
