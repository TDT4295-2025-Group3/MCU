#include "unity.h"
#include "spi_stm.hpp"
#include "main.h"



void test_spi_init(void)
{
    // Just initialize SPI and check no errors
    Rasterizer::SpiAsyncRasterizer rasterizer;
}



void test_create_vert(void){
    Rasterizer::SpiAsyncRasterizer rasterizer;
    Rasterizer::Vertex testVerts[3];
    testVerts[0].x = 1.0f;
    testVerts[0].y = 0.0f;
    testVerts[0].z = 11.11f;
    testVerts[0].r = 15;
    testVerts[0].g = 0;
    testVerts[0].b = 0; 

    testVerts[1].x = 0.0f;
    testVerts[1].y = 1.0f;
    testVerts[1].z = 0.0f;
    testVerts[1].r = 0;
    testVerts[1].g = 15;
    testVerts[1].b = 0; 

    testVerts[2].x = 0.0f;
    testVerts[2].y = 0.0f;
    testVerts[2].z = 1.0f;
    testVerts[2].r = 0;
    testVerts[2].g = 0;
    testVerts[2].b = 15;

    auto fut = rasterizer.createVertexAsync(testVerts, 3);
    // Simulate doing other work
    HAL_Delay(100);
    TEST_ASSERT_EQUAL(true, fut->done.load());
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(Rasterizer::StatusCode::OK), fut->returnCode);
    TEST_ASSERT_EQUAL(0x00, fut->data); // ID 0 expected for first vertexBuff
    delete fut;


}

test_create_tri(void){
    Rasterizer::SpiAsyncRasterizer rasterizer;
    Rasterizer::Triangle testTri[1];
    testTri[0].index0 = 0;
    testTri[0].index1 = 1;
    testTri[0].index2 = 2;

    auto fut = rasterizer.createTriangleAsync(testTri, 1);
    // Simulate doing other work
    HAL_Delay(100);
    TEST_ASSERT_EQUAL(true, fut->done.load());
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(Rasterizer::StatusCode::OK), fut->returnCode);
    TEST_ASSERT_EQUAL(0x00, fut->data); // ID 0 expected for first triBuff
    delete fut;
}

test_create_instance(void){
    Rasterizer::SpiAsyncRasterizer rasterizer;
    Rasterizer::Transform testInst;
    testInst.posX = 1.0f;
    testInst.posY = 2.0f;
    testInst.posZ = 3.0f;
    testInst.rotX = 3.0f;
    testInst.rotY = 2.0f;
    testInst.rotZ = 1.0f;
    testInst.scaleX = 1.0f;
    testInst.scaleY = 1.0f;
    testInst.scaleZ = 1.0f;

    auto fut = rasterizer.createInstanceAsync(0,0,testInst);
    // Simulate doing other work
    HAL_Delay(100);
    TEST_ASSERT_EQUAL(true, fut->done.load());
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(Rasterizer::StatusCode::OK), fut->returnCode);
    TEST_ASSERT_EQUAL(0x01, fut->data); // ID 1 expected for first instID
    delete fut;
}

test_update_instance(void){
    Rasterizer::SpiAsyncRasterizer rasterizer;
    Rasterizer::Transform testInst;
    testInst.posX = 1.0f;
    testInst.posY = 2.0f;
    testInst.posZ = 3.0f;
    testInst.rotX = 3.0f;
    testInst.rotY = 2.0f;
    testInst.rotZ = 1.0f;
    testInst.scaleX = 1.0f;
    testInst.scaleY = 1.0f;
    testInst.scaleZ = 1.0f;

    auto fut = rasterizer.updateInstanceAsync(1, testInst);
    // Simulate doing other work
    HAL_Delay(100);
    TEST_ASSERT_EQUAL(true, fut->done.load());
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(Rasterizer::StatusCode::OK), fut->returnCode);
    delete fut;
}

