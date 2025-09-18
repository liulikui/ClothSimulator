#include "Scene.h"
#include "Primitive.h"
#include "Cloth.h"
#include "Sphere.h"
#include "DX12Renderer.h"
#include <algorithm>
#include <iostream>
#include <string>

// 日志函数声明
extern void logDebug(const std::string& message);

Scene::Scene() {
    // 初始化场景
    // cameraConstBuffer将在渲染器中创建并传入
}

Scene::~Scene() {
    // 清空场景中的所有对象
    clear();
}

void Scene::update(float deltaTime) {
    // 更新场景中所有可见对象的状态
    for (auto& primitive : primitives) {
        if (primitive && primitive->isVisible()) {
            primitive->update(deltaTime);
        }
    }
}

bool Scene::addPrimitive(std::shared_ptr<Primitive> primitive) {
    if (!primitive) {
        return false;
    }

    // 检查对象是否已经在场景中
    auto it = std::find(primitives.begin(), primitives.end(), primitive);
    if (it != primitives.end()) {
        return false;
    }

    // 添加对象到场景中
    primitives.push_back(primitive);
    return true;
}

bool Scene::removePrimitive(std::shared_ptr<Primitive> primitive) {
    if (!primitive) {
        return false;
    }

    // 查找并移除对象
    auto it = std::find(primitives.begin(), primitives.end(), primitive);
    if (it != primitives.end()) {
        primitives.erase(it);
        return true;
    }

    return false;
}

void Scene::clear() {
    // 清空所有对象
    primitives.clear();
}

void Scene::render(DX12Renderer* renderer, const dx::XMMATRIX& viewMatrix, const dx::XMMATRIX& projectionMatrix) {
    logDebug("[DEBUG] Scene::render called");
    if (!renderer) {
        logDebug("[DEBUG] renderer is null");
        return;
    }

    // 设置光源位置和颜色
    logDebug("[DEBUG] Setting light position and color");
    renderer->UpdateLightPosition(lightPosition);
    renderer->UpdateLightColor(lightColor);

    // 渲染每个可见的Primitive对象
    logDebug("[DEBUG] Number of primitives in scene: " + std::to_string(primitives.size()));
    for (size_t i = 0; i < primitives.size(); ++i) {
        auto& primitive = primitives[i];
        logDebug("[DEBUG] Primitive " + std::to_string(i) + ": " + std::to_string(reinterpret_cast<uintptr_t>(primitive.get())));
        if (primitive && primitive->isVisible()) {
            logDebug("[DEBUG] Primitive " + std::to_string(i) + " is visible");
            
            // 获取对象类型
            std::string typeName = typeid(*primitive).name();
            logDebug("[DEBUG] Primitive " + std::to_string(i) + " type: " + typeName);
            
            // 获取对象的世界矩阵、材质颜色、顶点数据和索引数据
            const dx::XMMATRIX& worldMatrix = primitive->getWorldMatrix();
            const dx::XMFLOAT4& diffuseColor = primitive->getDiffuseColor();
            const std::vector<dx::XMFLOAT3>& positions = primitive->getPositions();
            const std::vector<dx::XMFLOAT3>& normals = primitive->getNormals();
            const std::vector<uint32_t>& indices = primitive->getIndices();

            logDebug("[DEBUG] Primitive " + std::to_string(i) + " has " + std::to_string(positions.size()) + " positions, " + 
                     std::to_string(normals.size()) + " normals, " + std::to_string(indices.size()) + " indices");
            
            // 检查是否有有效数据
            if (positions.empty() || normals.empty() || indices.empty()) {
                logDebug("[DEBUG] Primitive " + std::to_string(i) + " has empty data, skipping");
                continue;
            }

            // 渲染对象（具体的渲染方式将在DX12Renderer中实现为更通用的接口）
            if (typeid(*primitive) == typeid(Cloth)) {
                logDebug("[DEBUG] Rendering cloth primitive");
                renderer->SetClothVertices(positions, normals, indices);
                renderer->RenderPrimitive(worldMatrix, diffuseColor, true);
            } else if (typeid(*primitive) == typeid(Sphere)) {
                logDebug("[DEBUG] Rendering sphere primitive");
                renderer->SetSphereVertices(positions, normals, indices);
                renderer->RenderPrimitive(worldMatrix, diffuseColor, false);
            } else {
                logDebug("[DEBUG] Unknown primitive type, skipping");
            }
        } else {
            logDebug("[DEBUG] Primitive " + std::to_string(i) + " is null or not visible");
        }
    }
    logDebug("[DEBUG] Scene::render finished");
}