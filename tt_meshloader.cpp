#include "tt_meshloader.h"
#include "../tt_cpplib/tt_files.h"

namespace {
    std::string cachedFilePath(const std::string_view path) {
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
    FbxExtractor::~FbxExtractor() {
        freeMeshes(_meshes.data(), (uint32_t)_meshes.size());
        freeNodes(_nodes.data(), (uint32_t)_nodes.size());
    }

    FbxExtractor::FbxExtractor(const std::string_view filePath, int up, int front, int flip, Units unit) {
        if (loadFromCache(filePath)) 
            return;
        FbxImportContext* context = importFbx(filePath.data(), up, front, flip, unit);
        _meshes = TT_FBX::extractMeshes(context);
        _nodes = TT_FBX::extractNodes(context);
        freeFbx(context);
        saveToCache(filePath);
    }

    void FbxExtractor::saveToCache(const std::string_view filePath) const {
        const std::string cacheFile = cachedFilePath(filePath);
        TT::BinaryWriter writer(cacheFile);
        writer.u64(_meshes.size());
        for(const auto& multiMesh : _meshes) {
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

        writer.u64(_nodes.size());
        for (const auto& node : _nodes) {
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

    bool FbxExtractor::loadFromCache(const std::string_view filePath) {
        const std::string cacheFile = cachedFilePath(filePath);

        // compare write times
        if(!TT::fileExists(cacheFile) || TT::fileLastWriteTime(filePath) > TT::fileLastWriteTime(cacheFile))
            return false;

        TT::BinaryReader reader(cacheFile);
        _meshes.resize(reader.u64());
        for(size_t i = 0; i < _meshes.size(); ++i) {
            auto& mesh = _meshes[i];
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
