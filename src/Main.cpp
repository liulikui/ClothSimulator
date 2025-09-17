#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <vector>
#include <cstdint>
#include "Cloth.h"
#include "DX12Renderer.h"
#include "Camera.h"
#include <windowsx.h>

// 为了方便使用，定义一个简化的命名空间别名
namespace dx = DirectX;

// 窗口尺寸常量 - 这里会被覆盖为全屏尺寸
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;

// 全局变量
HWND hWnd = NULL;              // 窗口句柄
HINSTANCE hInstance = NULL;    // 应用程序实例
bool running = true;           // 运行标志
bool debugOutputEnabled = false; // 调试输出开关，默认关闭
bool f9Pressed = false;        // F9键按下标志，用于检测按键状态变化
int frameCount = 0;            // 当前帧数计数器
int maxFrames = -1;            // 最大帧数限制（-1表示不限制）

// 相机对象
Camera* camera = nullptr;

// 鼠标控制参数
float yaw = -90.0f;   // yaw is initialized to -90.0 degrees since a yaw of 0.0 results in a direction vector pointing to the right
float pitch = 0.0f;
float lastX = SCR_WIDTH / 2.0f;
float lastY = SCR_HEIGHT / 2.0f;
bool firstMouse = true;
bool mouseCaptured = false;

// 键盘状态数组，用于跟踪按键状态
bool keys[256] = { false }; // 假设是标准ASCII键盘

// 时间控制
float deltaTime = 0.0f;
float lastFrame = 0.0f;

// 前向声明
void UpdateCamera(const dx::XMVECTOR& position, const dx::XMVECTOR& target, const dx::XMVECTOR& up);

// 布料和渲染器对象
Cloth* cloth = nullptr;
DX12Renderer* renderer = nullptr;

