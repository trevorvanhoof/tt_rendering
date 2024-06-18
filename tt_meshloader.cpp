#include "tt_meshloader.h"

namespace {
    bool FileExists(const std::string_view filePath) {
        DWORD fileAttrib = GetFileAttributesA(filePath.data());
        return (fileAttrib != INVALID_FILE_ATTRIBUTES && !(fileAttrib & FILE_ATTRIBUTE_DIRECTORY));
    }

    uint64_t FileLastWriteTime(const std::string_view filePath) {
        WIN32_FILE_ATTRIBUTE_DATA info = {};
        if(!GetFileAttributesExA(filePath.data(), GetFileExInfoStandard, &info))
            return 0;
        return static_cast<uint64_t>(info.ftLastWriteTime.dwHighDateTime) << 32 | static_cast<uint64_t>(info.ftLastWriteTime.dwLowDateTime);
    }

    std::string CachedFilePath(const std::string_view path) {
        const size_t pathHash = std::hash<std::string>{}(std::string(path));
        return "cache/" + std::to_string(pathHash) + ".bin";
    }

    void readCString(const TT::BinaryReader& reader, String& dst) {
        dst.length = reader.u32();
        dst.buffer = new char[dst.length];
        TT::assert(dst.length == reader.readInto(dst.buffer, dst.length));
    }
}

namespace TT {
    FbxScene::~FbxScene() {
        freeMeshes(meshes.data(), (uint32_t)meshes.size());
        freeNodes(nodes.data(), (uint32_t)nodes.size());
    }

    FbxScene::FbxScene(const std::string_view filePath, FbxAxisSystem::EUpVector up, FbxAxisSystem::EFrontVector front, FbxAxisSystem::ECoordSystem flip, Units unit) {
        if (LoadFromCache(filePath)) 
            return;
        FbxImportContext* context = importFbx(filePath.data(), up, front, flip, unit);
        meshes = TT_FBX::extractMeshes(context);
        nodes = TT_FBX::extractNodes(context);
        freeFbx(context);
        SaveToCache(filePath);
    }

    void FbxScene::SaveToCache(const std::string_view filePath) const {
        const std::string cacheFile = CachedFilePath(filePath);
        TT::BinaryWriter writer(cacheFile);
        writer.u64(meshes.size());
        for(const auto& multiMesh : meshes) {
            writer.u32(multiMesh.name.length);
            writer.write(multiMesh.name.buffer, multiMesh.name.length);

            writer.u32(multiMesh.materialNameCount);
            for(uint32_t i = 0; i < multiMesh.materialNameCount; ++i) {
                writer.u32(multiMesh.materialNames[i].length);
                writer.write(multiMesh.materialNames[i].buffer, multiMesh.materialNames[i].length);
            }

            writer.u32(multiMesh.uvSetNameCount);
            for(uint32_t i = 0; i < multiMesh.uvSetNameCount; ++i) {
                writer.u32(multiMesh.uvSetNames[i].length);
                writer.write(multiMesh.uvSetNames[i].buffer, multiMesh.uvSetNames[i].length);
            }

            writer.u32(multiMesh.attributeCount);
            for(uint32_t i = 0; i < multiMesh.attributeCount; ++i) {
                writer.u8((uint8_t)multiMesh.attributeLayout[i].semantic);
                writer.u8((uint8_t)multiMesh.attributeLayout[i].numElements);
                writer.u32((uint32_t)multiMesh.attributeLayout[i].elementType);
            }

            writer.u32(multiMesh.primitiveType);

            writer.u8(multiMesh.indexElementSizeInBytes);

            writer.u32(multiMesh.meshCount);
            for(uint32_t i = 0; i < multiMesh.meshCount; ++i) {
                writer.u32(multiMesh.meshes[i].materialId);
                writer.u32(multiMesh.meshes[i].vertexDataSizeInBytes);
                writer.u32(multiMesh.meshes[i].indexDataSizeInBytes);
                writer.write(multiMesh.meshes[i].vertexDataBlob, multiMesh.meshes[i].vertexDataSizeInBytes);
                if(multiMesh.meshes[i].indexDataSizeInBytes)
                    writer.write(multiMesh.meshes[i].indexDataBlob, multiMesh.meshes[i].indexDataSizeInBytes);
            }

            writer.u32(multiMesh.jointCount);
            for(uint32_t i = 0; i < multiMesh.jointCount; ++i) {
                writer.u32(multiMesh.jointIndexData[i]);
            }
        }

        writer.u64(nodes.size());
        for (const auto& node : nodes) {
            writer.u32(node.name.length);
            writer.write(node.name.buffer, node.name.length);

            writer.f64(node.translateX);
            writer.f64(node.translateY);
            writer.f64(node.translateZ);
            writer.f64(node.rotateX);
            writer.f64(node.rotateY);
            writer.f64(node.rotateZ);
            writer.f64(node.scaleX);
            writer.f64(node.scaleY);
            writer.f64(node.scaleZ);
            writer.i32(node.rotateOrder);
            writer.i32(node.parentIndex);
            writer.i32(node.meshIndex);
        }
    }

