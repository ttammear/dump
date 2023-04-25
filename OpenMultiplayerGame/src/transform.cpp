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
}

void Transform::setRotation(Quaternion *rot)
{
    rotation = *rot;
}

void Transform::tsetPosition(Transform *t, Vec3 *pos)
{
    t->setPosition(pos);
}

void Transform::tsetRotation(Transform *t, Quaternion *rot)
{
    t->setRotation(rot);
}

Vec3 Transform::tgetPosition(Transform *t)
{
    return t->position;
}

Quaternion Transform::tgetRotation(Transform *t)
{
    return t->rotation;
}