// 窗口过程函数
LRESULT CALLBACK WndProc(HWND hWnd, uint32_t message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED && renderer) {
            uint32_t width = LOWORD(lParam);
            uint32_t height = HIWORD(lParam);
            renderer->Resize(width, height);
        }
        break;
    case WM_KEYDOWN:
        // 更新键盘状态数组
        keys[wParam] = true;
        
        // 处理一次性按键事件（如ESC）
        switch (wParam) {
        case VK_ESCAPE:
            running = false;
            break;
        case VK_F9:
            if (!f9Pressed) {  // 只在按键状态变化时处理
                debugOutputEnabled = !debugOutputEnabled;
                f9Pressed = true;
                
                // 显示调试状态切换消息
                const char* statusMsg = debugOutputEnabled ? "Debug output ENABLED" : "Debug output DISABLED";
                std::cout << "[F9] " << statusMsg << std::endl;
            }
            break;
        default:
            break;
        }
        break;
    case WM_KEYUP:
        // 更新键盘状态数组
        keys[wParam] = false;
        
        // 重置F9键按下标志
        if (wParam == VK_F9) {
            f9Pressed = false;
        }
        break;
    case WM_RBUTTONDOWN:
        // 鼠标右键按下时开始捕获鼠标
        SetCapture(hWnd);
        ShowCursor(FALSE);
        mouseCaptured = true;
        firstMouse = true;
        // 保存当前鼠标位置
        lastX = GET_X_LPARAM(lParam);
        lastY = GET_Y_LPARAM(lParam);
        break;
    case WM_RBUTTONUP:
        // 鼠标右键释放时释放鼠标捕获
        ReleaseCapture();
        ShowCursor(TRUE);
        mouseCaptured = false;
        break;
    case WM_MOUSEMOVE:
        if (mouseCaptured) {
            int xPos = GET_X_LPARAM(lParam);
            int yPos = GET_Y_LPARAM(lParam);

            if (firstMouse) {
                // 忽略第一次移动，避免初始跳跃
                lastX = xPos;
                lastY = yPos;
                firstMouse = false;
                return 0; // 不执行后续旋转逻辑
            }

            // 计算鼠标移动的相对量
            float xoffset = xPos - lastX;
            float yoffset = lastY - yPos; // 注意这里是相反的，因为y坐标是向下增长的

            // 保存当前位置供下次计算
            lastX = xPos;
            lastY = yPos;

            // 使用非常低的灵敏度以获得更平滑的控制
            float sensitivity = 0.001f;
            xoffset *= sensitivity;
            yoffset *= sensitivity;

            // 计算相机当前朝向
            dx::XMVECTOR pos = camera->GetPosition();
            dx::XMVECTOR target = camera->GetTarget();
            dx::XMVECTOR up = camera->GetUp();
            dx::XMVECTOR front = dx::XMVector3Normalize(dx::XMVectorSubtract(target, pos));
            dx::XMVECTOR right = dx::XMVector3Normalize(dx::XMVector3Cross(front, up));

            // 根据鼠标移动旋转相机朝向
            // 绕up轴旋转（yaw）
            dx::XMMATRIX rotationMatrix = dx::XMMatrixRotationAxis(up, xoffset);
            front = dx::XMVector3TransformNormal(front, rotationMatrix);
            front = dx::XMVector3Normalize(front);

            // 绕right轴旋转（pitch）
            rotationMatrix = dx::XMMatrixRotationAxis(right, yoffset);
            front = dx::XMVector3TransformNormal(front, rotationMatrix);
            front = dx::XMVector3Normalize(front);

            // 更新相机目标
            target = dx::XMVectorAdd(pos, front);
            camera->SetTarget(target);
        }
        break;
    case WM_MOUSEWHEEL:
        // 处理鼠标滚轮（缩放）
        {
            float zoomFactor = 1.1f;
            if (GET_WHEEL_DELTA_WPARAM(wParam) < 0)
                zoomFactor = 1.0f / zoomFactor;

            dx::XMVECTOR pos = camera->GetPosition();
            dx::XMVECTOR target = camera->GetTarget();
            dx::XMVECTOR dir = dx::XMVectorSubtract(pos, target);
            dir = dx::XMVectorScale(dir, zoomFactor);
            pos = dx::XMVectorAdd(target, dir);
            camera->SetPosition(pos);
        }
        break;
    case WM_CLOSE:
        running = false;
        break;
    case WM_DESTROY:
        PostQuitMessage(0);
        break;
    default:
        return DefWindowProc(hWnd, message, wParam, lParam);
    }
    return 0;
}

// 注册窗口类
BOOL RegisterWindowClass(HINSTANCE hInstance) {
    WNDCLASSEX wc;
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.style = CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc = WndProc;
    wc.cbClsExtra = 0;
    wc.cbWndExtra = 0;
    wc.hInstance = hInstance;
    wc.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.lpszMenuName = NULL;
    wc.lpszClassName = TEXT("DX12ClothSimulator");
    wc.hIconSm = LoadIcon(NULL, IDI_APPLICATION);

    return RegisterClassEx(&wc);
}

// 创建窗口
BOOL CreateWindowApp(HINSTANCE hInstance) {
    // 注册窗口类
    if (!RegisterWindowClass(hInstance)) {
        MessageBox(NULL, TEXT("RegisterWindowClass failed!"), TEXT("Error"), MB_ICONERROR);
        return FALSE;
    }

    // 获取系统屏幕分辨率
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);

    // 创建全屏窗口
    hWnd = CreateWindow(
        TEXT("DX12ClothSimulator"),  // 窗口类名称
        TEXT("XPBD Cloth Simulator (DirectX 12)"),  // 窗口标题
        WS_POPUP,                    // 无边框全屏窗口样式
        0, 0,                        // 窗口位置（全屏时从(0,0)开始）
        screenWidth, screenHeight,   // 窗口尺寸（使用屏幕分辨率）
        NULL,                        // 父窗口
        NULL,                        // 菜单
        hInstance,                   // 实例句柄
        NULL                         // 附加数据
    );

    if (!hWnd) {
        MessageBox(NULL, TEXT("CreateWindow failed!"), TEXT("Error"), MB_ICONERROR);
        return FALSE;
    }

    // 显示窗口
    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);

    return TRUE;
}

