#include "model_loader.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdio>
#include <limits>
#include <system_error>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#if defined(PLATFORM_STM32)
#include "ff.h"
#endif


namespace mcu_game::assets {
namespace {

constexpr uint16_t kMaxSupportedVertices = 4096;
constexpr uint16_t kMaxSupportedTriangles = 4096;
constexpr uint8_t kDefaultColor = 15;

bool parse_float(std::string_view token, float& out) {
    const char* begin = token.data();
    const char* end = begin + token.size();
    const auto result = std::from_chars(begin, end, out);
    return (result.ec == std::errc{}) && (result.ptr == end);
}

bool parse_int(std::string_view token, int& out) {
    const char* begin = token.data();
    const char* end = begin + token.size();
    const auto result = std::from_chars(begin, end, out);
    return (result.ec == std::errc{}) && (result.ptr == end);
}

bool parse_face_index(std::string_view token, size_t vertexCount, uint16_t& out) {
    if (token.empty()) {
        return false;
    }

    const size_t slashPos = token.find('/');
    std::string_view indexToken = (slashPos == std::string_view::npos) ? token : token.substr(0, slashPos);
    if (indexToken.empty()) {
        return false;
    }

    int value = 0;
    if (!parse_int(indexToken, value) || value == 0) {
        return false;
    }

    const int resolved = (value > 0) ? (value - 1) : static_cast<int>(vertexCount) + value;
    if (resolved < 0 || resolved >= static_cast<int>(vertexCount)) {
        return false;
    }

    out = static_cast<uint16_t>(resolved);
    return true;
}

void tokenize(std::string_view line, std::vector<std::string_view>& tokens) {
    tokens.clear();
    size_t pos = 0;
    while (pos < line.size()) {
        while (pos < line.size() && std::isspace(static_cast<unsigned char>(line[pos]))) {
            ++pos;
        }
        if (pos >= line.size()) {
            break;
        }
        if (line[pos] == '#') {
            break;
        }

        const size_t start = pos;
        while (pos < line.size() && !std::isspace(static_cast<unsigned char>(line[pos]))) {
            ++pos;
        }
        tokens.emplace_back(line.substr(start, pos - start));
    }
}

#if defined(PLATFORM_STM32)
ModelLoadResult read_entire_file(const char* path, std::string& out) {
    FIL file{};
    const FRESULT openRes = f_open(&file, path, FA_READ);
    if (openRes != FR_OK) {
        std::printf("[Model] f_open failed for %s: %d\n", path, openRes);
        return ModelLoadResult::FileOpenFailed;
    }

    const FSIZE_t rawSize = f_size(&file);
    out.clear();
    if (rawSize > 0) {
        if (rawSize > static_cast<FSIZE_t>(std::numeric_limits<UINT>::max())) {
            std::printf("[Model] %s is too large (%lu bytes)\n", path, static_cast<unsigned long>(rawSize));
            f_close(&file);
            return ModelLoadResult::DataReadFailed;
        }

        out.resize(static_cast<size_t>(rawSize));
        UINT bytesRead = 0;
        const FRESULT readRes = f_read(&file, out.data(), static_cast<UINT>(out.size()), &bytesRead);
        f_close(&file);
        if (readRes != FR_OK || bytesRead != out.size()) {
            std::printf("[Model] Failed reading %s (res=%d, read=%u)\n", path, readRes, bytesRead);
            out.clear();
            return ModelLoadResult::DataReadFailed;
        }
    } else {
        f_close(&file);
    }

    return ModelLoadResult::Ok;
}
#elif defined(PLATFORM_PC)
ModelLoadResult read_entire_file(const char* path, std::string& out) {
    std::FILE* file = std::fopen(path, "rb");
    if (file == nullptr) {
        std::printf("[Model] fopen failed for %s\n", path);
        return ModelLoadResult::FileOpenFailed;
    }

    if (std::fseek(file, 0, SEEK_END) != 0) {
        std::printf("[Model] fseek failed for %s\n", path);
        std::fclose(file);
        return ModelLoadResult::DataReadFailed;
    }

    const long size = std::ftell(file);
    if (size < 0) {
        std::printf("[Model] ftell failed for %s\n", path);
        std::fclose(file);
        return ModelLoadResult::DataReadFailed;
    }

    if (std::fseek(file, 0, SEEK_SET) != 0) {
        std::printf("[Model] rewind failed for %s\n", path);
        std::fclose(file);
        return ModelLoadResult::DataReadFailed;
    }

    out.resize(static_cast<size_t>(size));
    if (!out.empty()) {
        const size_t read = std::fread(out.data(), 1, out.size(), file);
        if (read != out.size()) {
            std::printf("[Model] fread failed for %s (read=%zu)\n", path, read);
            std::fclose(file);
            out.clear();
            return ModelLoadResult::DataReadFailed;
        }
    }

    std::fclose(file);
    return ModelLoadResult::Ok;
}
#endif

ModelLoadResult parse_obj(const char* path, const std::string& content, ModelData& outModel) {
    std::vector<Rasterizer::Vertex> vertices;
    std::vector<Rasterizer::Triangle> triangles;
    vertices.reserve(32);
    triangles.reserve(64);

    std::vector<std::string_view> tokens;
    tokens.reserve(16);

    size_t lineNumber = 0;
    size_t offset = 0;
    while (offset <= content.size()) {
        const size_t endPos = content.find('\n', offset);
        std::string_view line;
        if (endPos == std::string::npos) {
            line = std::string_view(content.data() + offset, content.size() - offset);
            offset = content.size() + 1;
        } else {
            line = std::string_view(content.data() + offset, endPos - offset);
            offset = endPos + 1;
        }

        if (!line.empty() && line.back() == '\r') {
            line = line.substr(0, line.size() - 1);
        }

        ++lineNumber;
        tokenize(line, tokens);
        if (tokens.empty()) {
            continue;
        }

        // Ignore everything that is not a vertex or face
        if (!(tokens[0] == "v" || tokens[0] == "f")) {
            continue;
        }

        if (tokens[0] == "v") {
            if (tokens.size() < 4) {
                std::printf("[Model] Parse error in %s (line %zu): vertex needs x y z\n", path, lineNumber);
                return ModelLoadResult::ParseError;
            }

            float x = 0.0f;
            float y = 0.0f;
            float z = 0.0f;
            if (!parse_float(tokens[1], x) || !parse_float(tokens[2], y) || !parse_float(tokens[3], z)) {
                std::printf("[Model] Parse error in %s (line %zu): invalid vertex coordinate\n", path, lineNumber);
                return ModelLoadResult::ParseError;
            }

            Rasterizer::Vertex vertex{};
            vertex.x = x;
            vertex.y = y;
            vertex.z = z;
            vertex.r = kDefaultColor;
            vertex.g = kDefaultColor;
            vertex.b = kDefaultColor;

            if (tokens.size() >= 7) {
                const size_t colorStart = tokens.size() - 3;
                float rf = 0.0f;
                float gf = 0.0f;
                float bf = 0.0f;
                if (!parse_float(tokens[colorStart], rf) || !parse_float(tokens[colorStart + 1], gf) || !parse_float(tokens[colorStart + 2], bf)) {
                    std::printf("[Model] Parse error in %s (line %zu): invalid vertex color\n", path, lineNumber);
                    return ModelLoadResult::ParseError;
                }

                const bool normalized = (rf >= 0.0f && rf <= 1.0f) &&
                                        (gf >= 0.0f && gf <= 1.0f) &&
                                        (bf >= 0.0f && bf <= 1.0f);

                const auto toNibble = [normalized](float value) {
                    float scaled = 0.0f;
                    if (normalized) {
                        scaled = value * 15.0f;
                    } else if (value > 15.0f) {
                        // Treat large values as 0-255 input and remap to nibble range
                        scaled = value * (15.0f / 255.0f);
                    } else {
                        scaled = value;
                    }

                    const float clamped = std::clamp(scaled, 0.0f, 15.0f);
                    return static_cast<uint8_t>(clamped + 0.5f);
                };

                vertex.r = toNibble(rf);
                vertex.g = toNibble(gf);
                vertex.b = toNibble(bf);
            }

            vertices.push_back(vertex);
            if (vertices.size() > kMaxSupportedVertices) {
                std::printf("[Model] Vertex count exceeds limit (%zu) in %s\n", vertices.size(), path);
                return ModelLoadResult::VertexCountInvalid;
            }
        } else if (tokens[0] == "f") {
            if (tokens.size() < 4) {
                std::printf("[Model] Parse error in %s (line %zu): face needs at least 3 vertices\n", path, lineNumber);
                return ModelLoadResult::ParseError;
            }

            std::vector<uint16_t> faceIndices;
            faceIndices.reserve(tokens.size() - 1);
            for (size_t i = 1; i < tokens.size(); ++i) {
                uint16_t index = 0;
                if (!parse_face_index(tokens[i], vertices.size(), index)) {
                    std::printf("[Model] Parse error in %s (line %zu): invalid face index\n", path, lineNumber);
                    return ModelLoadResult::ParseError;
                }
                faceIndices.push_back(index);
            }

            for (size_t i = 1; i + 1 < faceIndices.size(); ++i) {
                Rasterizer::Triangle tri{};
                tri.index0 = faceIndices[0];
                tri.index1 = faceIndices[i];
                tri.index2 = faceIndices[i + 1];
                triangles.push_back(tri);

                if (triangles.size() > kMaxSupportedTriangles) {
                    std::printf("[Model] Triangle count exceeds limit (%zu) in %s\n", triangles.size(), path);
                    return ModelLoadResult::TriangleCountInvalid;
                }
            }
        }
        // ignore other directives
    }

    if (vertices.empty()) {
        std::printf("[Model] No vertices parsed from %s\n", path);
        return ModelLoadResult::VertexCountInvalid;
    }

    if (triangles.empty()) {
        std::printf("[Model] No triangles parsed from %s\n", path);
        return ModelLoadResult::TriangleCountInvalid;
    }

    outModel.vertices = std::move(vertices);
    outModel.triangles = std::move(triangles);
    return ModelLoadResult::Ok;
}

}  // namespace

ModelLoadResult load_model(const char* path, ModelData& outModel) {
    if (path == nullptr) {
        return ModelLoadResult::FileOpenFailed;
    }

    std::string content;

#if defined(PLATFORM_STM32)
    const ModelLoadResult ioRes = read_entire_file(path, content);
    if (ioRes != ModelLoadResult::Ok) {
        return ioRes;
    }
#elif defined(PLATFORM_PC)
    const ModelLoadResult ioRes = read_entire_file(path, content);
    if (ioRes != ModelLoadResult::Ok) {
        return ioRes;
    }
#else
    (void)outModel;
    (void)path;
    return ModelLoadResult::FileOpenFailed;
#endif

    return parse_obj(path, content, outModel);
}

const char* to_string(ModelLoadResult result) {
    switch (result) {
        case ModelLoadResult::Ok:
            return "Ok";
        case ModelLoadResult::FileOpenFailed:
            return "FileOpenFailed";
        case ModelLoadResult::ParseError:
            return "ParseError";
        case ModelLoadResult::VertexCountInvalid:
            return "VertexCountInvalid";
        case ModelLoadResult::TriangleCountInvalid:
            return "TriangleCountInvalid";
        case ModelLoadResult::DataReadFailed:
            return "DataReadFailed";
        default:
            return "Unknown";
    }
}

}  // namespace mcu_game::assets
