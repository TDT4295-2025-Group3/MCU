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
#include <cstdint>
#include <string>
#include <vector>

namespace Rasterizer {
    struct Triangle;
    struct Vertex;
}

namespace mcu_game::assets {
    /**
     * Model data structure holding vertices and triangles.
     */
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
        SdCardUninitialized,
    };

    /**
     * Interface for loading .obj models.
     *  Implementations should provide platform-specific file I/O as needed.
     */
    class IModelLoader {
    public:
        virtual ~IModelLoader() = default;

        ModelLoadResult load_model(const char *path, ModelData &outModel);

        virtual ModelLoadResult read_entire_file(const char *path, std::string &out) = 0;

        [[nodiscard]] virtual const char *to_string(ModelLoadResult result) const;

    protected:
        static bool parse_float(std::string_view token, float &out);

        static bool parse_int(std::string_view token, int &out);

        static bool parse_face_index(std::string_view token, size_t vertexCount, uint16_t &out);

        static void tokenize(std::string_view line, std::vector<std::string_view> &tokens);

        static ModelLoadResult parse_obj(const char *path, const std::string &content, ModelData &outModel);

    private:
        constexpr static uint16_t kMaxSupportedVertices = 4096;
        constexpr static uint16_t kMaxSupportedTriangles = 4096;
        constexpr static uint8_t kDefaultColor = 15;
    };
}
