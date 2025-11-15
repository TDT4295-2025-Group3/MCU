#include "host_rasterizer.hpp"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <stdexcept>
#include <vector>
#include <array>
#include <atomic>

// SDL only in this TU
#include <SDL3/SDL_render.h>
#include <SDL3/SDL_video.h>
#include <SDL3/SDL.h>

#define S3L_PIXEL_FUNCTION HostRasterizer_DrawPixelShim
#define S3L_Z_BUFFER 1              // Enable z-buffer so faces occlude correctly (prevents see-through artifacts)
#define S3L_MAX_PIXELS (1024 * 768) // cap for dynamic res (matches max allowed framebuffer size)
#include "small3dlib.h"

// Utilities
static inline int32_t toS3L(float v)
{
    return static_cast<int32_t>(std::lround(v * S3L_F));
}

static inline int32_t angleToS3L(float radians)
{
    const float TWO_PI = 6.28318530717958647692f;
    return static_cast<int32_t>(std::lround((radians / TWO_PI) * S3L_F));
}

static inline uint32_t ARGB_to_ABGR(uint32_t argb)
{
    uint8_t a = (argb >> 24) & 0xFF;
    uint8_t r = (argb >> 16) & 0xFF;
    uint8_t g = (argb >> 8) & 0xFF;
    uint8_t b = (argb >> 0) & 0xFF;
    return (a << 24) | (b << 16) | (g << 8) | r;
}

static inline void HostRasterizer_DrawPixelShim(S3L_PixelInfo *p);

namespace
{
    Rasterizer::SpiFuture *makeImmediateFuture(Rasterizer::StatusCode status, uint8_t data,
                                               Rasterizer::FutureCallback callback, void *userCtx)
    {
        auto *fut = new Rasterizer::SpiFuture();
        fut->returnCode = static_cast<uint8_t>(status);
        fut->data = data;
        fut->done.store(true, std::memory_order_release);
        if (callback)
        {
            callback(fut, userCtx);
        }
        return fut;
    }
}

static bool processEvents()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type)
        {
        case SDL_EVENT_QUIT: // Window close button
            return false;
        case SDL_EVENT_KEY_DOWN: // Optional: ESC key
            if (event.key.key == SDLK_ESCAPE && event.key.down)
            {
                return false; // exit
            }
            break;
        default:
            break;
        }
    }
    return true;
}

struct HostRasterizer::Impl
{
    // SDL
    SDL_Window *window = nullptr;
    SDL_Renderer *renderer = nullptr;
    SDL_Texture *texture = nullptr;

    int fbW = 0, fbH = 0;
    std::vector<uint32_t> framebuffer;

    // Buffers
    struct Instance
    {
        uint8_t vertexId, triangleId;
        Rasterizer::Transform transform;
    };

    std::array<std::vector<Rasterizer::Vertex>, 256> vbuf{};
    std::array<std::vector<Rasterizer::Triangle>, 256> tbuf{};
    std::array<bool, 256> vused{}, tused{};

    // Converted for small3dlib
    std::array<std::vector<S3L_Unit>, 256> s3lVerts{}; // x,y,z per vertex
    std::array<std::vector<S3L_Index>, 256> s3lTris{}; // i0,i1,i2 per tri

    std::vector<Instance> instances;

    // Scene & models
    std::vector<S3L_Model3D> models;
    std::vector<S3L_Transform3D> transforms;
    std::vector<uint8_t> modelToVId;
    std::vector<uint8_t> modelToIId;

    S3L_Scene scene{};

    // singleton for pixel callback
    static Impl *g_current;

    // methods
    explicit Impl(int w, int h) : fbW(w), fbH(h)
    {
    }

    ~Impl()
    {
        if (texture)
            SDL_DestroyTexture(texture);
        if (renderer)
            SDL_DestroyRenderer(renderer);
        if (window)
            SDL_DestroyWindow(window);
        g_current = nullptr;
    }

