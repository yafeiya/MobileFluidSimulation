//
// Created by zmy on 2024/10/16.
//
#include"renderer.h"
#include"renderer_types.h"
#include"../pk/glm/gtc/matrix_transform.hpp"

#include<iostream>
#include<exception>
#include<chrono>
#include<string>
#include<algorithm>


#include <game-activity/native_app_glue/android_native_app_glue.h>
#include <vulkan/vulkan.h>
#include <thread>
#include <atomic>
#include <chrono>
#include <android/log.h>


#undef APIENTRY
#define NOMINMAX
//#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

#define LOG_TAG "native-log"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, LOG_TAG, __VA_ARGS__)

#define PI 3.1415926f

struct VulkanEngine {
    struct android_app *app;
    Renderer *app_backend;
    bool canRender = false;
};

static void HandleCmd(struct android_app *app, int32_t cmd) {
    auto *engine = (VulkanEngine *)app->userData;
    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            if (engine->app->window != nullptr) {
                // engine->app_backend->reset(app->window, app->activity->assetManager);
                engine->app_backend->Init(app);
                engine->canRender = true;
            }
            break;
        case APP_CMD_TERM_WINDOW:
            // The window is being hidden or closed, clean it up.
            engine->canRender = false;
            break;
        case APP_CMD_DESTROY:
            // The window is being hidden or closed, clean it up.
            //LOGI("Destroying");
            engine->app_backend->Cleanup();
        default:
            break;
    }
}


void android_main(struct android_app* app){
    VulkanEngine engine{};
    Renderer renderer = Renderer(1080,2400,true);

    engine.app = app;
    engine.app_backend = &renderer;
    app->userData =  &engine;
    app->onAppCmd = HandleCmd;

    float radius = 0.008;
    float restDesity = 1000.0f;
    float diam = 2*radius;

    //定义渲染对象，包括空间视角变化矩阵、3d到2d的投影矩阵、近景远景裁切、俯视角度。
    UniformRenderingObject renderingobj{};
    renderingobj.model = glm::mat4(1.0f);
    renderingobj.view = glm::lookAt(glm::vec3(1.5,1.3,1.5),glm::vec3(0,0.3,0),glm::vec3(0,1,0));
    renderingobj.projection = glm::perspective(glm::radians(90.0f),1.0f,0.1f,10.0f);
    renderingobj.projection[1][1]*=-1;
    renderingobj.inv_projection = glm::inverse(renderingobj.projection);
    renderingobj.zNear = 0.1f;
    renderingobj.zFar = 10.0f;
    renderingobj.aspect = 1;
    renderingobj.fovy = glm::radians(90.0f);
    renderingobj.particleRadius = radius;
    renderer.SetRenderingObj(renderingobj);


    //定义模拟对象，包括两帧相隔的时间，模拟空间的基本物理信息。
    UniformSimulatingObject simulatingobj{};
    simulatingobj.dt = 1/240.0f;
    simulatingobj.restDensity = 1.0f/(diam*diam*diam);
    simulatingobj.sphRadius = 4*radius;

    simulatingobj.coffPoly6 = 315.0f/(64*PI*pow(simulatingobj.sphRadius,3));
    simulatingobj.coffGradSpiky = -45/(PI*pow(simulatingobj.sphRadius,4));
    simulatingobj.coffSpiky = 15/(PI*pow(simulatingobj.sphRadius,3));

    simulatingobj.scorrK = 0.0001;
    simulatingobj.scorrQ = 0.1;
    simulatingobj.scorrN = 4;
    renderer.SetSimulatingObj(simulatingobj);

    //定义流体对象，由粒子组成。
    UniformNSObject nsobj{};
    nsobj.sphRadius = 4*radius;
    renderer.SetNSObj(nsobj);

    UniformBoxInfoObject boxinfoobj{};
    boxinfoobj.clampX = glm::vec2{0,1.5};
    boxinfoobj.clampY = glm::vec2{0,1};
    boxinfoobj.clampZ = glm::vec2{0,1};
    boxinfoobj.clampX_still = glm::vec2{0,1.5};
    boxinfoobj.clampY_still = glm::vec2{0,1};
    boxinfoobj.clampZ_still = glm::vec2{0,1};
    renderer.SetBoxinfoObj(boxinfoobj);

    //定义粒子群，并指定它们的位置。
    std::vector<Particle> particles;
    for(float x=0.25;x<=0.5;x+=diam){
        for(float z=0.25;z<=0.5;z+=diam){
            for(float y=0.25;y<=0.5;y+=diam){
                Particle particle{};
                particle.Location = glm::vec3(x,y,z);

                particle.Mass = 1;
                particle.NumNgbrs = 0;
                particles.push_back(particle);
            }
        }
    }
    renderer.SetParticles(particles);

    float accumulated_time = 0.0f;
    //渲染器初始化，对各种相关对象进行设置。
    //renderer.Init(app);
    auto now = std::chrono::high_resolution_clock::now();
    //int cnt=0;
    for(;;){
        int ident;
        int events;
        android_poll_source *source;
        while ((ident = ALooper_pollAll(engine.canRender ? 0 : -1, nullptr, &events,
                                        (void **)&source)) >= 0) {
            if (source != nullptr) {
                source->process(app, source);
            }
        }
        auto last = now;
        now = std::chrono::high_resolution_clock::now();
        float deltatime = std::chrono::duration<float,std::chrono::seconds::period>(now-last).count();

        //限制在范围内
        float dt = std::clamp(deltatime,1/360.0f,1/60.0f);
        accumulated_time += dt;

        simulatingobj.dt = dt;
        renderer.SetSimulatingObj(simulatingobj);

        boxinfoobj.clampX.y = 1+0.25*(1-glm::cos(5*accumulated_time));
        renderer.SetBoxinfoObj(boxinfoobj);

        //调用Simulate函数，进行新一帧位置的计算。
        renderer.Simulate();

        //调用函数，得到图像的呈现效果。
        renderer.Draw();

        printf("%f\n",1/deltatime);
        //std::this_thread::sleep_for(std::chrono::milliseconds(10));
//        std::string str = std::to_string(cnt);
//        const char* cstr = str.c_str();

        //LOGI("fuck you");
    }

    //renderer.Cleanup();
}

