#include "Camera.h"

Camera::Camera(uint32_t width, uint32_t height)
    : width_(width), height_(height),
      fieldOfView_(dx::XMConvertToRadians(45.0f)),
      nearClipPlane_(0.1f),
      farClipPlane_(100.0f) {

    cameraPosition_ = dx::XMVectorSet(10.0f, 10.0f, 10.0f, 1.0f);
    cameraTarget_ = dx::XMVectorZero();
    cameraUp_ = dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    UpdateViewMatrix();
    UpdateProjectionMatrix();
}

Camera::~Camera() {
    // No special cleanup needed
}

void Camera::UpdateCamera(const dx::XMVECTOR& position, const dx::XMVECTOR& target, const dx::XMVECTOR& up) {
    cameraPosition_ = position;
    cameraTarget_ = target;
    cameraUp_ = up;
    UpdateViewMatrix();
}

void Camera::Resize(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;
    UpdateProjectionMatrix();
}

void Camera::SetPosition(const dx::XMVECTOR& position) {
    cameraPosition_ = position;
    UpdateViewMatrix();
}

void Camera::SetTarget(const dx::XMVECTOR& target) {
    cameraTarget_ = target;
    UpdateViewMatrix();
}

void Camera::SetUp(const dx::XMVECTOR& up) {
    cameraUp_ = up;
    UpdateViewMatrix();
}

void Camera::SetFieldOfView(float fov) {
    fieldOfView_ = fov;
    UpdateProjectionMatrix();
}

void Camera::SetNearClipPlane(float nearPlane) {
    nearClipPlane_ = nearPlane;
    UpdateProjectionMatrix();
}

void Camera::SetFarClipPlane(float farPlane) {
    farClipPlane_ = farPlane;
    UpdateProjectionMatrix();
}

void Camera::UpdateViewMatrix() {
    viewMatrix_ = dx::XMMatrixLookAtLH(cameraPosition_, cameraTarget_, cameraUp_);
}

void Camera::UpdateProjectionMatrix() {
    projectionMatrix_ = dx::XMMatrixPerspectiveFovLH(
        fieldOfView_,
        static_cast<float>(width_) / static_cast<float>(height_),
        nearClipPlane_,
        farClipPlane_
    );
}

const dx::XMMATRIX& Camera::GetViewMatrix() const {
    return viewMatrix_;
}

const dx::XMMATRIX& Camera::GetProjectionMatrix() const {
    return projectionMatrix_;
}

const dx::XMVECTOR& Camera::GetPosition() const {
    return cameraPosition_;
}

const dx::XMVECTOR& Camera::GetTarget() const {
    return cameraTarget_;
}

const dx::XMVECTOR& Camera::GetUp() const {
    return cameraUp_;
}

// 实现键盘输入处理方法
void Camera::ProcessKeyboardInput(const bool keys[], float deltaTime) {
    // 计算相机方向向量
    dx::XMVECTOR front = dx::XMVector3Normalize(dx::XMVectorSubtract(cameraTarget_, cameraPosition_));
    dx::XMVECTOR up = cameraUp_;
    dx::XMVECTOR right = dx::XMVector3Normalize(dx::XMVector3Cross(front, up));
    
    // 移动速度
    float moveSpeed = 2.5f * deltaTime;
    
    // 向前移动 (W键)
    if (keys['W']) {
        cameraPosition_ = dx::XMVectorAdd(cameraPosition_, dx::XMVectorScale(front, moveSpeed));
        cameraTarget_ = dx::XMVectorAdd(cameraTarget_, dx::XMVectorScale(front, moveSpeed));
    }
    
    // 向后移动 (S键)
    if (keys['S']) {
        cameraPosition_ = dx::XMVectorSubtract(cameraPosition_, dx::XMVectorScale(front, moveSpeed));
        cameraTarget_ = dx::XMVectorSubtract(cameraTarget_, dx::XMVectorScale(front, moveSpeed));
    }
    
    // 向左移动 (A键)
    if (keys['A']) {
        cameraPosition_ = dx::XMVectorAdd(cameraPosition_, dx::XMVectorScale(right, moveSpeed));
        cameraTarget_ = dx::XMVectorAdd(cameraTarget_, dx::XMVectorScale(right, moveSpeed));
    }
    
    // 向右移动 (D键)
    if (keys['D']) {
        cameraPosition_ = dx::XMVectorSubtract(cameraPosition_, dx::XMVectorScale(right, moveSpeed));
        cameraTarget_ = dx::XMVectorSubtract(cameraTarget_, dx::XMVectorScale(right, moveSpeed));
    }
    
    // 更新视图矩阵
    UpdateViewMatrix();
}