#pragma once
#include <cstdint>
#include <atomic>

namespace Rasterizer {
    enum class StatusCode : uint8_t {
        CORRUPT_INVALID_DATA = 0b0000,
        OK = 0b0001,
        OUT_OF_MEMORY = 0b0010,
        INVALID_ID = 0b0011,
        INVALID_OPCODE = 0b0100,
        SPI_ERROR = 0b1111,
    };

    enum class Operation: uint8_t {
        WIPE_ALL = 0b0000,
        CREATE_VERT = 0b0001,
        CREATE_TRI = 0b0010,
        CREATE_INST = 0b0011,
        UPDATE_INST = 0b0100,
    };

    // Minimal future/promise pair
    struct SpiFuture {
        std::atomic<bool> done{false};
        uint8_t returnCode = 0;
        uint8_t data = 0;
    };

    using FutureCallback = void (*)(SpiFuture*, void*);

    struct SpiPromise {
        SpiFuture* fut = nullptr;
        FutureCallback callback = nullptr;
        void* userCtx = nullptr;
    };

    struct Vertex {
        float x, y, z;
        uint8_t r, g, b;
    };

    struct Triangle {
        uint16_t index0, index1, index2;
    };

    struct Transform {
        float posX, posY, posZ;
        float rotX, rotY, rotZ;
        float scaleX, scaleY, scaleZ;
    };


    class BaseResponse {
    public:
        explicit BaseResponse(StatusCode status) : status_(status) {
        }

        virtual ~BaseResponse() = default;

        [[nodiscard]] bool isValid() const { return status_ == StatusCode::OK; }
        [[nodiscard]] bool isSuccess() const { return status_ == StatusCode::OK; }
        [[nodiscard]] StatusCode getStatus() const { return status_; }

        explicit operator bool() const { return isValid(); }

        [[nodiscard]] bool isOutOfMemory() const { return status_ == StatusCode::OUT_OF_MEMORY; }
        [[nodiscard]] bool isInvalidId() const { return status_ == StatusCode::INVALID_ID; }
        [[nodiscard]] bool isInvalidOpcode() const { return status_ == StatusCode::INVALID_OPCODE; }
        [[nodiscard]] bool isCorruptData() const { return status_ == StatusCode::CORRUPT_INVALID_DATA; }

    protected:
        StatusCode status_;
    };

    class WipeAllResponse : public BaseResponse {
    public:
        explicit WipeAllResponse(StatusCode status) : BaseResponse(status) {
        }
    };

    class UpdateInstResponse : public BaseResponse {
    public:
        explicit UpdateInstResponse(StatusCode status) : BaseResponse(status) {
        }
    };

    class CreateVertResponse : public BaseResponse {
    public:
        explicit CreateVertResponse(StatusCode status, uint8_t vert_id = 0)
            : BaseResponse(status), vert_id_(vert_id) {
        }

        [[nodiscard]] uint8_t getVertexId() const {
            return vert_id_;
        }

    private:
        uint8_t vert_id_;
    };

    class CreateTriResponse : public BaseResponse {
    public:
        explicit CreateTriResponse(StatusCode status, uint8_t tri_id = 0)
            : BaseResponse(status), tri_id_(tri_id) {
        }

        [[nodiscard]] uint8_t getTriangleId() const {
            return tri_id_;
        }

    private:
        uint8_t tri_id_;
    };

    class CreateInstResponse : public BaseResponse {
    public:
        explicit CreateInstResponse(StatusCode status, uint8_t inst_id = 0)
            : BaseResponse(status), inst_id_(inst_id) {
        }

        [[nodiscard]] uint8_t getInstanceId() const {
            return inst_id_;
        }

    private:
        uint8_t inst_id_;
    };

    /**
     * Interface for rasterizer.
     */
    class IRasterizer {
    public:
        // Inteface for rasterizer
        virtual ~IRasterizer() = default;

        virtual void clear(uint32_t argb) = 0;

        virtual void rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t argb) = 0;

        virtual void end_frame() = 0;

        /**
         * Wipe all vertex, triangle and instance data from the rasterizer.
         * @return response
         */
        virtual WipeAllResponse wipeAll() = 0;

        /**
         * Creates a new vertex buffer.
         * @param vertices array of vertices
         * @param count number of vertices in the array
         * @return status, ID assigned to the created vertex buffer
         */
        virtual CreateVertResponse createVertex(const Vertex* vertices, uint16_t count) = 0;

        /**
         * Create a new triangle buffer.
         * @param triangles array of triangles
         * @param count number of triangles in the array
         * @return status, ID assigned to the created triangle buffer
         */
        virtual CreateTriResponse createTriangle(const Triangle* triangles, uint16_t count) = 0;

        /**
         * Create model instance from vertex and triangle buffer.
         * @param vertexId ID of vertex buffer to use
         * @param triangleId ID of triangle buffer to use
         * @param transform initial transform of the instance
         * @return status, instance ID assigned to the created instance
         */
        virtual CreateInstResponse createInstance(uint8_t vertexId, uint8_t triangleId, const Transform& transform) = 0;

        /**
         * Update transform of an existing instance.
         * @param instanceId ID of the instance to update
         * @param transform new transform
         * @return status
         */
        virtual UpdateInstResponse updateInstance(uint8_t vertID, uint8_t triID, uint8_t instanceId, const Transform& transform) = 0;

        virtual SpiFuture* wipeAllAsync(FutureCallback callback = nullptr, void* userCtx = nullptr) = 0;
        virtual SpiFuture* createVertexAsync(const Vertex* vertices, uint16_t count,
                            FutureCallback callback = nullptr, void* userCtx = nullptr) = 0;
        virtual SpiFuture* createTriangleAsync(const Triangle* triangles, uint16_t count,
                            FutureCallback callback = nullptr, void* userCtx = nullptr) = 0;
        virtual SpiFuture* createInstanceAsync(uint8_t vertexId, uint8_t triangleId, const Transform& transform,
                            FutureCallback callback = nullptr, void* userCtx = nullptr) = 0;
        virtual SpiFuture* updateInstanceAsync(uint8_t vertID, uint8_t triID, uint8_t instanceId, const Transform& transform,
                            FutureCallback callback = nullptr, void* userCtx = nullptr) = 0;

        /**
         * Camera transform update;
         * @param transform new transform
         * @return status
         */
        UpdateInstResponse updateCamera(uint8_t red, uint8_t blue, uint8_t green, const Transform& transform){
            return updateInstance(red & 0x0F, (green & 0x0F) << 4 | (blue & 0x0F), 0, transform); // assuming camera has instance ID 0
        }
    };
}
