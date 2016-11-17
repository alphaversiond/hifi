#include "vr/gvr/capi/include/gvr.h"
#include "vr/gvr/capi/include/gvr_types.h"
#include "vr/gvr/capi/include/gvr_controller.h"

#if defined(ANDROID)
#ifndef hifi_display_plugins_DisplayPlugin_h
#define hifi_display_plugins_DisplayPlugin_h

class LibInstance {
public:
  LibInstance(); 
  ~LibInstance() {
    qDebug() << __FILE__ << "has been unloaded";
  }
};

Q_GLOBAL_STATIC(LibInstance, libInstance)

class LibExecutor {
  public: LibExecutor() { libInstance(); }
};
static LibExecutor libExecutor;

extern gvr_context* __gvr_context;
extern std::unique_ptr<gvr::GvrApi> __gvr_api;

#endif
#endif