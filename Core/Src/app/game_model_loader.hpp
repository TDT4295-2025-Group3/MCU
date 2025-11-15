#pragma once

#include "baked_models.hpp"
#include "model_loader.hpp"
#include <cstdio>
#include <string>

bool loadModelGeometry(Rasterizer::IRasterizer &gfx,
                       const char *relativePath,
                       uint32_t &vertexId,
                       uint32_t &triangleId,
                       size_t *outVertexCount,
                       size_t *outTriangleCount)
{
    vertexId = 0xFF;
    triangleId = 0xFF;

    if (!relativePath)
    {
        return false;
    }

    std::string fullPath = "models/";
    fullPath.append(relativePath);

    mcu_game::assets::ModelData modelData;
    const auto result = mcu_game::assets::load_model(fullPath.c_str(), modelData);
    if (result != mcu_game::assets::ModelLoadResult::Ok)
    {
        std::printf("[Model] load_model failed for %s: %s\n", fullPath.c_str(), mcu_game::assets::to_string(result));
        // SevenSeg::displayNumber(19);
        // HAL_Delay(10000U);
        return false;
    }

    const size_t vertexCount = modelData.vertices.size();
    const size_t triangleCount = modelData.triangles.size();
    // Removed obsolete cube instance creation
    // Streamlined player mesh initialization
    // const auto playerInstanceResp = gfx.createInstance(static_cast<uint8_t>(playerVertexId),
    //                                                    static_cast<uint8_t>(playerTriangleId),
    //                                                    {0.0f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f});
    const auto vertResp = gfx.createVertex(modelData.vertices.data(), static_cast<uint16_t>(vertexCount));
    if (!vertResp.isSuccess())
    {
        return false;
    }

    const auto triResp = gfx.createTriangle(modelData.triangles.data(), static_cast<uint16_t>(triangleCount));
    if (!triResp.isSuccess())
    {
        return false;
    }

    vertexId = vertResp.getVertexId();
    triangleId = triResp.getTriangleId();

    if (outVertexCount)
    {
        *outVertexCount = vertexCount;
    }
    if (outTriangleCount)
    {
        *outTriangleCount = triangleCount;
    }

    return true;
}

bool createBuffersWithFallback(Rasterizer::IRasterizer &gfx, const char *objName,
                               mcu_game::assets::baked::MeshId bakedId,
                               uint32_t &vertexId,
                               uint32_t &triangleId,
                               bool logSuccessFromFile = true)
{
    vertexId = 0xFF;
    triangleId = 0xFF;

    // Try normal path: load from OBJ
    const bool geomLoaded = loadModelGeometry(gfx,
                                              objName,
                                              vertexId,
                                              triangleId,
                                              nullptr,
                                              nullptr);
    if (geomLoaded)
    {
        return true;
    }

    // Fallback: baked mesh
    std::printf("[Model] Falling back to baked geometry for %s\n", objName);

    if (!mcu_game::assets::baked::createBuffers(bakedId,
                                                gfx,
                                                vertexId,
                                                triangleId))
    {
        std::printf("[Model] Failed to create baked geometry for %s\n", objName);
        vertexId = 0xFF;
        triangleId = 0xFF;
        return false;
    }

    return true;
}