    static uint8_t findFree(const std::array<bool, 256> &used)
    {
        for (int i = 0; i < 256; ++i)
            if (!used[i])
                return static_cast<uint8_t>(i);
        return 0xFF;
    }

    void set_draw_color(uint32_t argb)
    {
        Uint8 a = (argb >> 24) & 0xFF;
        (void)a;
        Uint8 r = (argb >> 16) & 0xFF;
        Uint8 g = (argb >> 8) & 0xFF;
        Uint8 b = (argb >> 0) & 0xFF;
        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
    }

    static void toS3LTransform(const Rasterizer::Transform &t, S3L_Transform3D &out)
    {
        out.translation.x = toS3L(t.position.x);
        out.translation.y = toS3L(t.position.y);
        out.translation.z = toS3L(t.position.z);
        out.rotation.x = angleToS3L(t.rotation.x);
        out.rotation.y = angleToS3L(t.rotation.y);
        out.rotation.z = angleToS3L(t.rotation.z);
        out.scale.x = toS3L(t.scale.x);
        out.scale.y = toS3L(t.scale.y);
        out.scale.z = toS3L(t.scale.z);
        out.scale.w = 0;
    }

    // Pixel callback
    void drawPixel(S3L_PixelInfo *p)
    {
        if (p->x < 0 || p->y < 0 || p->x >= fbW || p->y >= fbH)
            return;
        size_t mIdx = p->modelIndex;
        if (mIdx >= modelToVId.size())
            return;

        uint8_t vbufId = modelToVId[mIdx];
        uint8_t ibufId = modelToIId[mIdx];

        const auto &tris = s3lTris[ibufId];
        size_t tbase = static_cast<size_t>(p->triangleIndex) * 3;
        if (tbase + 2 >= tris.size())
            return;

        S3L_Index i0 = tris[tbase + 0];
        S3L_Index i1 = tris[tbase + 1];
        S3L_Index i2 = tris[tbase + 2];

        const auto &verts = vbuf[vbufId];
        if (i0 >= verts.size() || i1 >= verts.size() || i2 >= verts.size())
            return;

        const auto &v0 = verts[i0];
        const auto &v1 = verts[i1];
        const auto &v2 = verts[i2];

        auto bary = p->barycentric;
        auto interp = [&](uint8_t c0, uint8_t c1, uint8_t c2) -> uint8_t
        {
            int32_t v = (int32_t)c0 * bary[0] + (int32_t)c1 * bary[1] + (int32_t)c2 * bary[2];
            v /= S3L_F;
            // expand 0..15 -> 0..255 if your inputs are nibble colors
            v *= 17; // 15*17 = 255
            if (v < 0)
                v = 0;
            if (v > 255)
                v = 255;
            return (uint8_t)v;
        };
        uint8_t r = interp(v0.r, v1.r, v2.r);
        uint8_t g = interp(v0.g, v1.g, v2.g);
        uint8_t b = interp(v0.b, v1.b, v2.b);

        framebuffer[static_cast<size_t>(p->y) * fbW + p->x] =
            (0xFFu << 24) | (uint32_t(b) << 16) | (uint32_t(g) << 8) | uint32_t(r);
    }
};

HostRasterizer::Impl *HostRasterizer::Impl::g_current = nullptr;

// Draw pixel
static inline void HostRasterizer_DrawPixelShim(S3L_PixelInfo *p)
{
    if (HostRasterizer::Impl::g_current)
        HostRasterizer::Impl::g_current->drawPixel(p);
}

