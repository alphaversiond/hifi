#include <plugins/PluginManager.h>
#include "daydream/DaydreamDisplayPlugin.h"
#include "daydream/DaydreamControllerManager.h"

#if defined(ANDROID)
gvr_context* __gvr_context;

DaydreamLibInstance::DaydreamLibInstance(){
    static std::once_flag once;
    std::call_once(once, [&] {
        qDebug() << __FILE__ << "has been initialized";
        DisplayPlugin* DISPLAY_PLUGIN_POOL[] = {
            new DaydreamDisplayPlugin(),
            nullptr
        };
        PluginManager::getInstance()->loadDisplayPlugins(DISPLAY_PLUGIN_POOL);

        InputPlugin* INPUT_PLUGIN_POOL[] = {
            new DaydreamControllerManager(),
            nullptr
        };

        PluginManager::getInstance()->loadInputPlugins(INPUT_PLUGIN_POOL);
    });
  }

extern "C" {

JNIEXPORT void Java_io_highfidelity_hifiinterface_InterfaceActivity_nativeOnCreate(JNIEnv* env, jobject obj, jobject asset_mgr, jlong gvr_context_ptr) {
    //qDebug() << "nativeOnCreate" << gvr_context_ptr << " On thread " << QThread::currentThreadId();
    __gvr_context = reinterpret_cast<gvr_context*>(gvr_context_ptr);
}

}

 GvrState* GvrState::instance = nullptr;

 GvrState::GvrState(gvr_context *ctx) :
    _gvr_context(ctx),
    _gvr_api(gvr::GvrApi::WrapNonOwned(_gvr_context)),
    _viewport_list(_gvr_api->CreateEmptyBufferViewportList()),
    _scratch_viewport(_gvr_api->CreateBufferViewport()) {

}

void GvrState::init(gvr_context *ctx)
{
    if (ctx && !instance) {
        instance = new GvrState(ctx);
    }
}

GvrState * GvrState::getInstance()
{
    return instance;            
}

#endif