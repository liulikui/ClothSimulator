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