HostRasterizer::HostRasterizer(int width, int height)
    : impl(std::make_unique<Impl>(width, height))
{
    // Added for debugging screen size issues
    const size_t requestedPixels = static_cast<size_t>(width) * static_cast<size_t>(height);
    if (requestedPixels > S3L_MAX_PIXELS)
    {
        throw std::runtime_error(
            "Requested resolution exceeds S3L_MAX_PIXELS. Increase the limit in host_rasterizer.cpp to continue.");
    }
    Impl::g_current = impl.get();

    // SDL setup
    impl->window = SDL_CreateWindow("small3dlib + SDL", width, height, 0);
    if (!impl->window)
        throw std::runtime_error(SDL_GetError());
    SDL_SetWindowPosition(impl->window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

    impl->renderer = SDL_CreateRenderer(impl->window, nullptr);
    if (!impl->renderer)
        throw std::runtime_error(SDL_GetError());

    impl->texture = SDL_CreateTexture(impl->renderer, SDL_PIXELFORMAT_ABGR8888,
                                      SDL_TEXTUREACCESS_STREAMING, width, height);
    if (!impl->texture)
        throw std::runtime_error(SDL_GetError());

    impl->framebuffer.resize(static_cast<size_t>(width) * height, 0xFF000000u);

    // small3dlib dynamic res
    S3L_resolutionX = static_cast<uint16_t>(width);
    S3L_resolutionY = static_cast<uint16_t>(height);

    // Scene & camera
    S3L_sceneInit(nullptr, 0, &impl->scene);
    impl->scene.camera.focalLength = S3L_F;
    impl->scene.camera.transform.translation.z = -2 * S3L_F;

    SDL_ShowWindow(impl->window);
}

HostRasterizer::~HostRasterizer() = default;

void HostRasterizer::clear(uint32_t argb)
{
    std::fill(impl->framebuffer.begin(), impl->framebuffer.end(), ARGB_to_ABGR(argb));
    S3L_newFrame();
    impl->set_draw_color(argb);
    SDL_RenderClear(impl->renderer);
}

void HostRasterizer::rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t argb)
{
    const uint32_t abgr = ARGB_to_ABGR(argb);
    uint32_t X0 = std::min<uint32_t>(x, impl->fbW);
    uint32_t Y0 = std::min<uint32_t>(y, impl->fbH);
    uint32_t X1 = std::min<uint32_t>(x + w, impl->fbW);
    uint32_t Y1 = std::min<uint32_t>(y + h, impl->fbH);
    for (uint32_t yy = Y0; yy < Y1; ++yy)
    {
        uint32_t *row = impl->framebuffer.data() + size_t(yy) * impl->fbW;
        for (uint32_t xx = X0; xx < X1; ++xx)
            row[xx] = abgr;
    }
}

void HostRasterizer::end_frame()
{

    // Process events
    if (!processEvents())
    {
        // Clean up & exit
        SDL_DestroyTexture(impl->texture);
        SDL_DestroyRenderer(impl->renderer);
        SDL_DestroyWindow(impl->window);
        SDL_Quit();
        std::exit(0);
    }

    if (impl->scene.modelCount > 0)
    {
        // ensure transforms are current
        for (size_t i = 0; i < impl->models.size(); ++i)
            impl->models[i].transform = impl->transforms[i];
        S3L_drawScene(impl->scene);
    }

    void *pixels = nullptr;
    int pitch = 0;
    if (!SDL_LockTexture(impl->texture, nullptr, &pixels, &pitch))
    {
        throw std::runtime_error(SDL_GetError());
    }
    const uint8_t *src = reinterpret_cast<const uint8_t *>(impl->framebuffer.data());
    // uint8_t *dst = <uint8_t *>(pixels);
    uint8_t *dst = reinterpret_cast<uint8_t *>(pixels);
    const int rowBytes = impl->fbW * 4;
    for (int y = 0; y < impl->fbH; ++y)
    {
        std::memcpy(dst + y * pitch, src + y * rowBytes, rowBytes);
    }
    SDL_UnlockTexture(impl->texture);

    SDL_RenderTexture(impl->renderer, impl->texture, nullptr, nullptr);
    SDL_RenderPresent(impl->renderer);
}

