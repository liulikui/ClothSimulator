#include "Camera.h"

Camera::Camera(uint32_t width, uint32_t height)
    : m_width(width), m_height(height),
      m_fieldOfView(dx::XMConvertToRadians(45.0f)),
      m_nearClipPlane(0.1f),
      m_farClipPlane(100.0f)
{

    m_cameraPosition = dx::XMVectorSet(10.0f, 10.0f, 10.0f, 1.0f);
    m_cameraTarget = dx::XMVectorZero();
    m_cameraUp = dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);

    UpdateViewMatrix();
    UpdateProjectionMatrix();
}

Camera::~Camera()
{
    // No special cleanup needed
}

void Camera::UpdateCamera(const dx::XMVECTOR& position, const dx::XMVECTOR& target, const dx::XMVECTOR& up)
{
    m_cameraPosition = position;
    m_cameraTarget = target;
    m_cameraUp = up;
    UpdateViewMatrix();
}

void Camera::Resize(uint32_t width, uint32_t height)
{
    m_width = width;
    m_height = height;
    UpdateProjectionMatrix();
}

void Camera::SetPosition(const dx::XMVECTOR& position)
{
    m_cameraPosition = position;
    UpdateViewMatrix();
}

void Camera::SetTarget(const dx::XMVECTOR& target)
{
    m_cameraTarget = target;
    UpdateViewMatrix();
}

void Camera::SetUp(const dx::XMVECTOR& up)
{
    m_cameraUp = up;
    UpdateViewMatrix();
}

void Camera::SetFieldOfView(float fov)
{
    m_fieldOfView = fov;
    UpdateProjectionMatrix();
}

void Camera::SetNearClipPlane(float nearPlane)
{
    m_nearClipPlane = nearPlane;
    UpdateProjectionMatrix();
}

void Camera::SetFarClipPlane(float farPlane)
{
    m_farClipPlane = farPlane;
    UpdateProjectionMatrix();
}

void Camera::UpdateViewMatrix()
{
    m_viewMatrix = dx::XMMatrixLookAtLH(m_cameraPosition, m_cameraTarget, m_cameraUp);
}

void Camera::UpdateProjectionMatrix()
{
    m_projectionMatrix = dx::XMMatrixPerspectiveFovLH(
        m_fieldOfView,
        static_cast<float>(m_width) / static_cast<float>(m_height),
        m_nearClipPlane,
        m_farClipPlane
    );
}

const dx::XMMATRIX& Camera::GetViewMatrix() const {
    return m_viewMatrix;
}

const dx::XMMATRIX& Camera::GetProjectionMatrix() const {
    return m_projectionMatrix;
}

const dx::XMVECTOR& Camera::GetPosition() const {
    return m_cameraPosition;
}

const dx::XMVECTOR& Camera::GetTarget() const {
    return m_cameraTarget;
}

const dx::XMVECTOR& Camera::GetUp() const {
    return m_cameraUp;
}

// 实现键盘输入处理方法
void Camera::ProcessKeyboardInput(const bool keys[], float deltaTime) {
    // 计算相机方向向量
    dx::XMVECTOR front = dx::XMVector3Normalize(dx::XMVectorSubtract(m_cameraTarget, m_cameraPosition));
    dx::XMVECTOR up = m_cameraUp;
    dx::XMVECTOR right = dx::XMVector3Normalize(dx::XMVector3Cross(front, up));
    
    // 移动速度
    float moveSpeed = 2.5f * deltaTime;
    
    // 向前移动 (W键)
    if (keys['W']) {
        m_cameraPosition = dx::XMVectorAdd(m_cameraPosition, dx::XMVectorScale(front, moveSpeed));
        m_cameraTarget = dx::XMVectorAdd(m_cameraTarget, dx::XMVectorScale(front, moveSpeed));
    }
    
    // 向后移动 (S键)
    if (keys['S']) {
        m_cameraPosition = dx::XMVectorSubtract(m_cameraPosition, dx::XMVectorScale(front, moveSpeed));
        m_cameraTarget = dx::XMVectorSubtract(m_cameraTarget, dx::XMVectorScale(front, moveSpeed));
    }
    
    // 向左移动 (A键)
    if (keys['A']) {
        m_cameraPosition = dx::XMVectorAdd(m_cameraPosition, dx::XMVectorScale(right, moveSpeed));
        m_cameraTarget = dx::XMVectorAdd(m_cameraTarget, dx::XMVectorScale(right, moveSpeed));
    }
    
    // 向右移动 (D键)
    if (keys['D']) {
        m_cameraPosition = dx::XMVectorSubtract(m_cameraPosition, dx::XMVectorScale(right, moveSpeed));
        m_cameraTarget = dx::XMVectorSubtract(m_cameraTarget, dx::XMVectorScale(right, moveSpeed));
    }
    
    // 更新视图矩阵
    UpdateViewMatrix();
}