// 处理Windows消息
void ProcessMessages() {
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

// 初始化DirectX 12渲染器
BOOL InitializeRenderer() {
    std::cout << "  - Converting window title to wide character..." << std::endl;
    // 转换窗口标题为宽字符
    std::wstring windowName(L"XPBD Cloth Simulator");

    std::cout << "  - Creating DX12Renderer object..." << std::endl;
    // 获取系统屏幕分辨率
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    // 创建渲染器实例，传入正确顺序的参数和全屏尺寸
    renderer = new DX12Renderer(screenWidth, screenHeight, windowName, hWnd);
    std::cout << "  - DX12Renderer object created successfully" << std::endl;
    
    // 创建相机对象
    camera = new Camera(screenWidth, screenHeight);
    std::cout << "  - Camera object created successfully" << std::endl;
    
    // 设置相机初始位置和目标
    dx::XMVECTOR cameraPos = dx::XMVectorSet(0.0f, 10.0f, 15.0f, 1.0f); // 直接在布料正前方
    dx::XMVECTOR cameraTarget = dx::XMVectorSet(0.0f, 5.0f, 0.0f, 1.0f); // 直接指向布料中心
    dx::XMVECTOR cameraUp = dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    UpdateCamera(cameraPos, cameraTarget, cameraUp);

    std::cout << "  - Calling renderer->Initialize()..." << std::endl;
    // 初始化渲染器
    if (!renderer->Initialize()) {
        std::cerr << "  - renderer->Initialize() failed!" << std::endl;
        MessageBox(NULL, TEXT("Failed to initialize DirectX 12 renderer!"), TEXT("Error"), MB_ICONERROR);
        return FALSE;
    }
    std::cout << "  - renderer->Initialize() succeeded" << std::endl;

    return TRUE;
}

// 生成球体数据
void GenerateSphereData(float radius, uint32_t sectors, uint32_t stacks,
    std::vector<dx::XMFLOAT3>& positions, std::vector<dx::XMFLOAT3>& normals, std::vector<uint32_t>& indices) {
    // 清空现有数据
    positions.clear();
    normals.clear();
    indices.clear();

    // 生成顶点和法线
    for (uint32_t i = 0; i <= stacks; ++i) {
        float phi = dx::XMConvertToRadians(180.0f * static_cast<float>(i) / stacks); // 极角
        float sinPhi = sin(phi);
        float cosPhi = cos(phi);

        for (uint32_t j = 0; j <= sectors; ++j) {
            float theta = dx::XMConvertToRadians(360.0f * static_cast<float>(j) / sectors); // 方位角
            float sinTheta = sin(theta);
            float cosTheta = cos(theta);

            // 添加顶点位置
            dx::XMFLOAT3 pos;
            pos.x = radius * sinPhi * cosTheta;
            pos.y = radius * cosPhi;
            pos.z = radius * sinPhi * sinTheta;
            positions.push_back(pos);

            // 添加法线（单位向量）
            dx::XMFLOAT3 normal;
            normal.x = sinPhi * cosTheta;
            normal.y = cosPhi;
            normal.z = sinPhi * sinTheta;
            normals.push_back(normal);
        }
    }

    // 生成索引
    for (uint32_t i = 0; i < stacks; ++i) {
        uint32_t row1 = i * (sectors + 1);
        uint32_t row2 = (i + 1) * (sectors + 1);

        for (uint32_t j = 0; j < sectors; ++j) {
            // 第一个三角形
            indices.push_back(row1 + j);
            indices.push_back(row2 + j + 1);
            indices.push_back(row1 + j + 1);

            // 第二个三角形
            indices.push_back(row1 + j);
            indices.push_back(row2 + j);
            indices.push_back(row2 + j + 1);
        }
    }
}

// 全局渲染计数器，用于控制调试输出频率
int globalRenderFrameCount = 0;

// 更新布料渲染数据
void UpdateClothRenderData() {
    if (debugOutputEnabled) {
        globalRenderFrameCount++;
    }
    
    if (!cloth || !renderer) {
        if (debugOutputEnabled) {
            std::cout << "UpdateClothRenderData: cloth or renderer is null" << std::endl;
        }
        return;
    }
    
    const std::vector<Particle>& particles = cloth->getParticles();
    int width = cloth->getWidth();
    int height = cloth->getHeight();
    
    if (debugOutputEnabled) {
        std::cout << "UpdateClothRenderData: Frame " << globalRenderFrameCount << ", particles count = " << particles.size() << ", width = " << width << ", height = " << height << std::endl;
        
        // 调试输出第一个粒子的位置和状态
        if (!particles.empty()) {
            std::cout << "First particle: position = (" << particles[0].position.x << ", " << particles[0].position.y << ", " << particles[0].position.z << "), "
                      << "velocity = (" << particles[0].velocity.x << ", " << particles[0].velocity.y << ", " << particles[0].velocity.z << "), "
                      << "isStatic = " << (particles[0].isStatic ? "true" : "false") << std::endl;
        }
        
        // 每10帧输出一次特定行的粒子位置，以便观察布料变形情况
        if (globalRenderFrameCount % 10 == 0 && width > 10 && height > 5) {
            std::cout << "\n--- Frame " << globalRenderFrameCount << " Selected Particle Positions ---" << std::endl;
            // 输出中间行的粒子位置
            int middleRow = height / 2;
            for (int x = 0; x < width; x += 5) { // 每隔5个粒子输出一个
                int index = middleRow * width + x;
                if (index < particles.size()) {
                    std::cout << "Particle (" << x << ", " << middleRow << "): ("
                              << particles[index].position.x << ", "
                              << particles[index].position.y << ", "
                              << particles[index].position.z << ")" << std::endl;
                }
            }
            std::cout << "--------------------------------------\n" << std::endl;
        }
    }
    
    // 创建顶点数据（位置和法线）
    std::vector<dx::XMFLOAT3> positions;
    std::vector<dx::XMFLOAT3> normals;
    std::vector<uint32_t> indices;
    
    // 计算法线（使用面法线的平均值）
    std::vector<dx::XMFLOAT3> vertexNormals(particles.size(), dx::XMFLOAT3(0.0f, 0.0f, 0.0f));
    
    // 生成三角形面和计算法线
    for (int y = 0; y < height - 1; ++y) {
        for (int x = 0; x < width - 1; ++x) {
            // 第一个三角形：(x,y), (x+1,y), (x+1,y+1)
            int i1 = y * width + x;
            int i2 = y * width + x + 1;
            int i3 = (y + 1) * width + x + 1;
            
            indices.push_back(i1);
            indices.push_back(i2);
            indices.push_back(i3);
            
            // 计算面法线 - 反转法线方向
            dx::XMVECTOR v1 = dx::XMVectorSubtract(dx::XMLoadFloat3(&particles[i2].position), dx::XMLoadFloat3(&particles[i1].position));
            dx::XMVECTOR v2 = dx::XMVectorSubtract(dx::XMLoadFloat3(&particles[i3].position), dx::XMLoadFloat3(&particles[i1].position));
            dx::XMVECTOR crossProduct = dx::XMVector3Cross(v2, v1); // 反转叉乘顺序以反转法线方向
            
            // 检查叉乘结果是否为零向量，避免NaN
            float lengthSquared = dx::XMVectorGetX(dx::XMVector3LengthSq(crossProduct));
            dx::XMVECTOR normal;
            
            if (lengthSquared > 0.0001f) {
                normal = dx::XMVector3Normalize(crossProduct);
            } else {
                // 如果叉乘结果接近零，使用默认法线
                normal = dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); // 向上的法线
            }
            
            // 累加到顶点法线
            dx::XMVECTOR n1 = dx::XMLoadFloat3(&vertexNormals[i1]);
            dx::XMVECTOR n2 = dx::XMLoadFloat3(&vertexNormals[i2]);
            dx::XMVECTOR n3 = dx::XMLoadFloat3(&vertexNormals[i3]);
            
            dx::XMStoreFloat3(&vertexNormals[i1], dx::XMVectorAdd(n1, normal));
            dx::XMStoreFloat3(&vertexNormals[i2], dx::XMVectorAdd(n2, normal));
            dx::XMStoreFloat3(&vertexNormals[i3], dx::XMVectorAdd(n3, normal));
            
            // 第二个三角形：(x,y), (x+1,y+1), (x,y+1)
            i1 = y * width + x;
            i2 = (y + 1) * width + x + 1;
            i3 = (y + 1) * width + x;
            
            indices.push_back(i1);
            indices.push_back(i2);
            indices.push_back(i3);
            
            // 计算面法线 - 反转法线方向
            v1 = dx::XMVectorSubtract(dx::XMLoadFloat3(&particles[i2].position), dx::XMLoadFloat3(&particles[i1].position));
            v2 = dx::XMVectorSubtract(dx::XMLoadFloat3(&particles[i3].position), dx::XMLoadFloat3(&particles[i1].position));
            crossProduct = dx::XMVector3Cross(v2, v1); // 反转叉乘顺序以反转法线方向
            
            // 检查叉乘结果是否为零向量，避免NaN
            lengthSquared = dx::XMVectorGetX(dx::XMVector3LengthSq(crossProduct));
            
            if (lengthSquared > 0.0001f) {
                normal = dx::XMVector3Normalize(crossProduct);
            } else {
                // 如果叉乘结果接近零，使用默认法线
                normal = dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f); // 向上的法线
            }
            
            // 累加到顶点法线
            n1 = dx::XMLoadFloat3(&vertexNormals[i1]);
            n2 = dx::XMLoadFloat3(&vertexNormals[i2]);
            n3 = dx::XMLoadFloat3(&vertexNormals[i3]);
            
            dx::XMStoreFloat3(&vertexNormals[i1], dx::XMVectorAdd(n1, normal));
            dx::XMStoreFloat3(&vertexNormals[i2], dx::XMVectorAdd(n2, normal));
            dx::XMStoreFloat3(&vertexNormals[i3], dx::XMVectorAdd(n3, normal));
        }
    }
    
    // 归一化顶点法线并准备位置和法线数据
    for (size_t i = 0; i < particles.size(); ++i) {
        positions.push_back(particles[i].position);
        
        // 归一化顶点法线，添加检查避免对零向量进行归一化
        dx::XMVECTOR n = dx::XMLoadFloat3(&vertexNormals[i]);
        float lengthSquared = dx::XMVectorGetX(dx::XMVector3LengthSq(n));
        dx::XMFLOAT3 normalizedNormal;
        
        if (lengthSquared > 0.0001f) {
            n = dx::XMVector3Normalize(n);
            dx::XMStoreFloat3(&normalizedNormal, n);
        } else {
            // 如果顶点法线接近零，使用默认法线
            normalizedNormal = dx::XMFLOAT3(0.0f, 1.0f, 0.0f); // 向上的法线
        }
        
        normals.push_back(normalizedNormal);
    }
    
    // 只有在启用调试输出时才打印详细信息
    if (debugOutputEnabled) {
        std::cout << "UpdateClothRenderData: generated " << positions.size() << " positions, " << normals.size() << " normals, " << indices.size() << " indices" << std::endl;
        
        // 每10帧输出一次部分顶点和法线数据进行验证
        if (globalRenderFrameCount % 10 == 0) {
            std::cout << "\n--- Frame " << globalRenderFrameCount << " Selected Vertex Data ---" << std::endl;
            // 输出前5个顶点的位置和法线
            int outputCount = (positions.size() < 5) ? static_cast<int>(positions.size()) : 5;
            for (int i = 0; i < outputCount; i++) {
                std::cout << "Vertex " << i << ": position = (" 
                          << positions[i].x << ", " << positions[i].y << ", " << positions[i].z << "), "
                          << "normal = (" 
                          << normals[i].x << ", " << normals[i].y << ", " << normals[i].z << ")" << std::endl;
            }
            
            // 检查是否有NaN值
            bool hasNaN = false;
            for (int i = 0; i < normals.size(); i++) {
                if (std::isnan(normals[i].x) || std::isnan(normals[i].y) || std::isnan(normals[i].z)) {
                    std::cout << "WARNING: NaN found in normal at index " << i << std::endl;
                    hasNaN = true;
                    break;
                }
            }
            if (!hasNaN) {
                std::cout << "All normals are valid (no NaN values)" << std::endl;
            }
            std::cout << "--------------------------------------\n" << std::endl;
        }
        
        // 更新渲染器中的布料顶点数据
        std::cout << "UpdateClothRenderData: Calling SetClothVertices with " << positions.size() << " positions" << std::endl;
    }
    
    renderer->SetClothVertices(positions, normals, indices);
    
    if (debugOutputEnabled) {
        std::cout << "UpdateClothRenderData: SetClothVertices called" << std::endl;
    }
}

