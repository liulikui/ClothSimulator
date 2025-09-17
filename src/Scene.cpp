#include "Scene.h"
#include "Primitive.h"
#include <algorithm>

Scene::Scene() {
    // 初始化场景
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