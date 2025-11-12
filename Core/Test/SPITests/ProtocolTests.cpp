#include "unity.h"
#include "spi_stm.hpp"
#include "main.h"
#include "seven_seg_display.hpp"

#define INTER_TEST_DELAY_MS 100



void test_create_vert(void){
    Rasterizer::SpiAsyncRasterizer rasterizer;
    Rasterizer::Vertex testVerts[3];
    testVerts[0].x = 1.0f;
    testVerts[0].y = 0.0f;
    testVerts[0].z = 11.11f;
    testVerts[0].r = 1;
    testVerts[0].g = 0;
    testVerts[0].b = 0; 

    testVerts[1].x = 0.0f;
    testVerts[1].y = 1.0f;
    testVerts[1].z = 0.0f;
    testVerts[1].r = 2;
    testVerts[1].g = 15;
    testVerts[1].b = 0; 

    testVerts[2].x = 0.0f;
    testVerts[2].y = 0.0f;
    testVerts[2].z = 1.0f;
    testVerts[2].r = 3;
    testVerts[2].g = 0;
    testVerts[2].b = 15;

    auto fut = rasterizer.createVertexAsync(testVerts, 3);
    // Simulate doing other work
    HAL_Delay(INTER_TEST_DELAY_MS);
    TEST_ASSERT_EQUAL(true, fut->done.load());
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(Rasterizer::StatusCode::OK), fut->returnCode);
    TEST_ASSERT_EQUAL(0x00, fut->data); // ID 0 expected for first vertexBuff
    delete fut;


}

void test_even_vert(void){
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

    auto fut = rasterizer.createVertexAsync(testVerts, 3);
    // Simulate doing other work
    HAL_Delay(INTER_TEST_DELAY_MS);
    TEST_ASSERT_EQUAL(true, fut->done.load());
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(Rasterizer::StatusCode::OK), fut->returnCode);
    TEST_ASSERT_EQUAL(0x01, fut->data); // ID 1 expected for second vertexBuff
    delete fut;
}

void test_create_tri(void){
    Rasterizer::SpiAsyncRasterizer rasterizer;
    Rasterizer::Triangle testTri[1];
    testTri[0].index0 = 0;
    testTri[0].index1 = 1;
    testTri[0].index2 = 2;

    auto fut = rasterizer.createTriangleAsync(testTri, 1);
    // Simulate doing other work
    HAL_Delay(INTER_TEST_DELAY_MS);
    TEST_ASSERT_EQUAL(true, fut->done.load());
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(Rasterizer::StatusCode::OK), fut->returnCode);
    TEST_ASSERT_EQUAL(0x00, fut->data); // ID 0 expected for first triBuff
    delete fut;
}

void test_create_even_tri(void){
    Rasterizer::SpiAsyncRasterizer rasterizer;
    Rasterizer::Triangle testTri[2];
    testTri[0].index0 = 0;
    testTri[0].index1 = 1;
    testTri[0].index2 = 2;
    testTri[1].index0 = 2;
    testTri[1].index1 = 1;
    testTri[1].index2 = 0;

    auto fut = rasterizer.createTriangleAsync(testTri, 2);
    // Simulate doing other work
    HAL_Delay(INTER_TEST_DELAY_MS);
    TEST_ASSERT_EQUAL(true, fut->done.load());
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(Rasterizer::StatusCode::OK), fut->returnCode);
    TEST_ASSERT_EQUAL(0x01, fut->data); // ID 1 expected for second triBuff
    delete fut;
}

void test_create_instance(void){
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

    auto fut = rasterizer.createInstanceAsync(0,0,testInst); //hardcoded to use vertbuff and tribuff from prev. tests
    // Simulate doing other work
    HAL_Delay(INTER_TEST_DELAY_MS);
    TEST_ASSERT_EQUAL(true, fut->done.load());
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(Rasterizer::StatusCode::OK), fut->returnCode);
    TEST_ASSERT_EQUAL(0x01, fut->data); // ID 1 expected for first instID, 0 reserved for camera
    delete fut;
}

void test_update_instance(void){
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

    auto fut = rasterizer.updateInstanceAsync(0, 0, 1, testInst);
    // Simulate doing other work
    HAL_Delay(INTER_TEST_DELAY_MS);
    TEST_ASSERT_EQUAL(true, fut->done.load());
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(Rasterizer::StatusCode::OK), fut->returnCode);
    delete fut;
}

void test_update_camera(void){
    Rasterizer::SpiAsyncRasterizer rasterizer;
    Rasterizer::Transform testCam;
    testCam.posX = 5.0f;
    testCam.posY = 5.0f;
    testCam.posZ = 5.0f;
    testCam.rotX = 0.0f;
    testCam.rotY = 0.0f;
    testCam.rotZ = 0.0f;
    testCam.scaleX = 1.0f;
    testCam.scaleY = 1.0f;
    testCam.scaleZ = 1.0f;

    auto fut = rasterizer.updateInstanceAsync(0, 0, 0, testCam); //instanceID 0 reserved for camera
    // Simulate doing other work
    HAL_Delay(INTER_TEST_DELAY_MS);
    TEST_ASSERT_EQUAL(true, fut->done.load());
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(Rasterizer::StatusCode::OK), fut->returnCode);
    delete fut;
}

