#pragma once
#include <cstdint>
#include <memory>
#include "irasterizer.hpp"

class HostRasterizer : public Rasterizer::IRasterizer
{
public:
    explicit HostRasterizer(int width, int height);

    ~HostRasterizer() override;

    // IRasterizer
    void clear() override;

    void rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t argb) override;

    void end_frame() override;

    Rasterizer::WipeAllResponse wipeAll() override;

    Rasterizer::CreateVertResponse createVertex(const Rasterizer::Vertex *vertices, uint16_t count) override;

    Rasterizer::CreateTriResponse createTriangle(const Rasterizer::Triangle *triangles, uint16_t count) override;

    Rasterizer::CreateInstResponse createInstance(uint8_t vertexId, uint8_t triangleId,
                                                  const Rasterizer::Transform &transform) override;

    Rasterizer::UpdateInstResponse updateInstance(uint8_t vertexId, uint8_t triangleId, uint8_t instanceId,
                                                  const Rasterizer::Transform &transform) override;

    Rasterizer::SpiFuture *wipeAllAsync(Rasterizer::FutureCallback callback = nullptr, void *userCtx = nullptr) override;
    Rasterizer::SpiFuture *createVertexAsync(const Rasterizer::Vertex *vertices, uint16_t count,
                                             Rasterizer::FutureCallback callback = nullptr, void *userCtx = nullptr) override;
    Rasterizer::SpiFuture *createTriangleAsync(const Rasterizer::Triangle *triangles, uint16_t count,
                                               Rasterizer::FutureCallback callback = nullptr, void *userCtx = nullptr) override;
    Rasterizer::SpiFuture *createInstanceAsync(uint8_t vertexId, uint8_t triangleId,
                                               const Rasterizer::Transform &transform,
                                               Rasterizer::FutureCallback callback = nullptr, void *userCtx = nullptr) override;
    Rasterizer::SpiFuture *updateInstanceAsync(uint8_t vertexId, uint8_t triangleId, uint8_t instanceId,
                                               const Rasterizer::Transform &transform,
                                               Rasterizer::FutureCallback callback = nullptr, void *userCtx = nullptr) override;

    Rasterizer::UpdateInstResponse updateCamera(uint8_t red, uint8_t blue, uint8_t green,
                                                const Rasterizer::Transform &transform) override;

    struct Impl;
    std::unique_ptr<Impl> impl;
};