Rasterizer::WipeAllResponse HostRasterizer::wipeAll()
{
    for (auto &b : impl->vused)
        b = false;
    for (auto &b : impl->tused)
        b = false;
    for (auto &v : impl->vbuf)
        v.clear();
    for (auto &v : impl->tbuf)
        v.clear();
    for (auto &v : impl->s3lVerts)
        v.clear();
    for (auto &v : impl->s3lTris)
        v.clear();

    impl->instances.clear();
    impl->models.clear();
    impl->transforms.clear();
    impl->modelToVId.clear();
    impl->modelToIId.clear();

    impl->scene.models = nullptr;
    impl->scene.modelCount = 0;

    return Rasterizer::WipeAllResponse(Rasterizer::StatusCode::OK);
}

Rasterizer::CreateVertResponse HostRasterizer::createVertex(const Rasterizer::Vertex *vertices, uint16_t count)
{
    if (!vertices || count == 0 || count > 4096)
        return Rasterizer::CreateVertResponse(Rasterizer::StatusCode::CORRUPT_INVALID_DATA);

    uint8_t id = Impl::findFree(impl->vused);
    if (id == 0xFF)
        return Rasterizer::CreateVertResponse(Rasterizer::StatusCode::OUT_OF_MEMORY);

    auto &dst = impl->vbuf[id];
    dst.resize(count);
    for (uint16_t i = 0; i < count; ++i)
        dst[i] = vertices[i];

    auto &fx = impl->s3lVerts[id];
    fx.resize(size_t(count) * 3);
    for (uint16_t i = 0; i < count; ++i)
    {
        fx[i * 3 + 0] = toS3L(vertices[i].x);
        fx[i * 3 + 1] = toS3L(vertices[i].y);
        fx[i * 3 + 2] = toS3L(vertices[i].z);
    }

    impl->vused[id] = true;
    return Rasterizer::CreateVertResponse(Rasterizer::StatusCode::OK, id);
}

Rasterizer::CreateTriResponse HostRasterizer::createTriangle(const Rasterizer::Triangle *triangles, uint16_t count)
{
    if (!triangles || count == 0 || count > 4096)
        return Rasterizer::CreateTriResponse(Rasterizer::StatusCode::CORRUPT_INVALID_DATA);

    uint8_t id = Impl::findFree(impl->tused);
    if (id == 0xFF)
        return Rasterizer::CreateTriResponse(Rasterizer::StatusCode::OUT_OF_MEMORY);

    auto &dst = impl->tbuf[id];
    dst.resize(count);
    for (uint16_t i = 0; i < count; ++i)
        dst[i] = triangles[i];

    auto &ix = impl->s3lTris[id];
    ix.resize(size_t(count) * 3);
    for (uint16_t i = 0; i < count; ++i)
    {
        ix[i * 3 + 0] = triangles[i].index0;
        ix[i * 3 + 1] = triangles[i].index1;
        ix[i * 3 + 2] = triangles[i].index2;
    }

    impl->tused[id] = true;
    return Rasterizer::CreateTriResponse(Rasterizer::StatusCode::OK, id);
}

Rasterizer::CreateInstResponse HostRasterizer::createInstance(uint8_t vertexId, uint8_t triangleId,
                                                              const Rasterizer::Transform &transform)
{
    if (vertexId == 0xFF || triangleId == 0xFF ||
        !impl->vused[vertexId] || !impl->tused[triangleId])
    {
        return Rasterizer::CreateInstResponse(Rasterizer::StatusCode::INVALID_ID);
    }

    auto &vx = impl->s3lVerts[vertexId];
    auto &ix = impl->s3lTris[triangleId];
    if (vx.empty() || ix.empty())
        return Rasterizer::CreateInstResponse(Rasterizer::StatusCode::CORRUPT_INVALID_DATA);

    S3L_Model3D model{};
    S3L_model3DInit(vx.data(),
                    static_cast<S3L_Index>(vx.size() / 3),
                    ix.data(),
                    static_cast<S3L_Index>(ix.size() / 3),
                    &model);

    model.config.backfaceCulling = 0;

    S3L_Transform3D t{};
    Impl::toS3LTransform(transform, t);
    model.transform = t;

    uint8_t instId = static_cast<uint8_t>(impl->instances.size());
    impl->instances.push_back(Impl::Instance{vertexId, triangleId, transform});
    impl->transforms.push_back(t);
    impl->models.push_back(model);
    impl->modelToVId.push_back(vertexId);
    impl->modelToIId.push_back(triangleId);

    impl->scene.models = impl->models.data();
    impl->scene.modelCount = static_cast<S3L_Index>(impl->models.size());

    return Rasterizer::CreateInstResponse(Rasterizer::StatusCode::OK, instId + 1);
}

