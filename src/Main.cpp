#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <vector>
#include <cstdint>
#include <fstream>
#include <string>
#include "Cloth.h"
#include "DX12Renderer.h"
#include "Camera.h"
#include "Sphere.h"
#include "Scene.h"
#include <windowsx.h>

// 日志文件
std::ofstream logFile;

// 日志函数
void logDebug(const std::string& message)
{
    std::cout << message << std::endl;
    if (logFile.is_open()) {
        logFile << message << std::endl;
        logFile.flush();
    }
}

// 初始化日志文件
void initLogFile()
{
    logFile.open("debug_log.txt", std::ios::out | std::ios::trunc);
    if (logFile.is_open()) {
        logFile << "[LOG] Debug log started." << std::endl;
    } else {
        std::cerr << "Failed to open log file!" << std::endl;
    }
}

// 关闭日志文件
void closeLogFile()
{
    if (logFile.is_open()) {
        logFile << "[LOG] Debug log ended." << std::endl;
        logFile.close();
    }
}

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

// 布料、渲染器和场景对象
std::shared_ptr<Cloth> cloth = nullptr;
DX12Renderer* renderer = nullptr;
Scene* scene = nullptr;

// 窗口过程函数
LRESULT CALLBACK WndProc(HWND hWnd, uint32_t message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED && renderer) {
            uint32_t width = LOWORD(lParam);
            uint32_t height = HIWORD(lParam);
            renderer->Resize(width, height);
            
            // 同时更新相机的尺寸
            if (camera) {
                camera->Resize(width, height);
            }
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
                logDebug("[F9] " + std::string(statusMsg));
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
BOOL RegisterWindowClass(HINSTANCE hInstance)
{
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
BOOL CreateWindowApp(HINSTANCE hInstance)
{
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
void ProcessMessages()
{
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

// 初始化DirectX 12渲染器
BOOL InitializeRenderer()
{
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
    
    // 创建场景对象
    std::cout << "  - Creating Scene object..." << std::endl;
    scene = new Scene();
    std::cout << "  - Scene object created successfully" << std::endl;
    
    // 初始化场景，创建根签名
    std::cout << "  - Initializing Scene with renderer..." << std::endl;
    if (!scene->Initialize(renderer)) {
        std::cerr << "  - scene->Initialize() failed!" << std::endl;
        MessageBox(NULL, TEXT("Failed to initialize Scene!"), TEXT("Error"), MB_ICONERROR);
        return FALSE;
    }
    std::cout << "  - scene->Initialize() succeeded" << std::endl;

    return TRUE;
}

// 全局渲染计数器，用于控制调试输出频率
int globalRenderFrameCount = 0;

// 更新布料渲染数据
void UpdateClothRenderData(std::shared_ptr<Cloth> cloth)
{
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
    
    // 通过getter方法获取渲染数据
    const std::vector<dx::XMFLOAT3>& positions = cloth->getPositions();
    const std::vector<dx::XMFLOAT3>& normals = cloth->getNormals();
    const std::vector<uint32_t>& indices = cloth->getIndices();
    
    // 只有在启用调试输出时才打印详细信息
    if (debugOutputEnabled) {
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
            std::cout << "--------------------------------------\n" << std::endl;
        }
        
        // 更新渲染器中的布料顶点数据
        std::cout << "UpdateClothRenderData: Calling SetClothVertices with " << positions.size() << " positions" << std::endl;
    }
    
    if (debugOutputEnabled) {
        std::cout << "UpdateClothRenderData: SetClothVertices called" << std::endl;
    }
}

// 更新相机的辅助函数
void UpdateCamera(const dx::XMVECTOR& position, const dx::XMVECTOR& target, const dx::XMVECTOR& up)
{
    if (camera) {
        camera->UpdateCamera(position, target, up);
    }
}

// 清理资源
void Cleanup()
{
    // 清理场景对象
    if (scene) {
        delete scene;
        scene = nullptr;
    }
    
    // 清理布料对象
    if (cloth) {
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
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    // 保存实例句柄
    ::hInstance = hInstance;
    
    // 创建一个单独的控制台窗口，以确保输出可见
    AllocConsole();
    freopen("CONOUT$", "w", stdout);
    freopen("CONOUT$", "w", stderr);
    
    // 初始化日志文件
    initLogFile();
    
    // 无条件输出一些基本信息
    logDebug("[TEST] Console window created");
    
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
            logDebug("Max frames set to: " + std::to_string(maxFrames));
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

    cloth = std::make_shared<Cloth>(dx::XMFLOAT3(-5.0f, 5.0f, -5.0f), 40, 40, 10.0f, 1.0f); // 布料左上角位置，增加分辨率到40x40
    
    // 默认启用XPBD碰撞约束
    cloth->setUseXPBDCollision(true);
    
    // 设置布料的材质颜色（蓝色）
    cloth->setDiffuseColor(dx::XMFLOAT4(0.3f, 0.5f, 1.0f, 1.0f));
    // 将布料添加到场景中
    scene->addPrimitive(cloth);
    std::cout << "Sphere object added to scene successfully" << std::endl;

    std::cout << "Cloth object created successfully" << std::endl;
    
    // 创建并初始化球体对象
    std::cout << "Creating sphere object..." << std::endl;
    auto sphere = std::make_shared<Sphere>(dx::XMFLOAT3(0.0f, 0.0f, 0.0f), 2.0f, 32, 32);
    
    // 设置球体的材质颜色（红色）
    sphere->setDiffuseColor(dx::XMFLOAT4(1.0f, 0.3f, 0.3f, 1.0f));
    
    // 设置球体的世界矩阵
    sphere->setPosition(dx::XMFLOAT3(0.0f, 0.0f, 0.0f));
    sphere->setScale(dx::XMFLOAT3(1.0f, 1.0f, 1.0f));
    sphere->setRotation(dx::XMFLOAT3(0.0f, 0.0f, 0.0f));
    
    // 将球体添加到场景中
    scene->addPrimitive(sphere);
    std::cout << "Sphere object added to scene successfully" << std::endl;

    // 设置场景光源属性
    scene->setLightPosition(dx::XMFLOAT4(10.0f, 10.0f, 10.0f, 1.0f));
    scene->setLightColor(dx::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
    
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
        
        renderer->BeginFrame();

        IRALGraphicsCommandList* commandList = renderer->CreateGraphicsCommandList();

        // 处理键盘输入
        camera->ProcessKeyboardInput(keys, deltaTime);

        // 更新场景
        scene->update(commandList, deltaTime);

        // 渲染场景
        static int renderCount = 0;
        renderCount++;
        logDebug("[DEBUG] Rendering scene, render count: " + std::to_string(renderCount));
        logDebug("[DEBUG] Scene has " + std::to_string(scene->getMeshCount()) + " meshes");

        //renderer->Render(scene, camera->GetViewMatrix(), camera->GetProjectionMatrix());

        renderer->EndFrame();
    }
    
    logDebug("Exiting main loop");
    // 清理资源
    Cleanup();
    logDebug("Resources cleaned up");
    
    // 关闭日志文件
    closeLogFile();
    
    return 0;
}
