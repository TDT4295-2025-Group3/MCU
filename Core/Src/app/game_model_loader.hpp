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
                       size_t *outTriangleCount);

bool createBuffersWithFallback(Rasterizer::IRasterizer &gfx, const char *objName,
                               mcu_game::assets::baked::MeshId bakedId,
                               uint32_t &vertexId,
                               uint32_t &triangleId);
