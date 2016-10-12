//
//  Created by Bradley Austin Davis on 2015/05/29
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
#include "OpenGLDisplayPlugin.h"

#include <condition_variable>
#include <queue>

#include <QtCore/QCoreApplication>
#include <QtCore/QThread>
#include <QtCore/QTimer>

#include <QtOpenGL/QGLWidget>
#include <QtGui/QImage>

#if defined(Q_OS_MAC)
#include <OpenGL/CGLCurrent.h>
#endif

#include <NumericalConstants.h>
#include <DependencyManager.h>
#include <GLMHelpers.h>

#include <gl/QOpenGLContextWrapper.h>
#include <gl/GLWidget.h>
#include <gl/Config.h>
#include <gl/GLEscrow.h>
#include <gl/Context.h>

#include <gpu/Texture.h>
#include <gpu/StandardShaderLib.h>
#include <gpu/gl/GLShared.h>
#include <gpu/gl/GLBackend.h>
#include <GeometryCache.h>

#include <FramebufferCache.h>
#include <shared/NsightHelpers.h>
#include <ui-plugins/PluginContainer.h>
#include <ui/Menu.h>
#include <CursorManager.h>

#include "CompositorHelper.h"

const char* SRGB_TO_LINEAR_FRAG = R"SCRIBE(

uniform sampler2D colorMap;

in vec2 varTexCoord0;

out vec4 outFragColor;

float sRGBFloatToLinear(float value) {
    const float SRGB_ELBOW = 0.04045;
    
    return (value <= SRGB_ELBOW) ? value / 12.92 : pow((value + 0.055) / 1.055, 2.4);
}

vec3 colorToLinearRGB(vec3 srgb) {
    return vec3(sRGBFloatToLinear(srgb.r), sRGBFloatToLinear(srgb.g), sRGBFloatToLinear(srgb.b));
}

void main(void) {
    outFragColor.a = 1.0;
    outFragColor.rgb = colorToLinearRGB(texture(colorMap, varTexCoord0).rgb);
}

)SCRIBE";

extern QThread* RENDER_THREAD;

class PresentThread : public QThread, public Dependency {
    using Mutex = std::mutex;
    using Condition = std::condition_variable;
    using Lock = std::unique_lock<Mutex>;
public:

    PresentThread() {
        connect(qApp, &QCoreApplication::aboutToQuit, [this] {
            shutdown();
        });
    }

    ~PresentThread() {
        shutdown();
    }

    void shutdown() {
        if (isRunning()) {
            // First ensure that we turn off any current display plugin
            setNewDisplayPlugin(nullptr);

            Lock lock(_mutex);
            _shutdown = true;
            _condition.wait(lock, [&] { return !_shutdown;  });
            qDebug() << "Present thread shutdown";
        }
    }


    void setNewDisplayPlugin(OpenGLDisplayPlugin* plugin) {
        Lock lock(_mutex);
        qDebug()<<"setNewDisplayPlugin 00102";
        if (isRunning()) {
            qDebug()<<"setNewDisplayPlugin 00104 " << plugin;
            _newPluginQueue.push(plugin);
            qDebug()<<"setNewDisplayPlugin 00106 " << _newPluginQueue.size();
            _condition.wait(lock, [=]()->bool {
                     qDebug()<<"setNewDisplayPlugin 001xxx";
                     bool isEmpty = _newPluginQueue.empty(); 
                     qDebug()<<"Isempty: "<< isEmpty;
                     return true;
                });
        }
        qDebug()<<"setNewDisplayPlugin 00111";

    }

    void setContext(gl::Context* context) {
        // Move the OpenGL context to the present thread
        // Extra code because of the widget 'wrapper' context
        _context = context;
        _context->moveToThread(this);
    }


