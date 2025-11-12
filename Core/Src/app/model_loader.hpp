/*********************************************************************************************
 *
 * Model loader
 * ============
 * Loads small Wavefront OBJ models into Rasterizer-friendly buffers on both STM32 (FatFs)
 * and desktop builds. The entire file is read into memory before parsing, so the models
 * must fit within the platform’s RAM limits.
 *
 * Supported OBJ features
 * ----------------------
 * - Vertex positions (`v`) with optional per-vertex colors (either 0–255 or 0.0–1.0 ranges).
 * - Faces (`f`) referencing any number of vertices; polygons are fan-triangulated.
 * - Positive and negative vertex indices (per the OBJ spec).
 *
 * Currently unsupported
 * ---------------------
 * - Texture coordinates (`vt`), normals (`vn`), materials, groups, smoothing,
 * and other directives are ignored during parsing.
 * - Non-manifold checks, degenerate triangle culling, or automatic normal generation.
 *
 * Limits
 * ------
 * - Up to 4096 vertices and 4096 triangles per model (enforced during parsing).
 *
 * Error handling
 * --------------
 * Parsing and I/O failures return a `ModelLoadResult` value and emit diagnostic messages.
 *
 * Usage
 * -----
 * - Call `load_model(path, outModel)`.
 * - Inspect the returned `ModelLoadResult`; on success, `outModel` contains vertices
 * and triangles.
 *
 *********************************************************************************************/

#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "irasterizer.hpp"

namespace mcu_game::assets {

struct ModelData {
    std::vector<Rasterizer::Vertex> vertices;
    std::vector<Rasterizer::Triangle> triangles;
};

enum class ModelLoadResult : uint8_t {
    Ok = 0,
    FileOpenFailed,
    ParseError,
    VertexCountInvalid,
    TriangleCountInvalid,
    DataReadFailed,
};

[[nodiscard]] ModelLoadResult load_model(const char* path, ModelData& outModel);
[[nodiscard]] const char* to_string(ModelLoadResult result);

}  // namespace mcu_game::assets
