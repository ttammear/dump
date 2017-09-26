#include "transform.h"

Transform::Transform()
{
    this->position = Vec3(0.0f, 0.0f, 0.0f);
    this->rotation = Quaternion::Identity();
    this->scale = Vec3(1.0f, 1.0f, 1.0f);
}

Mat4 Transform::getModelMatrix()
{
    return Mat4::TRS(this->position, this->rotation, this->scale);
}

Vec3 Transform::forward()
{
    Mat4 rot = Mat4::Rotation(this->rotation);
    Vec4 ret(0.0f, 0.0f, 1.0f, 1.0f);
    ret = rot*ret;
    return Vec3(ret.x, ret.y, ret.z);
}

Vec3 Transform::right()
{
    Mat4 rot = Mat4::Rotation(this->rotation);
    Vec4 ret(1.0f, 0.0f, 0.0f, 1.0f);
    ret = rot*ret;
    return Vec3(ret.x, ret.y, ret.z);
}

void Transform::setPosition(Vec3 *pos)
{
    position = *pos;
    printf("position set to %f %f %f\n", pos->x, pos->y, pos->z);
}

Vec3 Transform::getPosition()
{
    return position;
}

void Transform::setRotation(Quaternion *rot)
{
    rotation = *rot;
}

Quaternion Transform::getRotation()
{
    return rotation;
}