    virtual void run() override {
        // FIXME determine the best priority balance between this and the main thread...
        // It may be dependent on the display plugin being used, since VR plugins should 
        // have higher priority on rendering (although we could say that the Oculus plugin
        // doesn't need that since it has async timewarp).
        // A higher priority here 
        qDebug()<<"RUN 00100";

        setPriority(QThread::HighPriority);
        OpenGLDisplayPlugin* currentPlugin{ nullptr };
        Q_ASSERT(_context);
        qDebug()<<"RUN 00105";
        _context->makeCurrent();
        while (!_shutdown) {
            if (_pendingMainThreadOperation) {
                PROFILE_RANGE("MainThreadOp") 
                {
                    Lock lock(_mutex);
                    _context->doneCurrent();
                    // Move the context to the main thread
                    _context->moveToThread(qApp->thread());
                    _pendingMainThreadOperation = false;
                    // Release the main thread to do it's action
                    _condition.notify_one();
                }


                {
                    // Main thread does it's thing while we wait on the lock to release
                    Lock lock(_mutex);
                    _condition.wait(lock, [&] { return _finishedMainThreadOperation; });
                }
            }

            // Check for a new display plugin
            {
                Lock lock(_mutex);
                // Check if we have a new plugin to activate
                while (!_newPluginQueue.empty()) {
                    auto newPlugin = _newPluginQueue.front();
                    if (newPlugin != currentPlugin) {
                        // Deactivate the old plugin

                        if (currentPlugin != nullptr) {
                            _context->makeCurrent();
                            currentPlugin->uncustomizeContext();
                            CHECK_GL_ERROR();
                            _context->doneCurrent();
                        }

                        if (newPlugin) {
                            bool hasVsync = true;
                            bool wantVsync = newPlugin->wantVsync();
                            _context->makeCurrent();
#if defined(Q_OS_WIN)
                            wglSwapIntervalEXT(wantVsync ? 1 : 0);
                            hasVsync = wglGetSwapIntervalEXT() != 0;
#elif defined(Q_OS_MAC)
                            GLint interval = wantVsync ? 1 : 0;
                            newPlugin->swapBuffers();

                            CGLSetParameter(CGLGetCurrentContext(), kCGLCPSwapInterval, &interval);
                            newPlugin->swapBuffers();
                            CGLGetParameter(CGLGetCurrentContext(), kCGLCPSwapInterval, &interval);
                            hasVsync = interval != 0;

#else
                            // TODO: Fill in for linux
                            //Q_UNUSED(wantVsync);
#endif

                            newPlugin->setVsyncEnabled(hasVsync);
                            newPlugin->customizeContext();

                            CHECK_GL_ERROR();
                            _context->doneCurrent();
                        }
                        currentPlugin = newPlugin;

                        _newPluginQueue.pop();
                        _condition.notify_one();
                    }
                }
            }

            // If there's no active plugin, just sleep
            if (currentPlugin == nullptr) {
                // Minimum sleep ends up being about 2 ms anyway
                QThread::msleep(1);
                continue;
            }

            // Execute the frame and present it to the display device.
            _context->makeCurrent();
            {
                PROFILE_RANGE("PluginPresent")
                currentPlugin->present();
                CHECK_GL_ERROR();
            }

            _context->doneCurrent();
        }

        Lock lock(_mutex);
        _context->moveToThread(qApp->thread());
        _shutdown = false;
        _condition.notify_one();
    }

    void withMainThreadContext(std::function<void()> f) {
        // Signal to the thread that there is work to be done on the main thread
        Lock lock(_mutex);
        _pendingMainThreadOperation = true;
        _finishedMainThreadOperation = false;
        _condition.wait(lock, [&] { return !_pendingMainThreadOperation; });

        _context->makeCurrent();
        f();
        _context->doneCurrent();

        // Move the context back to the presentation thread
        _context->moveToThread(this);

        // restore control of the context to the presentation thread and signal 
        // the end of the operation
        _finishedMainThreadOperation = true;
        lock.unlock();
        _condition.notify_one();
    }


private:
    void makeCurrent();
    void doneCurrent();

    bool _shutdown { false };
    Mutex _mutex;
    // Used to allow the main thread to perform context operations
    Condition _condition;
    bool _pendingMainThreadOperation { false };
    bool _finishedMainThreadOperation { false };
    QThread* _mainThread { nullptr };
    std::queue<OpenGLDisplayPlugin*> _newPluginQueue;
    gl::Context* _context { nullptr };
};

