#pragma once
#include <stdint.h>
#include "irasterizer.hpp"
#include <atomic>

// Define the raw type for Q16.16
using Q16_16 = int32_t;

// floating point to Q16.16 conversion

inline Q16_16 floatToQ16_16(float value) {
    return static_cast<int32_t>(value * (1 << 16));
}

// Q16.16 to floating point conversion
inline float q16_16ToFloat(Q16_16 value) {
    return static_cast<float>(value) / (1 << 16);
}

using SpiFutureCallback = Rasterizer::FutureCallback;

namespace Rasterizer {

class SpiAsyncRasterizer {
public:
    SpiAsyncRasterizer() = default;
    Rasterizer::SpiFuture* wipeAllAsync(FutureCallback callback = nullptr, void* userCtx = nullptr);
    Rasterizer::SpiFuture* createVertexAsync(const Vertex* vertices, uint16_t count,
                                             FutureCallback callback = nullptr, void* userCtx = nullptr);
    Rasterizer::SpiFuture* createTriangleAsync(const Triangle* triangles, uint16_t count,
                                               FutureCallback callback = nullptr, void* userCtx = nullptr);
    Rasterizer::SpiFuture* createInstanceAsync(uint8_t vertexId, uint8_t triangleId, const Transform& transform,
                                               FutureCallback callback = nullptr, void* userCtx = nullptr);
    Rasterizer::SpiFuture* updateInstanceAsync(uint8_t vertID, uint8_t triID, uint8_t instanceId, const Transform& transform,
                                               FutureCallback callback = nullptr, void* userCtx = nullptr);
};

class SpiRasterizer : public IRasterizer {
public:
    SpiRasterizer() = default;

    void clear(uint32_t argb) override;
    void rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t argb) override;
    void end_frame() override;

    WipeAllResponse wipeAll() override;
    CreateVertResponse createVertex(const Vertex* vertices, uint16_t count) override;
    CreateTriResponse createTriangle(const Triangle* triangles, uint16_t count) override;
    CreateInstResponse createInstance(uint8_t vertexId, uint8_t triangleId, const Transform& transform) override;
    UpdateInstResponse updateInstance(uint8_t vertID, uint8_t triID, uint8_t instanceId, const Transform& transform) override;

    SpiFuture* wipeAllAsync(FutureCallback callback = nullptr, void* userCtx = nullptr) override;
    SpiFuture* createVertexAsync(const Vertex* vertices, uint16_t count,
                                 FutureCallback callback = nullptr, void* userCtx = nullptr) override;
    SpiFuture* createTriangleAsync(const Triangle* triangles, uint16_t count,
                                   FutureCallback callback = nullptr, void* userCtx = nullptr) override;
    SpiFuture* createInstanceAsync(uint8_t vertexId, uint8_t triangleId, const Transform& transform,
                                   FutureCallback callback = nullptr, void* userCtx = nullptr) override;
    SpiFuture* updateInstanceAsync(uint8_t vertID, uint8_t triID, uint8_t instanceId,  const Transform& transform,
                                   FutureCallback callback = nullptr, void* userCtx = nullptr) override;

private:
// Used as userCtx so async callbacks can store results like status and data.
// This happens after the SPI transaction is done.
    struct CallbackState {
        std::atomic<bool> finished{false};
        StatusCode status = StatusCode::SPI_ERROR;
        uint8_t data = 0;
    };

    static void basic_callback(SpiFuture* future, void* ctx);
    static void wait_for_completion(CallbackState& state);

    SpiAsyncRasterizer rasterizer_{};
};

} // namespace Rasterizer

#ifdef __cplusplus
extern "C" {
#endif

Rasterizer::SpiFuture* wipe_all_async(SpiFutureCallback callback, void* userCtx);
Rasterizer::SpiFuture* create_vertex_async(Rasterizer::Vertex *vertexBuffer, uint16_t vertCount,
                                          SpiFutureCallback callback, void* userCtx);
Rasterizer::SpiFuture* create_triangle_async(Rasterizer::Triangle *triangleBuffer, uint16_t triCount,
                                            SpiFutureCallback callback, void* userCtx);
Rasterizer::SpiFuture* create_instance_async(Rasterizer::Transform *instanceData, uint8_t vertbufferID,
                                            uint8_t tribufferID, SpiFutureCallback callback, void* userCtx);
Rasterizer::SpiFuture* update_instance_async(Rasterizer::Transform *instanceData, uint8_t vertID, uint8_t triID, uint8_t instID,
                                            SpiFutureCallback callback, void* userCtx);
#ifdef __cplusplus
}
#endif