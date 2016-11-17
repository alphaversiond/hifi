#include <plugins/PluginManager.h>
#include "daydream/DaydreamDisplayPlugin.h"

#if defined(ANDROID)
gvr_context* __gvr_context;

LibInstance::LibInstance(){
    static std::once_flag once;
    std::call_once(once, [&] {
        qDebug() << __FILE__ << "has been initialized";
        DisplayPlugin* PLUGIN_POOL[] = {
            new DaydreamDisplayPlugin(),
//            new Basic2DWindowOpenGLDisplayPlugin(),
//            new SideBySideStereoDisplayPlugin(),
//            new DebugHmdDisplayPlugin(),
            nullptr
        };
        PluginManager::getInstance()->loadDisplayPlugins(PLUGIN_POOL);
    });
  }

extern "C" {

JNIEXPORT void Java_io_highfidelity_hifiinterface_InterfaceActivity_nativeOnCreate(JNIEnv* env, jobject obj, jobject asset_mgr, jlong gvr_context_ptr) {
    //qDebug() << "nativeOnCreate" << gvr_context_ptr << " On thread " << QThread::currentThreadId();
    __gvr_context = reinterpret_cast<gvr_context*>(gvr_context_ptr);
}

}

#endif