bool OpenGLDisplayPlugin::activate() {
    qDebug() << "OpenGLDisplayPlugin::activate 00100";
    if (!_cursorsData.size()) {
        qDebug() << "OpenGLDisplayPlugin::activate 00102";
        auto& cursorManager = Cursor::Manager::instance();
        qDebug() << "OpenGLDisplayPlugin::activate 00103";
        for (const auto iconId : cursorManager.registeredIcons()) {
            qDebug() << "OpenGLDisplayPlugin::activate 00104";
            auto& cursorData = _cursorsData[iconId];
            qDebug() << "OpenGLDisplayPlugin::activate 00105";
            auto iconPath = cursorManager.getIconImage(iconId);
            qDebug() << "OpenGLDisplayPlugin::activate 00106";
            auto image = QImage(iconPath);
            qDebug() << "OpenGLDisplayPlugin::activate 00107";
            image = image.mirrored();
            qDebug() << "OpenGLDisplayPlugin::activate 00108";
            image = image.convertToFormat(QImage::Format_RGBA8888);
            qDebug() << "OpenGLDisplayPlugin::activate 00109";
            cursorData.image = image;
            qDebug() << "OpenGLDisplayPlugin::activate 00110";
            cursorData.size = toGlm(image.size());
            qDebug() << "OpenGLDisplayPlugin::activate 00111";
            cursorData.hotSpot = vec2(0.5f);
            qDebug() << "OpenGLDisplayPlugin::activate 00112";
        }
    }
    if (!_container) {
        qDebug() << "OpenGLDisplayPlugin::activate 00114";
        return false;
    }

    // Start the present thread if necessary
    QSharedPointer<PresentThread> presentThread;
    if (DependencyManager::isSet<PresentThread>()) {
        qDebug() << "OpenGLDisplayPlugin::activate 00115";
        presentThread = DependencyManager::get<PresentThread>();
        qDebug() << "OpenGLDisplayPlugin::activate 00116";
    } else {
        qDebug() << "OpenGLDisplayPlugin::activate 00118";
        auto widget = _container->getPrimaryWidget();
        qDebug() << "OpenGLDisplayPlugin::activate 00119";
        DependencyManager::set<PresentThread>();
        qDebug() << "OpenGLDisplayPlugin::activate 00120";
        presentThread = DependencyManager::get<PresentThread>();
        qDebug() << "OpenGLDisplayPlugin::activate 00121";
        presentThread->setObjectName("Presentation Thread");
        qDebug() << "OpenGLDisplayPlugin::activate 00122";
        presentThread->setContext(widget->context());
        qDebug() << "OpenGLDisplayPlugin::activate 00123";
        // Start execution
        presentThread->start();
        qDebug() << "OpenGLDisplayPlugin::activate 00124";
    }

    qDebug() << "OpenGLDisplayPlugin::activate 00125";

    _presentThread = presentThread.data();
    if (!RENDER_THREAD) {
    qDebug() << "OpenGLDisplayPlugin::activate 00126";
        RENDER_THREAD = _presentThread;
    }
    
    // Child classes may override this in order to do things like initialize
    // libraries, etc
        qDebug() << "OpenGLDisplayPlugin::activate 00127";

    if (!internalActivate()) {
            qDebug() << "OpenGLDisplayPlugin::activate 00128";

        return false;
    }


    // This should not return until the new context has been customized
    // and the old context (if any) has been uncustomized
    qDebug() << "OpenGLDisplayPlugin::activate 00127bis";

    presentThread->setNewDisplayPlugin(this);
    qDebug() << "OpenGLDisplayPlugin::activate 00129";

    auto compositorHelper = DependencyManager::get<CompositorHelper>();
    connect(compositorHelper.data(), &CompositorHelper::alphaChanged, [this] {
    qDebug() << "OpenGLDisplayPlugin::activate 00130";
        auto compositorHelper = DependencyManager::get<CompositorHelper>();
        auto animation = new QPropertyAnimation(this, "overlayAlpha");
        animation->setDuration(200);
        animation->setEndValue(compositorHelper->getAlpha());
            qDebug() << "OpenGLDisplayPlugin::activate 00131";

        animation->start();
            qDebug() << "OpenGLDisplayPlugin::activate 00132";

    });
    qDebug() << "OpenGLDisplayPlugin::activate 00133";

    if (isHmd() && (getHmdScreen() >= 0)) {
            qDebug() << "OpenGLDisplayPlugin::activate 00133";

        _container->showDisplayPluginsTools();
    }
    qDebug() << "OpenGLDisplayPlugin::activate 00134";

    return Parent::activate();
}