test_many_vertBuffers(void){
    Rasterizer::SpiAsyncRasterizer rasterizer;
    Rasterizer::Vertex testVerts[1];
    testVerts[0].x = 1.0f;
    testVerts[0].y = 0.0f;
    testVerts[0].z = 11.11f;
    testVerts[0].r = 15;
    testVerts[0].g = 0;
    testVerts[0].b = 0;

    for (int i = 0; i < 10; i++) {
        auto fut = rasterizer.createVertexAsync(testVerts, 1);
        // Simulate doing other work
        HAL_Delay(100);
        TEST_ASSERT_EQUAL(true, fut->done.load());
        TEST_ASSERT_EQUAL(static_cast<uint8_t>(Rasterizer::StatusCode::OK), fut->returnCode);
        TEST_ASSERT_EQUAL(i+1, fut->data); // ID i+1 expected for ith vertexBuff due to previous test
        delete fut;
    }
}

test_wipe_all(void){
    Rasterizer::SpiAsyncRasterizer rasterizer;

    auto fut = rasterizer.wipeAllAsync();
    // Simulate doing other work
    HAL_Delay(100);
    TEST_ASSERT_EQUAL(true, fut->done.load());
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(Rasterizer::StatusCode::OK), fut->returnCode);
    delete fut;
}

test_full_model_load(void){
    Rasterizer::SpiAsyncRasterizer rasterizer;

    // Create vertex buffer
    Rasterizer::Vertex testVerts[3];
    testVerts[0].x = 1.0f;
    testVerts[0].y = 0.0f;
    testVerts[0].z = 11.11f;
    testVerts[0].r = 15;
    testVerts[0].g = 0;
    testVerts[0].b = 0; 

    testVerts[1].x = 0.0f;
    testVerts[1].y = 1.0f;
    testVerts[1].z = 0.0f;
    testVerts[1].r = 0;
    testVerts[1].g = 15;
    testVerts[1].b = 0; 

    testVerts[2].x = 0.0f;
    testVerts[2].y = 0.0f;
    testVerts[2].z = 1.0f;
    testVerts[2].r = 0;
    testVerts[2].g = 0;
    testVerts[2].b = 15;

    auto futVert = rasterizer.createVertexAsync(testVerts, 3);
    HAL_Delay(100);
    TEST_ASSERT_EQUAL(true, futVert->done.load());
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(Rasterizer::StatusCode::OK), futVert->returnCode);
    uint8_t vertBuffID = futVert->data;
    delete futVert;

    // Create triangle buffer
    Rasterizer::Triangle testTri[1];
    testTri[0].index0 = 0;
    testTri[0].index1 = 1;
    testTri[0].index2 = 2;

    auto futTri = rasterizer.createTriangleAsync(testTri, 1);
    HAL_Delay(100);
    TEST_ASSERT_EQUAL(true, futTri->done.load());
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(Rasterizer::StatusCode::OK), futTri->returnCode);
    uint8_t triBuffID = futTri->data;
    delete futTri;

    // Create instance
    Rasterizer::Transform testInst;
    testInst.posX = 1.0f;
    testInst.posY = 2.0f;
    testInst.posZ = 3.0f;
    testInst.rotX = 3.0f;
    testInst.rotY = 2.0f;
    testInst.rotZ = 1.0f;
    testInst.scaleX = 1.0f;
    testInst.scaleY = 1.0f;
    testInst.scaleZ = 1.0f;

    auto futInst = rasterizer.createInstanceAsync(vertBuffID, triBuffID, testInst);
    HAL_Delay(100);
    TEST_ASSERT_EQUAL(true, futInst->done.load());
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(Rasterizer::StatusCode::OK), futInst->returnCode);
    uint8_t instID = futInst->data;
    delete futInst;



extern "C" void run_spi_tests(void)
{
    RUN_TEST(test_spi_init);
    RUN_TEST(test_create_vert);
    RUN_TEST(test_create_tri);
    RUN_TEST(test_create_instance);
    RUN_TEST(test_update_instance);
    RUN_TEST(test_many_vertBuffers);
    RUN_TEST(test_wipe_all);
    RUN_TEST(test_full_model_load);
}