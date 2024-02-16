#pragma once

#include <raylib-cpp.hpp>
#include <physics.h>
#include <entity.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <assimp/mesh.h>

#include <Jolt/Physics/Collision/Shape/MeshShape.h>

class Map : public Entity {
public:
    raylib::Model model;
    JPH::BodyID bodyId;

    void Load(const char* path);
    void Tick() override;
    void Render() override;
};