void test_many_vertBuffers(void){
    Rasterizer::SpiAsyncRasterizer rasterizer;
    Rasterizer::Vertex testVerts[1];
    testVerts[0].x = 1.0f;
    testVerts[0].y = 0.0f;
    testVerts[0].z = 11.11f;
    testVerts[0].r = 15;
    testVerts[0].g = 0;
    testVerts[0].b = 0;

    for (int i = 0; i < 11; i++) { //10 verts to test if fpga handles byte aligned buffers
        auto fut = rasterizer.createVertexAsync(testVerts, 1);
        // Simulate doing other work
        HAL_Delay(INTER_TEST_DELAY_MS);
        TEST_ASSERT_EQUAL(true, fut->done.load());
        TEST_ASSERT_EQUAL(static_cast<uint8_t>(Rasterizer::StatusCode::OK), fut->returnCode);
        TEST_ASSERT_EQUAL(i+2, fut->data); // ID i+2 expected for ith vertexBuff due to previous tests
        delete fut;
    }
}

void test_wipe_all(void){
    Rasterizer::SpiAsyncRasterizer rasterizer;

    auto fut = rasterizer.wipeAllAsync();
    // Simulate doing other work
    HAL_Delay(INTER_TEST_DELAY_MS);
    TEST_ASSERT_EQUAL(true, fut->done.load());
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(Rasterizer::StatusCode::OK), fut->returnCode);
    delete fut;
}

void test_full_model_load(void){
    Rasterizer::SpiAsyncRasterizer rasterizer;

    // Create vertex buffer
    Rasterizer::Vertex testVerts[3];
    testVerts[0].x = 1.0f;
    testVerts[0].y = 0.0f;
    testVerts[0].z = 11.11f;
    testVerts[0].r = 4;
    testVerts[0].g = 0;
    testVerts[0].b = 0; 

    testVerts[1].x = 0.0f;
    testVerts[1].y = 1.0f;
    testVerts[1].z = 0.0f;
    testVerts[1].r = 5;
    testVerts[1].g = 15;
    testVerts[1].b = 0; 

    testVerts[2].x = 0.0f;
    testVerts[2].y = 0.0f;
    testVerts[2].z = 1.0f;
    testVerts[2].r = 6;
    testVerts[2].g = 0;
    testVerts[2].b = 15;

    auto futVert = rasterizer.createVertexAsync(testVerts, 3);
    HAL_Delay(INTER_TEST_DELAY_MS);
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
    HAL_Delay(INTER_TEST_DELAY_MS);
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
    HAL_Delay(INTER_TEST_DELAY_MS);
    TEST_ASSERT_EQUAL(true, futInst->done.load());
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(Rasterizer::StatusCode::OK), futInst->returnCode);
    uint8_t instID = futInst->data;
    delete futInst;

    

    // update instance
    Rasterizer::Transform testInstUpd;
    testInstUpd.posX = 4.0f;
    testInstUpd.posY = 5.0f;
    testInstUpd.posZ = 6.0f;
    testInstUpd.rotX = 6.0f;
    testInstUpd.rotY = 5.0f;
    testInstUpd.rotZ = 4.0f;
    testInstUpd.scaleX = 1.0f;
    testInstUpd.scaleY = 1.0f;
    testInstUpd.scaleZ = 1.0f;
    auto futInstUpd = rasterizer.updateInstanceAsync(vertBuffID, triBuffID, instID, testInstUpd);
    HAL_Delay(INTER_TEST_DELAY_MS);
    TEST_ASSERT_EQUAL(static_cast<uint8_t>(Rasterizer::StatusCode::OK), futInstUpd->returnCode);
}


extern "C" void run_spi_tests(void)
{
    static bool displayInitialized = false;
    if (!displayInitialized)
    {
        SevenSeg::init();
        displayInitialized = true;
    }

    SevenSeg::displayNumber(1);
    RUN_TEST(test_wipe_all);
    SevenSeg::displayNumber(1234);
    RUN_TEST(test_create_vert);
    RUN_TEST(test_even_vert);
    RUN_TEST(test_create_tri);
    RUN_TEST(test_create_even_tri);
    RUN_TEST(test_create_instance);
    RUN_TEST(test_update_instance);
    RUN_TEST(test_many_vertBuffers);
    RUN_TEST(test_full_model_load);
    RUN_TEST(test_wipe_all);
    RUN_TEST(test_create_vert);
    RUN_TEST(test_create_tri);
    RUN_TEST(test_create_instance);
    RUN_TEST(test_update_instance);
    RUN_TEST(test_wipe_all);
}