void OpenGLDisplayPlugin::deactivate() {
    auto compositorHelper = DependencyManager::get<CompositorHelper>();
    disconnect(compositorHelper.data());

    auto presentThread = DependencyManager::get<PresentThread>();
    // Does not return until the GL transition has completeed
    presentThread->setNewDisplayPlugin(nullptr);
    internalDeactivate();

    _container->showDisplayPluginsTools(false);
    if (!_container->currentDisplayActions().isEmpty()) {
        auto menu = _container->getPrimaryMenu();
        foreach(auto itemInfo, _container->currentDisplayActions()) {
            menu->removeMenuItem(itemInfo.first, itemInfo.second);
        }
        _container->currentDisplayActions().clear();
    }
    Parent::deactivate();
}

void OpenGLDisplayPlugin::customizeContext() {
    qDebug() << "customizeContext A0100";
    auto presentThread = DependencyManager::get<PresentThread>();
    qDebug() << "customizeContext A0101" << presentThread; 
    Q_ASSERT(thread() == presentThread->thread());
    qDebug() << "customizeContext A0102 asserte passed " << getGLBackend();
    getGLBackend()->setCameraCorrection(mat4());

    qDebug() << "customizeContext A0105";

    for (auto& cursorValue : _cursorsData) {
        auto& cursorData = cursorValue.second;
        if (!cursorData.texture) {
            qDebug() << "customizeContext A0110";

            auto image = cursorData.image;
            qDebug() << "customizeContext A0111" << image;

            if (image.format() != QImage::Format_ARGB32) {
                image = image.convertToFormat(QImage::Format_ARGB32);
            }
            if ((image.width() > 0) && (image.height() > 0)) {
                qDebug() << "customizeContext A0120";

                cursorData.texture.reset(
                    gpu::Texture::create2D(
                        gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA), 
                        image.width(), image.height(), 
                        gpu::Sampler(gpu::Sampler::FILTER_MIN_MAG_MIP_LINEAR)));
                cursorData.texture->setSource("cursor texture");
                auto usage = gpu::Texture::Usage::Builder().withColor().withAlpha();
                cursorData.texture->setUsage(usage.build());
                cursorData.texture->assignStoredMip(0, gpu::Element(gpu::VEC4, gpu::NUINT8, gpu::RGBA), image.byteCount(), image.constBits());
                cursorData.texture->autoGenerateMips(-1);
            }
        }
    }

    if (!_presentPipeline) {
        {
            auto vs = gpu::StandardShaderLib::getDrawUnitQuadTexcoordVS();
            auto ps = gpu::StandardShaderLib::getDrawTexturePS();
            gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);
            gpu::Shader::makeProgram(*program);
            gpu::StatePointer state = gpu::StatePointer(new gpu::State());
            state->setDepthTest(gpu::State::DepthTest(false));
            state->setScissorEnable(true);
            _simplePipeline = gpu::Pipeline::create(program, state);
        }

        {
            auto vs = gpu::StandardShaderLib::getDrawUnitQuadTexcoordVS();
            auto ps = gpu::Shader::createPixel(std::string(SRGB_TO_LINEAR_FRAG));
            gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);
            gpu::Shader::makeProgram(*program);
            gpu::StatePointer state = gpu::StatePointer(new gpu::State());
            state->setDepthTest(gpu::State::DepthTest(false));
            state->setScissorEnable(true);
            _presentPipeline = gpu::Pipeline::create(program, state);
        }

        {
            auto vs = gpu::StandardShaderLib::getDrawUnitQuadTexcoordVS();
            auto ps = gpu::StandardShaderLib::getDrawTexturePS();
            gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);
            gpu::Shader::makeProgram(*program);
            gpu::StatePointer state = gpu::StatePointer(new gpu::State());
            state->setDepthTest(gpu::State::DepthTest(false));
            state->setBlendFunction(true, 
                gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
                gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);
            _overlayPipeline = gpu::Pipeline::create(program, state);
        }

        {
            auto vs = gpu::StandardShaderLib::getDrawTransformUnitQuadVS();
            auto ps = gpu::StandardShaderLib::getDrawTexturePS();
            gpu::ShaderPointer program = gpu::Shader::createProgram(vs, ps);
            gpu::Shader::makeProgram(*program);
            gpu::StatePointer state = gpu::StatePointer(new gpu::State());
            state->setDepthTest(gpu::State::DepthTest(false));
            state->setBlendFunction(true,
                gpu::State::SRC_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::INV_SRC_ALPHA,
                gpu::State::FACTOR_ALPHA, gpu::State::BLEND_OP_ADD, gpu::State::ONE);
            _cursorPipeline = gpu::Pipeline::create(program, state);
        }
    }
    updateCompositeFramebuffer();
}