// 更新相机的辅助函数（转换为XMVECTOR版本）
void UpdateCamera(const dx::XMVECTOR& position, const dx::XMVECTOR& target, const dx::XMVECTOR& up) {
    if (renderer) {
        renderer->UpdateCamera(position, target, up);
    }
    if (camera) {
        camera->UpdateCamera(position, target, up);
    }
}

// 清理资源
void Cleanup() {
    // 清理布料对象
    if (cloth) {
        delete cloth;
        cloth = nullptr;
    }
    
    // 清理渲染器
    if (renderer) {
        renderer->Cleanup();
        delete renderer;
        renderer = nullptr;
    }
    
    // 清理相机对象
    if (camera) {
        delete camera;
        camera = nullptr;
    }
}

// main函数
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    // 保存实例句柄
    ::hInstance = hInstance;
    
    // 创建一个单独的控制台窗口，以确保输出可见
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    
    // 无条件输出一些基本信息
    std::cout << "[TEST] Console window created" << std::endl;
    
    // 解析命令行参数
    std::string cmdLine(lpCmdLine);
    
    // 简单解析命令行参数，寻找 -maxFrames:xxx 格式的参数
    size_t maxFramesPos = cmdLine.find("-maxFrames:");
    if (maxFramesPos != std::string::npos) {
        try {
            size_t start = maxFramesPos + 11; // "-maxFrames:" 长度为11
            size_t end = cmdLine.find(' ', start);
            if (end == std::string::npos) {
                end = cmdLine.length();
            }
            std::string maxFramesStr = cmdLine.substr(start, end - start);
            maxFrames = std::stoi(maxFramesStr);
            std::cout << "Max frames set to: " << maxFrames << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error parsing maxFrames parameter: " << e.what() << std::endl;
            std::cerr << "Using default maxFrames value (-1 = unlimited)" << std::endl;
            maxFrames = -1; // 恢复为默认值
        }
    }
    
    if (cmdLine.find("--debug") != std::string::npos || cmdLine.find("-d") != std::string::npos) {
        debugOutputEnabled = true;
        std::cout << "[DEBUG] Debug output enabled via command line parameter: " << cmdLine << std::endl;
    } else {
        std::cout << "[INFO] Running in normal mode (debug output disabled)" << std::endl;
    }
    
    std::cout << "[INFO] Program started" << std::endl;
    std::cout << "[INFO] Press F9 to toggle debug output" << std::endl;
    std::cout.flush();
    
    // 创建窗口
    std::cout << "Creating window..." << std::endl;
    if (!CreateWindowApp(hInstance)) {
        std::cerr << "Failed to create window" << std::endl;
        return -1;
    }
    std::cout << "Window created successfully" << std::endl;
    
    // 初始化DirectX 12渲染器
    std::cout << "Initializing renderer..." << std::endl;
    std::cout << "  - Creating DX12Renderer instance..." << std::endl;
    if (!InitializeRenderer()) {
        std::cerr << "Failed to initialize renderer" << std::endl;
        MessageBox(hWnd, L"Failed to initialize DirectX 12 renderer", L"Error", MB_OK | MB_ICONERROR);
        Cleanup();
        return -1;
    }
    std::cout << "Renderer initialized successfully" << std::endl;
    
    // 创建布料对象，调整位置使布料正中心对准球体(0,0,0)
    std::cout << "Creating cloth object..." << std::endl;
    cloth = new Cloth(dx::XMFLOAT3(-5.0f, 5.0f, -5.0f), 40, 40, 10.0f, 1.0f); // 布料左上角位置，增加分辨率到40x40
    
    // 默认启用XPBD碰撞约束
    cloth->setUseXPBDCollision(true);
    
    std::cout << "Cloth object created successfully" << std::endl;
    
    // 初始化球体数据
    std::cout << "Initializing sphere data..." << std::endl;
    std::vector<dx::XMFLOAT3> spherePositions;
    std::vector<dx::XMFLOAT3> sphereNormals;
    std::vector<uint32_t> sphereIndices;
    GenerateSphereData(2.0f, 32, 32, spherePositions, sphereNormals, sphereIndices);
    renderer->SetSphereVertices(spherePositions, sphereNormals, sphereIndices);
    std::cout << "Sphere data initialized successfully" << std::endl;
    
    // 初始化时间
    lastFrame = static_cast<float>(GetTickCount64()) / 1000.0f;
    
    std::cout << "Entering main loop..." << std::endl;
    // 主循环
    while (running) {
        // 处理Windows消息
        ProcessMessages();
        
        // 计算帧率
        float currentFrame = static_cast<float>(GetTickCount64()) / 1000.0f;
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        
        // 限制最大帧率以避免CPU过载
        if (deltaTime < 1.0f / 60.0f) {
            Sleep(static_cast<DWORD>((1.0f / 60.0f - deltaTime) * 1000.0f));
            currentFrame = static_cast<float>(GetTickCount64()) / 1000.0f;
            deltaTime = currentFrame - lastFrame;
            lastFrame = currentFrame;
        }
        
        // 更新布料模拟
        if (cloth) {
            // 恢复布料物理更新
            cloth->update(deltaTime);
            
            // 增加帧计数器
            frameCount++;
            
            // 如果设置了最大帧数限制且已达到，则退出程序
            if (maxFrames > 0 && frameCount >= maxFrames) {
                std::cout << "Reached maximum frames (" << maxFrames << "), exiting..." << std::endl;
                running = false;
                continue;
            }
            
            // 定期输出当前帧数
            if (debugOutputEnabled && frameCount % 30 == 0) {
                std::cout << "Current frame: " << frameCount << ", deltaTime: " << deltaTime << std::endl;
            }
            
            // 恢复渲染数据更新
            UpdateClothRenderData();
            
            // 调用UpdateCamera函数，传入渲染器需要的相机参数
            UpdateCamera(camera->GetPosition(), camera->GetTarget(), camera->GetUp());
        }
        
        // 处理键盘输入
        camera->ProcessKeyboardInput(keys, deltaTime);
        
        // 可以在这里添加按某个键切换碰撞检测模式的功能
        // 例如：按'C'键切换传统碰撞和XPBD碰撞
        
        // 渲染场景
        if (renderer) {
            if (debugOutputEnabled) {
                static int renderCount = 0;
                renderCount++;
                if (renderCount % 30 == 0) {
                    std::cout << "Rendering scene, render count: " << renderCount << std::endl;
                }
            }
            renderer->Render();
        }
    }
    
    std::cout << "Exiting main loop" << std::endl;
    // 清理资源
    Cleanup();
    std::cout << "Resources cleaned up" << std::endl;
    
    return 0;
}
