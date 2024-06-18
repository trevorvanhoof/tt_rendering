#pragma once

#include <string_view>

#include "../tt_fbx/fbxLoader.h"
#include "../tt_fbx/meshParser.h"
#include "../tt_fbx/sceneParser.h"

#include "../tt_cpplib/windont.h"
#include "../tt_cpplib/tt_files.h"

namespace TT {
    class FbxScene {
        std::vector<MultiMeshData> meshes;
        std::vector<Node> nodes;

        FbxScene(const FbxScene&) = delete;
        FbxScene(FbxScene&&) = delete;
        FbxScene& operator=(const FbxScene&) = delete;
        FbxScene& operator=(FbxScene&&) = delete;

    public:
        ~FbxScene();
        FbxScene(const std::string_view filePath, FbxAxisSystem::EUpVector up = FbxAxisSystem::eYAxis, FbxAxisSystem::EFrontVector front = FbxAxisSystem::eParityOdd, FbxAxisSystem::ECoordSystem flip = FbxAxisSystem::eRightHanded, Units unit = Units::m);
        void SaveToCache(const std::string_view filePath) const;
        bool LoadFromCache(const std::string_view filePath);
    };
}