void OpenGLDisplayPlugin::uncustomizeContext() {
    _presentPipeline.reset();
    _cursorPipeline.reset();
    _overlayPipeline.reset();
    _compositeFramebuffer.reset();
    withPresentThreadLock([&] {
        _currentFrame.reset();
        _lastFrame = nullptr;
        while (!_newFrameQueue.empty()) {
            _gpuContext->consumeFrameUpdates(_newFrameQueue.front());
            _newFrameQueue.pop();
        }
    });
}


// Pressing Alt (and Meta) key alone activates the menubar because its style inherits the
// SHMenuBarAltKeyNavigation from QWindowsStyle. This makes it impossible for a scripts to
// receive keyPress events for the Alt (and Meta) key in a reliable manner.
//
// This filter catches events before QMenuBar can steal the keyboard focus.
// The idea was borrowed from
// http://www.archivum.info/qt-interest@trolltech.com/2006-09/00053/Re-(Qt4)-Alt-key-focus-QMenuBar-(solved).html

// Pass input events on to the application
bool OpenGLDisplayPlugin::eventFilter(QObject* receiver, QEvent* event) {
    switch (event->type()) {
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseMove:
        case QEvent::Wheel:

        case QEvent::TouchBegin:
        case QEvent::TouchEnd:
        case QEvent::TouchUpdate:

        case QEvent::FocusIn:
        case QEvent::FocusOut:

        case QEvent::KeyPress:
        case QEvent::KeyRelease:
        case QEvent::ShortcutOverride:

        case QEvent::DragEnter:
        case QEvent::Drop:

        case QEvent::Resize:
            if (QCoreApplication::sendEvent(QCoreApplication::instance(), event)) {
                return true;
            }
            break;
        default:
            break;
    }
    return false;
}

void OpenGLDisplayPlugin::submitFrame(const gpu::FramePointer& newFrame) {
    withNonPresentThreadLock([&] {
        _newFrameQueue.push(newFrame);
    });
}

void OpenGLDisplayPlugin::updateFrameData() {
    if (_lockCurrentTexture) {
        return;
    }
    withPresentThreadLock([&] {
        if (!_newFrameQueue.empty()) {
            // We're changing frames, so we can cleanup any GL resources that might have been used by the old frame
            _gpuContext->recycle();
        }
        if (_newFrameQueue.size() > 1) {
            _droppedFrameRate.increment(_newFrameQueue.size() - 1);
        }
        while (!_newFrameQueue.empty()) {
            _currentFrame = _newFrameQueue.front();
            _newFrameQueue.pop();
            _gpuContext->consumeFrameUpdates(_currentFrame);
        }
    });
}

void OpenGLDisplayPlugin::compositeOverlay() {
    render([&](gpu::Batch& batch){
        batch.enableStereo(false);
        batch.setFramebuffer(_compositeFramebuffer);
        batch.setPipeline(_overlayPipeline);
        batch.setResourceTexture(0, _currentFrame->overlay);
        if (isStereo()) {
            for_each_eye([&](Eye eye) {
                batch.setViewportTransform(eyeViewport(eye));
                batch.draw(gpu::TRIANGLE_STRIP, 4);
            });
        } else {
            batch.setViewportTransform(ivec4(uvec2(0), _compositeFramebuffer->getSize()));
            batch.draw(gpu::TRIANGLE_STRIP, 4);
        }
    });
}

void OpenGLDisplayPlugin::compositePointer() {
    auto& cursorManager = Cursor::Manager::instance();
    const auto& cursorData = _cursorsData[cursorManager.getCursor()->getIcon()];
    auto cursorTransform = DependencyManager::get<CompositorHelper>()->getReticleTransform(glm::mat4());
    render([&](gpu::Batch& batch) {
        batch.enableStereo(false);
        batch.setProjectionTransform(mat4());
        batch.setFramebuffer(_compositeFramebuffer);
        batch.setPipeline(_cursorPipeline);
        batch.setResourceTexture(0, cursorData.texture);
        batch.resetViewTransform();
        batch.setModelTransform(cursorTransform);
        if (isStereo()) {
            for_each_eye([&](Eye eye) {
                batch.setViewportTransform(eyeViewport(eye));
                batch.draw(gpu::TRIANGLE_STRIP, 4);
            });
        } else {
            batch.setViewportTransform(ivec4(uvec2(0), _compositeFramebuffer->getSize()));
            batch.draw(gpu::TRIANGLE_STRIP, 4);
        }
    });
}