//yuanban
//#undef APIENTRY
//#define NOMINMAX
//
//#define PI 3.1415926f
//
//void renderingLoop();
//
//// 渲染循环标志
//std::atomic<bool> isRendering(false);
//
//android_app * g_app;
//extern "C" JNIEXPORT void JNICALL
//Java_com_example_mytest_MainActivity_startRendering(JNIEnv* env, jobject obj) {
//    isRendering = true;
//    std::thread(renderingLoop).detach(); // 启动渲染线程
//}
//
//// 停止渲染循环
//extern "C" JNIEXPORT void JNICALL
//Java_com_example_mytest_MainActivity_stopRendering(JNIEnv* env, jobject obj) {
//    isRendering = false;
//}
//
//void android_main(struct android_app* app){
//    g_app = app;
//}
//
//void renderingLoop(){
//    float radius = 0.008;
//    float restDesity = 1000.0f;
//    float diam = 2*radius;
//
//    Renderer renderer = Renderer(1080,2400,true);
//
//    //定义渲染对象，包括空间视角变化矩阵、3d到2d的投影矩阵、近景远景裁切、俯视角度。
//    UniformRenderingObject renderingobj{};
//    renderingobj.model = glm::mat4(1.0f);
//    renderingobj.view = glm::lookAt(glm::vec3(1.5,1.3,1.5),glm::vec3(0,0.3,0),glm::vec3(0,1,0));
//    renderingobj.projection = glm::perspective(glm::radians(90.0f),1.0f,0.1f,10.0f);
//    renderingobj.projection[1][1]*=-1;
//    renderingobj.inv_projection = glm::inverse(renderingobj.projection);
//    renderingobj.zNear = 0.1f;
//    renderingobj.zFar = 10.0f;
//    renderingobj.aspect = 1;
//    renderingobj.fovy = glm::radians(90.0f);
//    renderingobj.particleRadius = radius;
//    renderer.SetRenderingObj(renderingobj);
//
//
//    //定义模拟对象，包括两帧相隔的时间，模拟空间的基本物理信息。
//    UniformSimulatingObject simulatingobj{};
//    simulatingobj.dt = 1/240.0f;
//    simulatingobj.restDensity = 1.0f/(diam*diam*diam);
//    simulatingobj.sphRadius = 4*radius;
//
//    simulatingobj.coffPoly6 = 315.0f/(64*PI*pow(simulatingobj.sphRadius,3));
//    simulatingobj.coffGradSpiky = -45/(PI*pow(simulatingobj.sphRadius,4));
//    simulatingobj.coffSpiky = 15/(PI*pow(simulatingobj.sphRadius,3));
//
//    simulatingobj.scorrK = 0.0001;
//    simulatingobj.scorrQ = 0.1;
//    simulatingobj.scorrN = 4;
//    renderer.SetSimulatingObj(simulatingobj);
//
//    //定义流体对象，由粒子组成。
//    UniformNSObject nsobj{};
//    nsobj.sphRadius = 4*radius;
//    renderer.SetNSObj(nsobj);
//
//    UniformBoxInfoObject boxinfoobj{};
//    boxinfoobj.clampX = glm::vec2{0,1.5};
//    boxinfoobj.clampY = glm::vec2{0,1};
//    boxinfoobj.clampZ = glm::vec2{0,1};
//    boxinfoobj.clampX_still = glm::vec2{0,1.5};
//    boxinfoobj.clampY_still = glm::vec2{0,1};
//    boxinfoobj.clampZ_still = glm::vec2{0,1};
//    renderer.SetBoxinfoObj(boxinfoobj);
//
//    //定义粒子群，并指定它们的位置。
//    std::vector<Particle> particles;
//    for(float x=0.25;x<=0.75;x+=diam){
//        for(float z=0.25;z<=0.75;z+=diam){
//            for(float y=0.25;y<=0.75;y+=diam){
//                Particle particle{};
//                particle.Location = glm::vec3(x,y,z);
//
//                particle.Mass = 1;
//                particle.NumNgbrs = 0;
//                particles.push_back(particle);
//            }
//        }
//    }
//    renderer.SetParticles(particles);
//
//    float accumulated_time = 0.0f;
//    //渲染器初始化，对各种相关对象进行设置。
//    renderer.Init(g_app);
//    auto now = std::chrono::high_resolution_clock::now();
//    for(;;){
//        auto last = now;
//        now = std::chrono::high_resolution_clock::now();
//        float deltatime = std::chrono::duration<float,std::chrono::seconds::period>(now-last).count();
//
//        //限制在范围内
//        float dt = std::clamp(deltatime,1/360.0f,1/60.0f);
//        accumulated_time += dt;
//
//        simulatingobj.dt = dt;
//        renderer.SetSimulatingObj(simulatingobj);
//
//        boxinfoobj.clampX.y = 1+0.25*(1-glm::cos(5*accumulated_time));
//        renderer.SetBoxinfoObj(boxinfoobj);
//
//        //调用Simulate函数，进行新一帧位置的计算。
//        renderer.Simulate();
//
//        //调用函数，得到图像的呈现效果。
//        renderer.Draw();
//
//        printf("%f\n",1/deltatime);
//    }
//
//    renderer.Cleanup();
//}
