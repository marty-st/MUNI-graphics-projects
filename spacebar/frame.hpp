#ifndef FRAME_INCLUDED
#define FRAME_INCLUDED

#include "pv112_application.hpp"

#include <memory>
#include <vector>

class Frame
{
    glm::vec3 position_vec;
    glm::quat rotation_quat;
    glm::vec3 scale_vec;

public:
    Frame(glm::vec3 _position_vec = glm::vec3(0, 0, 0), 
          glm::vec3 rotation_vec = glm::vec3(0, 0, 0),
          glm::vec3 _scale_vec = glm::vec3(1.0f, 1.0f, 1.0f))
     : position_vec{_position_vec}, scale_vec{_scale_vec}
    {    
        setRotationQuat(rotation_vec);
    }

    Frame(glm::vec3 _position_vec,
          glm::quat _rotation_quat,
          glm::vec3 _scale_vec = glm::vec3(1.0f, 1.0f, 1.0f))
     : position_vec{_position_vec}, rotation_quat{glm::normalize(_rotation_quat)}, scale_vec(_scale_vec) {}

    glm::vec3 getPosition() const { return position_vec; }
    glm::quat getRotation() const { return rotation_quat; }
    glm::vec3 getScale() const { return scale_vec; }

    glm::vec3 getRotationAxisX() const { return glm::normalize(glm::column(glm::toMat3(rotation_quat), 0)); }
    glm::vec3 getRotationAxisY() const { return glm::normalize(glm::column(glm::toMat3(rotation_quat), 1)); }
    glm::vec3 getRotationAxisZ() const { return glm::normalize(glm::column(glm::toMat3(rotation_quat), 2)); }

    glm::mat4 getTranslationMat() const { return glm::translate(glm::mat4(1.0f), position_vec); }
    glm::mat4 getRotationMat() const { return glm::toMat4(rotation_quat); }
    glm::mat4 getScaleMat() const { return glm::scale(glm::mat4(1.0f), scale_vec); }

    glm::mat4 getModelMat() const { return getTranslationMat() * getRotationMat() * getScaleMat(); }
    glm::mat4 getInverseModelMat() const { return glm::inverse(getModelMat()); }
    glm::mat4 getViewMat() const { return glm::inverse(getModelMat()); }

    void setPosVec(glm::vec3 _position_vec) { position_vec = _position_vec; }
    void translatePosVec(glm::vec3 direction) { position_vec += direction; }
    void setRotationQuat(glm::vec3 rotation_vec) { rotation_quat = glm::normalize(glm::tquat(glm::radians(rotation_vec))); }
    void setRotationQuat(glm::quat _rotation_quat) { rotation_quat = glm::normalize(_rotation_quat); }
    void rotateRotationQuat(glm::vec3 added_rotation) { rotation_quat = glm::normalize(glm::tquat(glm::radians(added_rotation)) * rotation_quat); }
    void rotateRotationQuat(glm::quat added_rotation) { rotation_quat = glm::normalize(added_rotation * rotation_quat); }
    void setScale(glm::vec3 _scale_vec) { scale_vec = _scale_vec; }

    void normalizeQuat() { rotation_quat = glm::normalize(rotation_quat); } 
};

using frame_ptr = std::shared_ptr<Frame>;

#endif