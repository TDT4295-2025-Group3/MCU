#include "game_model_loader.hpp"

bool loadModelGeometry(Rasterizer::IRasterizer &gfx,
                       const std::string &relativePath,
                       uint32_t &vertexId,
                       uint32_t &triangleId,
                       size_t *outVertexCount,
                       size_t *outTriangleCount)
{
    vertexId = 0xFF;
    triangleId = 0xFF;

    if (relativePath.empty())
        return false;

    std::string fullPath = "models/";
    fullPath.append(relativePath);

    mcu_game::assets::ModelData modelData;
    const auto result = mcu_game::assets::load_model(fullPath.c_str(), modelData);
    if (result != mcu_game::assets::ModelLoadResult::Ok)
        return false;

    const size_t vertexCount = modelData.vertices.size();
    const size_t triangleCount = modelData.triangles.size();
    const auto vertResp = gfx.createVertex(modelData.vertices.data(), static_cast<uint16_t>(vertexCount));
    if (!vertResp.isSuccess())
        return false;

    const auto triResp = gfx.createTriangle(modelData.triangles.data(), static_cast<uint16_t>(triangleCount));
    if (!triResp.isSuccess())
        return false;

    vertexId = vertResp.getVertexId();
    triangleId = triResp.getTriangleId();

    if (outVertexCount)
        *outVertexCount = vertexCount;

    if (outTriangleCount)
        *outTriangleCount = triangleCount;

    return true;
}
struct LoadedModel
{
    mcu_game::assets::baked::MeshId bakedId;
    uint32_t vertexId = 0xFF;
    uint32_t triangleId = 0xFF;
};

std::vector<LoadedModel> loadedModels;
bool createBuffersWithFallback(Rasterizer::IRasterizer &gfx,
                               mcu_game::assets::baked::MeshId bakedId,
                               uint32_t &vertexId,
                               uint32_t &triangleId)
{
    for (auto &loadedModel : loadedModels)
    {
        if (loadedModel.bakedId == bakedId)
        {
            vertexId = loadedModel.vertexId;
            triangleId = loadedModel.triangleId;
            return true;
        }
    }

    vertexId = 0xFF;
    triangleId = 0xFF;

    std::string objName = mcu_game::assets::baked::getMeshFileName(bakedId);

    // Try normal path: load from OBJ
    const bool geomLoaded = loadModelGeometry(gfx,
                                              objName,
                                              vertexId,
                                              triangleId,
                                              nullptr,
                                              nullptr);
    if (geomLoaded)
        return true;

    // Fallback: baked mesh
    std::printf("[Model] Falling back to baked geometry for %s\n", objName.c_str());

    if (!mcu_game::assets::baked::createBuffers(bakedId,
                                                gfx,
                                                vertexId,
                                                triangleId))
    {
        std::printf("[Model] Failed to create baked geometry for %s\n", objName.c_str());
        vertexId = 0xFF;
        triangleId = 0xFF;
        return false;
    }

    LoadedModel newLoadedModel;
    newLoadedModel.bakedId = bakedId;
    newLoadedModel.vertexId = vertexId;
    newLoadedModel.triangleId = triangleId;
    loadedModels.push_back(newLoadedModel);

    return true;
}
