#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <iostream>
#include <vector>
#include <cstdint>
#include <fstream>
#include <string>
#include "Cloth.h"
#include "DX12RALDevice.h"
#include "Camera.h"
#include "Sphere.h"
#include "Scene.h"
#include <windowsx.h>

// 日志文件
std::ofstream logFile;

// 日志函数
extern void logDebug(const std::string& message)
{
    //std::cout << message << std::endl;
    if (logFile.is_open())
    {
        logFile << message << std::endl;
        //logFile.flush();
    }
}

// 初始化日志文件
void initLogFile()
{
    logFile.open("debug_log.txt", std::ios::out | std::ios::trunc);
    if (logFile.is_open())
    {
        logFile << "[LOG] Debug log started." << std::endl;
    } else {
        std::cerr << "Failed to open log file!" << std::endl;
    }
}

// 关闭日志文件
void closeLogFile()
{
    if (logFile.is_open())
    {
        logFile.flush();
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
int iteratorCount = 50;        // XPBD求解器迭代次数，默认50
int widthResolution = 40;      // 布料宽度分辨率（粒子数），默认40
int heightResolution = 40;     // 布料高度分辨率（粒子数），默认40

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

// 高精度计时器变量
LARGE_INTEGER frequency;
LARGE_INTEGER lastCounter;
float deltaTime = 0.0f;

// 前向声明
void UpdateCamera(const dx::XMVECTOR& position, const dx::XMVECTOR& target, const dx::XMVECTOR& up);

// 布料、渲染设备和场景对象
IRALDevice* device = nullptr;
Scene* scene = nullptr;
Sphere* sphere = nullptr;
Cloth* cloth = nullptr;

// 窗口过程函数
LRESULT CALLBACK WndProc(HWND hWnd, uint32_t message, WPARAM wParam, LPARAM lParam)
{
    switch (message)
    {
    case WM_SIZE:
        if (wParam != SIZE_MINIMIZED && device)
        {
            uint32_t width = LOWORD(lParam);
            uint32_t height = HIWORD(lParam);
            device->Resize(width, height);
            
            // 同时更新相机的尺寸
            if (camera)
            {
                camera->Resize(width, height);
            }
        }
        break;
    case WM_KEYDOWN:
        // 更新键盘状态数组
        keys[wParam] = true;
        
        // 处理一次性按键事件（如ESC）
        switch (wParam)
        {
        case VK_ESCAPE:
            running = false;
            break;
        case VK_F9:
            if (!f9Pressed)
            {  // 只在按键状态变化时处理
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
        if (wParam == VK_F9)
        {
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
        if (mouseCaptured)
        {
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
    if (!RegisterWindowClass(hInstance))
    {
        MessageBox(NULL, TEXT("RegisterWindowClass failed!"), TEXT("Error"), MB_ICONERROR);
        return FALSE;
    }

    // 使用固定窗口尺寸800*600
    int screenWidth = 800;
    int screenHeight = 600;

    // 创建窗口（使用窗口样式，不再是全屏模式）
    hWnd = CreateWindow(
        TEXT("DX12ClothSimulator"),  // 窗口类名称
        TEXT("XPBD Cloth Simulator (DirectX 12)"),  // 窗口标题
        WS_OVERLAPPEDWINDOW,         // 窗口样式
        CW_USEDEFAULT, CW_USEDEFAULT,// 窗口位置
        screenWidth, screenHeight,   // 窗口尺寸（使用固定尺寸800*600）
        NULL,                        // 父窗口
        NULL,                        // 菜单
        hInstance,                   // 实例句柄
        NULL                         // 附加数据
    );

    if (!hWnd)
    {
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
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

// 初始化DirectX 12渲染设备
BOOL InitializeDevice()
{
    std::cout << "  - Converting window title to wide character..." << std::endl;
    // 转换窗口标题为宽字符
    std::wstring windowName(L"XPBD Cloth Simulator");

    std::cout << "  - Creating DX12RALDevice object..." << std::endl;
    // 使用固定窗口尺寸800*600
    int screenWidth = 800;
    int screenHeight = 600;
    
    // 创建渲染设备实例，传入正确顺序的参数和窗口尺寸
    device = new DX12RALDevice(screenWidth, screenHeight, windowName, hWnd);
    std::cout << "  - DX12RALDevice object created successfully" << std::endl;
    
    // 创建相机对象，使用窗口尺寸800*600
    camera = new Camera(screenWidth, screenHeight);
    std::cout << "  - Camera object created successfully" << std::endl;
    
    // 设置相机初始位置和目标
    dx::XMVECTOR cameraPos = dx::XMVectorSet(0.0f, 10.0f, 15.0f, 1.0f); // 直接在布料正前方
    dx::XMVECTOR cameraTarget = dx::XMVectorSet(0.0f, 5.0f, 0.0f, 1.0f); // 直接指向布料中心
    dx::XMVECTOR cameraUp = dx::XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
    UpdateCamera(cameraPos, cameraTarget, cameraUp);

    std::cout << "  - Calling device->Initialize()..." << std::endl;
    // 初始化渲染设备
    if (!device->Initialize())
    {
        std::cerr << "  - Failed to initialize DX12RALDevice!" << std::endl;
        MessageBox(NULL, TEXT("Failed to initialize DX12RALDevice!"), TEXT("Error"), MB_ICONERROR);
        return FALSE;
    }
    std::cout << "  - DX12RALDevice initialized successfully" << std::endl;
    
    // 创建场景对象
    std::cout << "  - Creating Scene object..." << std::endl;
    scene = new Scene();
    std::cout << "  - Scene object created successfully" << std::endl;
    
    // 初始化场景，创建根签名
    std::cout << "  - Initializing Scene with device..." << std::endl;
    if (!scene->Initialize(device)) {
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
    if (debugOutputEnabled)
    {
        globalRenderFrameCount++;
    }
    
    if (!cloth || !device)
    {
        if (debugOutputEnabled)
        {
            std::cout << "UpdateClothRenderData: cloth or device is null" << std::endl;
        }
        return;
    }
    
    const std::vector<Particle>& particles = cloth->GetParticles();
    int widthResolution = cloth->GetWidthResolution();
    int heightResolution = cloth->GetHeightResolution();
    
    if (debugOutputEnabled)
    {
        std::cout << "UpdateClothRenderData: Frame " << globalRenderFrameCount << ", particles count = " << particles.size() << ", widthResolution = " << widthResolution << ", heightResolution = " << heightResolution << std::endl;
        
        // 调试输出第一个粒子的位置和状态
        if (!particles.empty()) {
            std::cout << "First particle: position = (" << particles[0].position.x << ", " << particles[0].position.y << ", " << particles[0].position.z << "), "
                      << "velocity = (" << particles[0].velocity.x << ", " << particles[0].velocity.y << ", " << particles[0].velocity.z << "), "
                      << "isStatic = " << (particles[0].isStatic ? "true" : "false") << std::endl;
        }
        
        // 每10帧输出一次特定行的粒子位置，以便观察布料变形情况
        if (globalRenderFrameCount % 10 == 0 && widthResolution > 10 && heightResolution > 5) {
            std::cout << "\n--- Frame " << globalRenderFrameCount << " Selected Particle Positions ---" << std::endl;
            // 输出中间行的粒子位置
            int middleRow = heightResolution / 2;
            for (int x = 0; x < widthResolution; x += 5) { // 每隔5个粒子输出一个
                int index = middleRow * widthResolution + x;
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
    const std::vector<dx::XMFLOAT3>& positions = cloth->GetPositions();
    const std::vector<dx::XMFLOAT3>& normals = cloth->GetNormals();
    const std::vector<uint32_t>& indices = cloth->GetIndices();
    
    // 只有在启用调试输出时才打印详细信息
    if (debugOutputEnabled)
    {
        // 每10帧输出一次部分顶点和法线数据进行验证
        if (globalRenderFrameCount % 10 == 0)
        {
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
        
        // 更新渲染设备中的布料顶点数据
        std::cout << "UpdateClothRenderData: Calling SetClothVertices with " << positions.size() << " positions" << std::endl;
    }
    
    if (debugOutputEnabled)
    {
        std::cout << "UpdateClothRenderData: SetClothVertices called" << std::endl;
    }
}

// 更新相机的辅助函数
void UpdateCamera(const dx::XMVECTOR& position, const dx::XMVECTOR& target, const dx::XMVECTOR& up)
{
    if (camera)
    {
        camera->UpdateCamera(position, target, up);
    }
}

// 清理资源
void Cleanup()
{
    // 清理场景对象
    if (scene)
    {
        scene->Clear();
    }
    
    // 清理布料对象
    if (cloth)
    {
        delete cloth;
        cloth = nullptr;
    }

    if (sphere)
    {
        delete sphere;
        cloth = nullptr;
    }
    
    // 清理场景对象
    if (scene)
    {
        delete scene;
        scene = nullptr;
    }

    // 清理渲染设备
    if (device)
    {
        device->Cleanup();
        delete device;
        device = nullptr;
    }
    
    // 清理相机对象
    if (camera)
    {
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
    
    // 检查是否需要显示帮助信息
    if (cmdLine.find("-help") != std::string::npos)
    {
        std::cout << "XPBD Cloth Simulator (DirectX 12) - 命令行参数帮助" << std::endl;
        std::cout << "===================================================" << std::endl;
        std::cout << "可用的命令行参数：" << std::endl;
        std::cout << "  -help                 显示此帮助信息并退出" << std::endl;
        std::cout << "  -debug                启用调试输出模式" << std::endl;
        std::cout << "  -maxFrames:xxx        设置最大帧数限制（xxx为数字，-1表示不限制）" << std::endl;
        std::cout << "  -iteratorCount:xxx    设置XPBD求解器迭代次数（xxx为数字，默认50）" << std::endl;
        std::cout << "  -widthResolution:xxx  设置布料宽度分辨率（粒子数，xxx为数字，默认40）" << std::endl;
        std::cout << "  -heightResolution:xxx 设置布料高度分辨率（粒子数，xxx为数字，默认40）" << std::endl;
        std::cout << "  -addLRAConstraint:true/false 设置是否添加LRA约束（默认true）" << std::endl;
        std::cout << "===================================================" << std::endl;
        std::cout << "程序控制：" << std::endl;
        std::cout << "  F9                    切换调试输出开关" << std::endl;
        std::cout << "  ESC                   退出程序" << std::endl;
        std::cout << "  W/S/A/D               使用WASD键移动摄像机" << std::endl;
        std::cout << "  鼠标右键 + 移动       旋转相机视角" << std::endl;
        std::cout << "  鼠标滚轮              缩放相机距离" << std::endl;
        
        // 关闭日志文件
        closeLogFile();
        return 0; // 直接退出程序，不创建其他对象
    }

    // 简单解析命令行参数，寻找 -maxFrames:xxx 格式的参数
    size_t maxFramesPos = cmdLine.find("-maxFrames:");
    if (maxFramesPos != std::string::npos)
    {
        size_t start = maxFramesPos + 11; // "-maxFrames:" 长度为11
        size_t end = cmdLine.find(' ', start);
        if (end == std::string::npos) {
            end = cmdLine.length();
        }
        std::string maxFramesStr = cmdLine.substr(start, end - start);
        maxFrames = std::stoi(maxFramesStr);
        logDebug("Max frames set to: " + std::to_string(maxFrames));
    }

    // 解析-iteratorCount:xxx参数
    size_t iteratorCountPos = cmdLine.find("-iteratorCount:");
    if (iteratorCountPos != std::string::npos)
    {
        size_t start = iteratorCountPos + 15; // "-iteratorCount:" 长度为15
        size_t end = cmdLine.find(' ', start);
        if (end == std::string::npos) {
            end = cmdLine.length();
        }
        std::string iteratorCountStr = cmdLine.substr(start, end - start);
        iteratorCount = std::stoi(iteratorCountStr);
        logDebug("Iterator count set to: " + std::to_string(iteratorCount));
    }
    
    // 解析-widthResolution:xxx参数
    size_t widthResPos = cmdLine.find("-widthResolution:");
    if (widthResPos != std::string::npos)
    {
        size_t start = widthResPos + 17; // "-widthResolution:" 长度为18
        size_t end = cmdLine.find(' ', start);
        if (end == std::string::npos) {
            end = cmdLine.length();
        }
        std::string widthResStr = cmdLine.substr(start, end - start);
        widthResolution = std::stoi(widthResStr);

        if (widthResolution < 10)
        {
            widthResolution = 10;
        }
        logDebug("Width resolution set to: " + std::to_string(widthResolution));
    }
    
    // 解析-heightResolution:xxx参数
    size_t heightResPos = cmdLine.find("-heightResolution:");
    if (heightResPos != std::string::npos)
    {
        size_t start = heightResPos + 18; // "-heightResolution:" 长度为18
        size_t end = cmdLine.find(' ', start);
        if (end == std::string::npos) {
            end = cmdLine.length();
        }
        std::string heightResStr = cmdLine.substr(start, end - start);
        heightResolution = std::stoi(heightResStr);

        if (heightResolution < 10)
        {
            heightResolution = 10;
        }

        logDebug("Height resolution set to: " + std::to_string(heightResolution));
    }
    
    if (cmdLine.find("-debug") != std::string::npos)
    {
        debugOutputEnabled = true;
        std::cout << "[DEBUG] Debug output enabled via command line parameter: " << cmdLine << std::endl;
    }
    else
    {
        std::cout << "[INFO] Running in normal mode (debug output disabled)" << std::endl;
    }
    
    std::cout << "[INFO] Program started" << std::endl;
    std::cout << "[INFO] Press F9 to toggle debug output" << std::endl;
    std::cout.flush();
    
    // 创建窗口
    std::cout << "Creating window..." << std::endl;
    if (!CreateWindowApp(hInstance))
    {
        std::cerr << "Failed to create window" << std::endl;
        return -1;
    }
    std::cout << "Window created successfully" << std::endl;
    
    // 初始化DirectX 12渲染设备
    std::cout << "Initializing device..." << std::endl;
    std::cout << "  - Creating DX12RALDevice instance..." << std::endl;
    if (!InitializeDevice())
    {
        std::cerr << "Failed to initialize device" << std::endl;
        MessageBox(hWnd, L"Failed to initialize DirectX 12 device", L"Error", MB_OK | MB_ICONERROR);
        Cleanup();
        return -1;
    }
    std::cout << "Device initialized successfully" << std::endl;
    
    // 创建布料对象，调整位置使布料正中心对准球体(0,0,0)
    std::cout << "Creating cloth object..." << std::endl;

    cloth = new Cloth(widthResolution, heightResolution, 10.0f, 1.0f); // 使用命令行参数设置的分辨率
    
    // 默认启用XPBD碰撞约束
    cloth->SetUseXPBDCollision(true);
    
    // 解析-addLRAConstraint参数
    bool addLRAConstraint = true; // 默认启用
    size_t addLRAConstraintPos = cmdLine.find("-addLRAConstraint:");
    if (addLRAConstraintPos != std::string::npos)
    {
        size_t start = addLRAConstraintPos + 18; // "-addLRAConstraint:" 长度为18
        size_t end = cmdLine.find(' ', start);
        if (end == std::string::npos) {
            end = cmdLine.length();
        }
        std::string addLRAConstraintStr = cmdLine.substr(start, end - start);
        
        // 解析字符串为布尔值
        if (addLRAConstraintStr == "false" || addLRAConstraintStr == "0" || addLRAConstraintStr == "no")
        {
            addLRAConstraint = false;
        }
        else if (addLRAConstraintStr == "true" || addLRAConstraintStr == "1" || addLRAConstraintStr == "yes")
        {
            addLRAConstraint = true;
        }
        
        logDebug("LRA constraint set to: " + std::string(addLRAConstraint ? "true" : "false"));
    }
    
    // 设置是否添加LRA约束
    cloth->SetAddLRAConstraint(addLRAConstraint);
    
    // 设置位置
    cloth->SetPosition(dx::XMFLOAT3(-5.0f, 10.0f, -5.0f));

    // 设置布料的材质颜色（红色）
    cloth->SetDiffuseColor(dx::XMFLOAT3(1.0f, 0.3f, 0.3f));

	cloth->Initialize(device);

    // 设置XPBD求解器迭代次数
    cloth->SetIteratorCount(iteratorCount);
    logDebug("Cloth iterator count set to: " + std::to_string(iteratorCount));

    // 将布料添加到场景中
    scene->AddPrimitive(cloth);

    std::cout << "Sphere object added to scene successfully" << std::endl;

    std::cout << "Cloth object created successfully" << std::endl;
    
    // 创建并初始化球体对象
    dx::XMFLOAT3 sphereCenter(0.0f, 5.0f, 0.0f);
    float sphereRadius = 2.0f;

    std::cout << "Creating sphere object..." << std::endl;
    sphere = new Sphere(sphereRadius, 32, 32);

    // 设置球体的材质颜色（红色）
    sphere->SetDiffuseColor(dx::XMFLOAT3(1.0f, 0.3f, 0.3f));
    
    // 设置球体的世界矩阵
    sphere->SetPosition(dx::XMFLOAT3(0.0f, 5.0f, 0.0f));
    sphere->SetScale(dx::XMFLOAT3(1.0f, 1.0f, 1.0f));
    sphere->SetRotation(dx::XMFLOAT3(0.0f, 0.0f, 0.0f));
    
    sphere->Initialize(device);

    // 初始化球体碰撞约束（一次性创建，避免每帧重建）
    cloth->InitializeSphereCollisionConstraints(sphere->GetPosition(), sphereRadius);

    // 将球体添加到场景中
    scene->AddPrimitive(sphere);
    std::cout << "Sphere object added to scene successfully" << std::endl;

    // 设置场景光源属性
    scene->SetLightPosition(dx::XMFLOAT3(-10.0f, 30.0f, -10.0f));
    scene->SetLightDiffuseColor(dx::XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f));
    
    // 初始化高精度计时器
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&lastCounter);
    
    std::cout << "Entering main loop..." << std::endl;
    // 主循环
    while (running)
    {
        // 处理Windows消息
        ProcessMessages();
        
        // 使用高精度计时器计算帧时间
        LARGE_INTEGER currentCounter;
        QueryPerformanceCounter(&currentCounter);
        deltaTime = static_cast<float>(currentCounter.QuadPart - lastCounter.QuadPart) / static_cast<float>(frequency.QuadPart);
        lastCounter = currentCounter;
        
        // 增加帧计数器
        frameCount++;

        // 计算FPS并更新窗口标题
        static float fpsUpdateTimer = 0.0f;
        static int fpsCounter = 0;
        fpsUpdateTimer += deltaTime;
        fpsCounter++;
        
        // 每秒更新一次窗口标题
        if (fpsUpdateTimer >= 1.0f)
        {
            float fps = static_cast<float>(fpsCounter) / fpsUpdateTimer;
            
            // 构造新的窗口标题
        std::wstring originalTitle = L"XPBD Cloth Simulator (DirectX 12)";
        std::wstring newTitle = originalTitle + L" [" + std::to_wstring(static_cast<int>(fps)) + L" FPS, " + 
                                std::to_wstring(iteratorCount) + L" Iter, " + 
                                std::to_wstring(widthResolution) + L"x" + std::to_wstring(heightResolution) + L" Res]";
            
            // 更新窗口标题
            SetWindowText(hWnd, newTitle.c_str());
            
            // 重置计时器和计数器
            fpsUpdateTimer = 0.0f;
            fpsCounter = 0;
        }

        // 如果设置了最大帧数限制且已达到，则退出程序
        if (maxFrames > 0 && frameCount >= maxFrames)
        {
            std::cout << "Reached maximum frames (" << maxFrames << "), exiting..." << std::endl;
            running = false;
            continue;
        }

        // 定期输出当前帧数
        if (debugOutputEnabled && frameCount % 30 == 0)
        {
            std::cout << "Current frame: " << frameCount << ", deltaTime: " << deltaTime << std::endl;
        }

#ifdef DEBUG_SOLVER
        logDebug("[DEBUG] BeginFrame" + std::to_string(frameCount));
#endif//DEBUG_SOLVER
        device->BeginFrame();

        // 处理键盘输入
        camera->ProcessKeyboardInput(keys, deltaTime);

        // 更新场景
        scene->Update(deltaTime);

        // 渲染场景
        static int renderCount = 0;
        renderCount++;
        logDebug("[DEBUG] Rendering scene, render count: " + std::to_string(renderCount));
        logDebug("[DEBUG] Scene has " + std::to_string(scene->GetMeshCount()) + " meshes");

        scene->Render(camera->GetViewMatrix(), camera->GetProjectionMatrix());

        device->EndFrame();
#ifdef DEBUG_SOLVER
        logDebug("[DEBUG] EndFrame" + std::to_string(frameCount));
#endif//DEBUG_SOLVER
    }
    
    logDebug("Exiting main loop");
    // 清理资源
    Cleanup();
    logDebug("Resources cleaned up");
    
    // 关闭日志文件
    closeLogFile();
    
    return 0;
}