void OpenGLDisplayPlugin::compositeScene() {
    render([&](gpu::Batch& batch) {
        batch.enableStereo(false);
        batch.setFramebuffer(_compositeFramebuffer);
        batch.setViewportTransform(ivec4(uvec2(), _compositeFramebuffer->getSize()));
        batch.setStateScissorRect(ivec4(uvec2(), _compositeFramebuffer->getSize()));
        batch.resetViewTransform();
        batch.setProjectionTransform(mat4());
        batch.setPipeline(_simplePipeline);
        batch.setResourceTexture(0, _currentFrame->framebuffer->getRenderBuffer(0));
        batch.draw(gpu::TRIANGLE_STRIP, 4);
    });
}

void OpenGLDisplayPlugin::compositeLayers() {
    updateCompositeFramebuffer();

    {
        PROFILE_RANGE_EX("compositeScene", 0xff0077ff, (uint64_t)presentCount())
        compositeScene();
    }

    {
        PROFILE_RANGE_EX("compositeOverlay", 0xff0077ff, (uint64_t)presentCount())
        compositeOverlay();
    }
    auto compositorHelper = DependencyManager::get<CompositorHelper>();
    if (compositorHelper->getReticleVisible()) {
        PROFILE_RANGE_EX("compositePointer", 0xff0077ff, (uint64_t)presentCount())
        compositePointer();
    }

    {
        PROFILE_RANGE_EX("compositeExtra", 0xff0077ff, (uint64_t)presentCount())
        compositeExtra();
    }
}

void OpenGLDisplayPlugin::internalPresent() {
    render([&](gpu::Batch& batch) {
        batch.enableStereo(false);
        batch.resetViewTransform();
        batch.setFramebuffer(gpu::FramebufferPointer());
        batch.setViewportTransform(ivec4(uvec2(0), getSurfacePixels()));
        batch.setResourceTexture(0, _compositeFramebuffer->getRenderBuffer(0));
        batch.setPipeline(_presentPipeline);
        batch.draw(gpu::TRIANGLE_STRIP, 4);
    });
    swapBuffers();
    _presentRate.increment();
}

void OpenGLDisplayPlugin::present() {
    PROFILE_RANGE_EX(__FUNCTION__, 0xffffff00, (uint64_t)presentCount())
    updateFrameData();
    incrementPresentCount();

    {
        PROFILE_RANGE_EX("recycle", 0xff00ff00, (uint64_t)presentCount())
        _gpuContext->recycle();
    }

    if (_currentFrame) {
        {
            withPresentThreadLock([&] {
                _renderRate.increment();
                if (_currentFrame.get() != _lastFrame) {
                    _newFrameRate.increment();
                }
                _lastFrame = _currentFrame.get();
            });
            // Execute the frame rendering commands
            PROFILE_RANGE_EX("execute", 0xff00ff00, (uint64_t)presentCount())
            _gpuContext->executeFrame(_currentFrame);
        }

        // Write all layers to a local framebuffer
        {
            PROFILE_RANGE_EX("composite", 0xff00ffff, (uint64_t)presentCount())
            compositeLayers();
        }

        // Take the composite framebuffer and send it to the output device
        {
            PROFILE_RANGE_EX("internalPresent", 0xff00ffff, (uint64_t)presentCount())
            internalPresent();
        }

        gpu::Backend::setFreeGPUMemory(gpu::gl::getFreeDedicatedMemory());
    }
}

float OpenGLDisplayPlugin::newFramePresentRate() const {
    return _newFrameRate.rate();
}

float OpenGLDisplayPlugin::droppedFrameRate() const {
    return _droppedFrameRate.rate();
}

float OpenGLDisplayPlugin::presentRate() const {
    return _presentRate.rate();
}

float OpenGLDisplayPlugin::renderRate() const { 
    return _renderRate.rate();
}


void OpenGLDisplayPlugin::swapBuffers() {
    static auto context = _container->getPrimaryWidget()->context();
    context->swapBuffers();
}

void OpenGLDisplayPlugin::withMainThreadContext(std::function<void()> f) const {
    static auto presentThread = DependencyManager::get<PresentThread>();
    presentThread->withMainThreadContext(f);
    _container->makeRenderingContextCurrent();
}

