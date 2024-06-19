#pragma once

#include <string_view>

#include "../tt_fbx/fbxLoader.h"
#include "../tt_fbx/meshParser.h"
#include "../tt_fbx/sceneParser.h"

#include "../tt_cpplib/windont.h"
#include "../tt_cpplib/tt_files.h"

namespace TT {
    class FbxExtractor {
        std::vector<MultiMeshData> _meshes;
        std::vector<Node> _nodes;

        FbxExtractor(const FbxExtractor&) = delete;
        FbxExtractor(FbxExtractor&&) = delete;
        FbxExtractor& operator=(const FbxExtractor&) = delete;
        FbxExtractor& operator=(FbxExtractor&&) = delete;

    public:
        ~FbxExtractor();
        FbxExtractor(const std::string_view filePath, /*FbxAxisSystem::EUpVector*/int up = 2, /*FbxAxisSystem::EFrontVector*/ int front = 2, /*FbxAxisSystem::ECoordSystem*/ int flip = 0, Units unit = Units::m);
        void saveToCache(const std::string_view filePath) const;
        bool loadFromCache(const std::string_view filePath);
        const std::vector<MultiMeshData>& meshes() const { return _meshes; }
        const std::vector<Node>& nodes() const { return _nodes; }
    };
}