    bool FbxScene::LoadFromCache(const std::string_view filePath) {
        const std::string cacheFile = CachedFilePath(filePath);

        // compare write times
        if(!FileExists(cacheFile) || FileLastWriteTime(filePath) > FileLastWriteTime(cacheFile))
            return false;

        TT::BinaryReader reader(cacheFile);
        meshes.resize(reader.u64());
        for(size_t i = 0; i < meshes.size(); ++i) {
            auto& mesh = meshes[i];
            readCString(reader, mesh.name);

            mesh.materialNameCount = reader.u32();
            mesh.materialNames = new String[mesh.materialNameCount];
            for(uint32_t j = 0; j < mesh.materialNameCount; ++j) {
                readCString(reader, mesh.materialNames[j]);
            }

            mesh.uvSetNameCount = reader.u32();
            mesh.uvSetNames = new String[mesh.uvSetNameCount];
            for(uint32_t j = 0; j < mesh.uvSetNameCount; ++j) {
                readCString(reader, mesh.uvSetNames[j]);
            }

            mesh.attributeCount = reader.u32();
            mesh.attributeLayout = new VertexAttribute[mesh.attributeCount];
            for(uint32_t j = 0; j < mesh.attributeCount; ++j) {
                mesh.attributeLayout[j].semantic = (Semantic)reader.u8();
                mesh.attributeLayout[j].numElements = (NumElements)reader.u8();
                mesh.attributeLayout[j].elementType = (ElementType)reader.u32();
            }

            mesh.primitiveType = reader.u32();

            mesh.indexElementSizeInBytes = reader.u8();

            mesh.meshCount = reader.u32();
            mesh.meshes = new MeshData[mesh.meshCount];
            for(uint32_t j = 0; j < mesh.meshCount; ++j) {
                auto& subMesh = mesh.meshes[j];
                subMesh.materialId = reader.u32();
                subMesh.vertexDataSizeInBytes = reader.u32();
                subMesh.indexDataSizeInBytes = reader.u32();
                subMesh.vertexDataBlob = new unsigned char[subMesh.vertexDataSizeInBytes];
                reader.readInto((char*)subMesh.vertexDataBlob, (size_t)subMesh.vertexDataSizeInBytes);
                subMesh.indexDataBlob = new unsigned char[subMesh.indexDataSizeInBytes];
                reader.readInto((char*)subMesh.indexDataBlob, (size_t)subMesh.indexDataSizeInBytes);
            }

            mesh.jointCount = reader.u32();
            mesh.jointIndexData = new uint32_t[mesh.jointCount];
            for(uint32_t j = 0; j < mesh.jointCount; ++j) {
                mesh.jointIndexData[j] = reader.u32();
            }
        }

        return true;
    }
}