QImage OpenGLDisplayPlugin::getScreenshot(float aspectRatio) const {
    auto size = _compositeFramebuffer->getSize();
    if (isHmd()) {
        size.x /= 2;
    }
    auto bestSize = size;
    uvec2 corner(0);
    if (aspectRatio != 0.0f) { // Pick out the largest piece of the center that produces the requested width/height aspectRatio
        if (ceil(size.y * aspectRatio) < size.x) {
            bestSize.x = round(size.y * aspectRatio);
        } else {
            bestSize.y = round(size.x / aspectRatio);
        }
        corner.x = round((size.x - bestSize.x) / 2.0f);
        corner.y = round((size.y - bestSize.y) / 2.0f);
    }
    auto glBackend = const_cast<OpenGLDisplayPlugin&>(*this).getGLBackend();
    QImage screenshot(bestSize.x, bestSize.y, QImage::Format_ARGB32);
    withMainThreadContext([&] {
        glBackend->downloadFramebuffer(_compositeFramebuffer, ivec4(corner, bestSize), screenshot);
    });
    return screenshot.mirrored(false, true);
}

glm::uvec2 OpenGLDisplayPlugin::getSurfacePixels() const {
    uvec2 result;
    auto window = _container->getPrimaryWidget();
    if (window) {
        result = toGlm(window->geometry().size() * window->devicePixelRatio());
    }
    return result;
}

glm::uvec2 OpenGLDisplayPlugin::getSurfaceSize() const {
    uvec2 result;
    auto window = _container->getPrimaryWidget();
    if (window) {
        result = toGlm(window->geometry().size());
    }
    return result;
}

bool OpenGLDisplayPlugin::hasFocus() const {
    auto window = _container->getPrimaryWidget();
    return window ? window->hasFocus() : false;
}

void OpenGLDisplayPlugin::assertNotPresentThread() const {
    Q_ASSERT(QThread::currentThread() != _presentThread);
}

void OpenGLDisplayPlugin::assertIsPresentThread() const {
    Q_ASSERT(QThread::currentThread() == _presentThread);
}

bool OpenGLDisplayPlugin::beginFrameRender(uint32_t frameIndex) {
    withNonPresentThreadLock([&] {
        _compositeOverlayAlpha = _overlayAlpha;
    });
    return Parent::beginFrameRender(frameIndex);
}

ivec4 OpenGLDisplayPlugin::eyeViewport(Eye eye) const {
    uvec2 vpSize = _compositeFramebuffer->getSize();
    vpSize.x /= 2;
    uvec2 vpPos;
    if (eye == Eye::Right) {
        vpPos.x = vpSize.x;
    }
    return ivec4(vpPos, vpSize);
}

gpu::gl::GLBackend* OpenGLDisplayPlugin::getGLBackend() {
    qDebug() << "Getting GL Backend " << (_gpuContext?1:0);
    if (!_gpuContext || !_gpuContext->getBackend()) {
        return nullptr;
    }
    auto backend = _gpuContext->getBackend().get();
    qDebug() << "Getting GL Backend 2 " << (backend?1:0);
#if defined(Q_OS_MAC) || defined(ANDROID)
    // Should be dynamic_cast, but that doesn't work in plugins on OSX
    auto glbackend = static_cast<gpu::gl::GLBackend*>(backend);
#else
    auto glbackend = dynamic_cast<gpu::gl::GLBackend*>(backend);
#endif
    qDebug() << "Getting GL Backend 3 " << (glbackend?1:0);

    return glbackend;
}

void OpenGLDisplayPlugin::render(std::function<void(gpu::Batch& batch)> f) {
    gpu::Batch batch;
    f(batch);
    _gpuContext->executeBatch(batch);
}


OpenGLDisplayPlugin::~OpenGLDisplayPlugin() {
}

void OpenGLDisplayPlugin::updateCompositeFramebuffer() {
    auto renderSize = getRecommendedRenderSize();
    if (!_compositeFramebuffer || _compositeFramebuffer->getSize() != renderSize) {
        _compositeFramebuffer = gpu::FramebufferPointer(gpu::Framebuffer::create("OpenGLDisplayPlugin::composite", gpu::Element::COLOR_RGBA_32, renderSize.x, renderSize.y));
    }
}