Rasterizer::UpdateInstResponse HostRasterizer::updateInstance(uint8_t vertexId, uint8_t triangleId,
                                                              uint8_t instanceId,
                                                              const Rasterizer::Transform &transform)
{
    if (instanceId == 0)
    {
        // Camera updates use instance ID 0 in the shared interface.
        S3L_Transform3D camT{};
        Impl::toS3LTransform(transform, camT);
        impl->scene.camera.transform = camT;
        return Rasterizer::UpdateInstResponse(Rasterizer::StatusCode::OK);
    }

    const uint8_t internalId = static_cast<uint8_t>(instanceId - 1); // adjust for camera slot
    if (internalId >= impl->instances.size())
    {
        return Rasterizer::UpdateInstResponse(Rasterizer::StatusCode::INVALID_ID);
    }

    auto &inst = impl->instances[internalId];

    if ((vertexId != 0xFF && inst.vertexId != vertexId) ||
        (triangleId != 0xFF && inst.triangleId != triangleId))
    {
        return Rasterizer::UpdateInstResponse(Rasterizer::StatusCode::INVALID_ID);
    }

    inst.transform = transform;
    Impl::toS3LTransform(transform, impl->transforms[internalId]);
    impl->models[internalId].transform = impl->transforms[internalId];
    return Rasterizer::UpdateInstResponse(Rasterizer::StatusCode::OK);
}

Rasterizer::SpiFuture *HostRasterizer::wipeAllAsync(Rasterizer::FutureCallback callback, void *userCtx)
{
    auto resp = wipeAll();
    return makeImmediateFuture(resp.getStatus(), 0, callback, userCtx);
}

Rasterizer::SpiFuture *HostRasterizer::createVertexAsync(const Rasterizer::Vertex *vertices, uint16_t count,
                                                         Rasterizer::FutureCallback callback, void *userCtx)
{
    auto resp = createVertex(vertices, count);
    return makeImmediateFuture(resp.getStatus(), resp.getVertexId(), callback, userCtx);
}

Rasterizer::SpiFuture *HostRasterizer::createTriangleAsync(const Rasterizer::Triangle *triangles, uint16_t count,
                                                           Rasterizer::FutureCallback callback, void *userCtx)
{
    auto resp = createTriangle(triangles, count);
    return makeImmediateFuture(resp.getStatus(), resp.getTriangleId(), callback, userCtx);
}

Rasterizer::SpiFuture *HostRasterizer::createInstanceAsync(uint8_t vertexId, uint8_t triangleId,
                                                           const Rasterizer::Transform &transform,
                                                           Rasterizer::FutureCallback callback, void *userCtx)
{
    auto resp = createInstance(vertexId, triangleId, transform);
    return makeImmediateFuture(resp.getStatus(), resp.getInstanceId(), callback, userCtx);
}

Rasterizer::SpiFuture *HostRasterizer::updateInstanceAsync(uint8_t vertexId, uint8_t triangleId, uint8_t instanceId,
                                                           const Rasterizer::Transform &transform,
                                                           Rasterizer::FutureCallback callback, void *userCtx)
{
    auto resp = updateInstance(vertexId, triangleId, instanceId, transform);
    return makeImmediateFuture(resp.getStatus(), 0, callback, userCtx);
}