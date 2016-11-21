#include "vr/gvr/capi/include/gvr.h"
#include "vr/gvr/capi/include/gvr_types.h"
#include "vr/gvr/capi/include/gvr_controller.h"

#if defined(ANDROID)
#ifndef hifi_daydream_plugin_h
#define hifi_daydream_plugin_h

class DaydreamLibInstance {
public:
  DaydreamLibInstance(); 
  ~DaydreamLibInstance() {
    qDebug() << __FILE__ << "has been unloaded";
  }
};

Q_GLOBAL_STATIC(DaydreamLibInstance, daydreamLibInstance)

class LibExecutor {
  public: LibExecutor() { daydreamLibInstance(); }
};
static LibExecutor libExecutor;

extern gvr_context* __gvr_context;
extern std::unique_ptr<gvr::GvrApi> __gvr_api;

class GvrState {

public:
	GvrState(gvr_context *ctx);

	static GvrState *instance;
	
	static void init(gvr_context *ctx);

	static GvrState * getInstance();

    gvr_context* _gvr_context;
    std::unique_ptr<gvr::GvrApi> _gvr_api;

    std::unique_ptr<gvr::SwapChain> _swapchain;
    std::unique_ptr<gvr::ControllerApi> _controller_api;
    gvr::BufferViewportList _viewport_list;
    gvr::BufferViewport _scratch_viewport;
    // Size of the offscreen framebuffer.
    gvr::Sizei _framebuf_size;

    // The last controller state (updated once per frame).
    gvr::ControllerState _controller_state;

	int32_t _last_controller_api_status;
    int32_t _last_controller_connection_state;

};

#endif
#endif