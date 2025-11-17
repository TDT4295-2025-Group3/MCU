#pragma once

#include "baked_models.hpp"
#include <cstdio>
#include <string>
#include <vector>

#include "platform/imodelloader.hpp"

bool loadModelGeometry(Rasterizer::IRasterizer &gfx,
                       mcu_game::assets::IModelLoader &loader,
                       const char *relativePath,
                       uint32_t &vertexId,
                       uint32_t &triangleId,
                       size_t *outVertexCount,
                       size_t *outTriangleCount);

bool createBuffersWithFallback(Rasterizer::IRasterizer &gfx,
                               mcu_game::assets::IModelLoader &loader,
                               mcu_game::assets::baked::MeshId bakedId,
                               uint32_t &vertexId,
                               uint32_t &triangleId);
