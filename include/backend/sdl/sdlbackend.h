/*
    Copyright (c) 2015, Robin Burchell <robin.burchell@viroteck.net>
    Copyright (c) 2015, Gunnar Sletta <gunnar@sletta.org>
    All rights reserved.

    Redistribution and use in source and binary forms, with or without
    modification, are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice,
       this list of conditions and the following disclaimer in the documentation
       and/or other materials provided with the distribution.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
    ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
    WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
    DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
    ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
    (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
    LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
    ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
    (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
    SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <SDL.h>
#include <unordered_map>
#include <chrono>

RENGINE_BEGIN_NAMESPACE

class SDLBackend;
class SdlSurface;
class SdlWindow;

inline void SDLBackend_die(const char *msg)
{
    printf("%s: %s\n", msg, SDL_GetError());
    SDL_Quit();
    exit(1);
}

class SDLBackend : public Backend, SurfaceBackendImpl
{
public:
    SDLBackend()
    {
        if (SDL_Init(SDL_INIT_VIDEO) < 0)
            SDLBackend_die("Unable to initialize SDL");
        logi << "SDLBackend: created..." << std::endl;

        m_nextUpdateTime = m_clock.now();
    }

    ~SDLBackend()
    {
        SDL_Quit();
    }

    void processEvents() override;

    SurfaceBackendImpl *createSurface(Surface *iface) override;
    void destroySurface(Surface *surface, SurfaceBackendImpl *impl) override;

    Renderer *createRenderer() override;

    void sendPointerEvent(SDL_Event *e, Event::Type type);
    void sendKeyEvent(SDL_Event *sdlEvent, Event::Type type);

    unsigned devicePixelRatio() const;

    bool beginRender() override;
    bool commitRender() override;

    void show() override;
    void hide() override;
    vec2 size() const override;

    void requestSize(vec2 size) override;

    void requestRender() override;

    vec2 dpi() const override;

    void quit() override;


private:
    Surface *m_surface = nullptr;
    SDL_Window *m_window = nullptr;
    SDL_GLContext m_gl = nullptr;

    bool m_renderRequested = false;

    std::chrono::steady_clock m_clock;
    std::chrono::steady_clock::time_point m_nextUpdateTime;
};


inline unsigned SDLBackend::devicePixelRatio() const
{
    int dw, dh, ww, wh;
    SDL_GetWindowSize(m_window, &ww, &wh);
    SDL_GL_GetDrawableSize(m_window, &dw, &dh);
    return dw / ww;
}

inline vec2 SDLBackend::size() const
{
    int w;
    int h;
    SDL_GL_GetDrawableSize(m_window, &w, &h);
    return vec2(w, h);
}


inline void SDLBackend::processEvents()
{
    using namespace std::chrono_literals;
    using namespace std::chrono;

    SDL_Event event;
    int evt = 0;
    const steady_clock::time_point now = m_clock.now();
    const milliseconds waitTime  = duration_cast<milliseconds>(m_nextUpdateTime - now);
    m_nextUpdateTime = now + 16ms;
    if (waitTime < milliseconds::zero()) {
        evt = SDL_PollEvent(nullptr);
    } else {
        evt = SDL_WaitEventTimeout(nullptr, waitTime.count());
    }

    // This odd-looking construct ensures we do not process events that are
    // pushed onto the queue after we start processing, so as to not starve the
    // main loop.
    while (evt-- > 0) {
        SDL_PollEvent(&event);

        switch (event.type) {
        case SDL_USEREVENT: {
            // reset this before onRender so we don't prevent onRender from
            // scheduling another one..
            m_renderRequested = false;
            m_surface->onRender();
            break;
        }
        case SDL_MOUSEBUTTONDOWN: {
            sendPointerEvent(&event, Event::PointerDown);
            break;
        }
        case SDL_MOUSEBUTTONUP: {
            sendPointerEvent(&event, Event::PointerUp);
            break;
        }
        case SDL_MOUSEMOTION: {
            sendPointerEvent(&event, Event::PointerMove);
            break;
        }
        case SDL_KEYUP:
            sendKeyEvent(&event, Event::KeyUp);
            break;
        case SDL_KEYDOWN:
            sendKeyEvent(&event, Event::KeyDown);
            break;
        case SDL_QUIT: {
            m_running = false;
            break;
        }
        case SDL_WINDOWEVENT: {
            Uint32 windowID = SDL_GetWindowID(m_window);
            if (event.window.windowID != windowID) {
                break;
            }
            if (event.window.event == SDL_WINDOWEVENT_RESIZED) {
                requestRender();
            }

            break;
        }
        default: {
            // logw << "unknown event.type " << event.type << std::endl;
        }
        }
    }

    m_surface->onTick();
}

inline void SDLBackend::sendPointerEvent(SDL_Event *sdlEvent, Event::Type type)
{
    assert(m_window);

    // The mouse might go outside our window
    if (SDL_GetWindowID(m_window) != sdlEvent->button.windowID) {
        return;
    }

    assert(m_surface);

    PointerEvent pe(type);
    pe.initialize(vec2(sdlEvent->button.x, sdlEvent->button.y) * devicePixelRatio());
    m_surface->onEvent(&pe);
}

inline void SDLBackend::sendKeyEvent(SDL_Event *sdlEvent, Event::Type type)
{
    assert(m_window);
    assert(SDL_GetWindowID(m_window) == sdlEvent->key.windowID);
    assert(m_surface);

    // Can recommend "generating" this with vim
    const static std::unordered_map<SDL_Keycode, KeyEvent::Key> keyMap ({
        {SDLK_0, KeyEvent::Key_0},
        {SDLK_1, KeyEvent::Key_1},
        {SDLK_2, KeyEvent::Key_2},
        {SDLK_3, KeyEvent::Key_3},
        {SDLK_4, KeyEvent::Key_4},
        {SDLK_5, KeyEvent::Key_5},
        {SDLK_6, KeyEvent::Key_6},
        {SDLK_7, KeyEvent::Key_7},
        {SDLK_8, KeyEvent::Key_8},
        {SDLK_9, KeyEvent::Key_9},
        {SDLK_a, KeyEvent::Key_A},
        {SDLK_b, KeyEvent::Key_B},
        {SDLK_c, KeyEvent::Key_C},
        {SDLK_d, KeyEvent::Key_D},
        {SDLK_e, KeyEvent::Key_E},
        {SDLK_f, KeyEvent::Key_F},
        {SDLK_g, KeyEvent::Key_G},
        {SDLK_h, KeyEvent::Key_H},
        {SDLK_i, KeyEvent::Key_I},
        {SDLK_j, KeyEvent::Key_J},
        {SDLK_k, KeyEvent::Key_K},
        {SDLK_l, KeyEvent::Key_L},
        {SDLK_m, KeyEvent::Key_M},
        {SDLK_n, KeyEvent::Key_N},
        {SDLK_o, KeyEvent::Key_O},
        {SDLK_p, KeyEvent::Key_P},
        {SDLK_q, KeyEvent::Key_Q},
        {SDLK_r, KeyEvent::Key_R},
        {SDLK_s, KeyEvent::Key_S},
        {SDLK_t, KeyEvent::Key_T},
        {SDLK_u, KeyEvent::Key_U},
        {SDLK_v, KeyEvent::Key_V},
        {SDLK_w, KeyEvent::Key_W},
        {SDLK_x, KeyEvent::Key_X},
        {SDLK_y, KeyEvent::Key_Y},
        {SDLK_z, KeyEvent::Key_Z},

        {SDLK_UP, KeyEvent::Key_Up},
        {SDLK_LEFT, KeyEvent::Key_Left},
        {SDLK_RIGHT, KeyEvent::Key_Right},
        {SDLK_DOWN, KeyEvent::Key_Down},

        {SDLK_RETURN, KeyEvent::Key_Enter},
        {SDLK_KP_ENTER, KeyEvent::Key_Enter},
        {SDLK_SPACE, KeyEvent::Key_Space},
        {SDLK_BACKSPACE, KeyEvent::Key_Backspace},
        {SDLK_ESCAPE, KeyEvent::Key_Escape},
    });
    const std::unordered_map<SDL_Keycode, KeyEvent::Key>::const_iterator keyMapPos = keyMap.find(sdlEvent->key.keysym.sym);

    if (keyMapPos == keyMap.end()) {
        logd << "unknown key event " << SDL_GetScancodeName(sdlEvent->key.keysym.scancode) << std::endl;
        return;
    }

    KeyEvent keyEvent(type);
    keyEvent.initialize(keyMapPos->second, 0);
    m_surface->onEvent(&keyEvent);

}

inline SurfaceBackendImpl *SDLBackend::createSurface(Surface *surface)
{
    assert(surface); // Called with valid input

    assert(!m_surface); // there can be only one!
    assert(!m_window);
    assert(!m_gl);

    m_surface = surface;

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 0);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
    SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, 4);

    m_window = SDL_CreateWindow("rengine", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                1600, 1200, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN | SDL_WINDOW_ALLOW_HIGHDPI );

    m_gl = SDL_GL_CreateContext(m_window);
    SDL_GL_SetSwapInterval(1);

    requestRender();

    return this;
}

inline void SDLBackend::destroySurface(Surface */*surface*/, SurfaceBackendImpl */*impl*/)
{
    SDL_GL_DeleteContext(m_gl);
    SDL_DestroyWindow(m_window);
    m_gl = nullptr;
    m_window = nullptr;
    m_surface = nullptr;
}


inline bool SDLBackend::beginRender()
{
    assert(m_window);
    assert(m_surface);
    assert(m_gl);
    int error = SDL_GL_MakeCurrent(m_window, m_gl);
    if (error != 0) {
        logw << "SDL_GL_MakeCurrent failed: " << SDL_GetError();
        return false;
    }
    return true;
}

inline bool SDLBackend::commitRender()
{
    assert(m_window);
    assert(m_surface);
    assert(m_gl);
    SDL_GL_SwapWindow(m_window);
    return true;
}

inline Renderer *SDLBackend::createRenderer()
{
    assert(m_surface);
    assert(m_gl);
    OpenGLRenderer *r = new OpenGLRenderer();
    r->setTargetSurface(m_surface);
    return r;
}

inline void SDLBackend::requestRender()
{
    if (m_renderRequested)
        return;
    m_renderRequested = true;
    // we can't trigger the render synchronously. we need to give a chance
    // to process input, animations, whatever -- so push an event onto the
    // queue and we'll get back to this later.
    SDL_Event event;

    SDL_UserEvent renderev;
    renderev.type = SDL_USEREVENT;
    renderev.code = 0;
    renderev.data1 = this;
    renderev.data2 = NULL;

    event.type = SDL_USEREVENT;
    event.user = renderev;

    SDL_PushEvent(&event);
}

inline void SDLBackend::show()
{
    assert(m_window);
    assert(m_surface);
    SDL_ShowWindow(m_window);
}

inline void SDLBackend::hide()
{
    assert(m_window);
    assert(m_surface);
    SDL_HideWindow(m_window);
}

inline void SDLBackend::requestSize(vec2 size)
{
    assert(m_window);
    assert(m_surface);
    SDL_SetWindowSize(m_window, size.x, size.y);
}

inline vec2 SDLBackend::dpi() const
{
    assert(m_window);
    assert(m_surface);
    int index = SDL_GetWindowDisplayIndex(m_window);
    float d, h, v;
    SDL_GetDisplayDPI(index, &d, &h, &v);
    return vec2(h, v) * devicePixelRatio();
}

inline void SDLBackend::quit()
{
    // Wake up the event loop
    SDL_Event quitEvent;
    quitEvent.quit.type = SDL_QUIT;
    quitEvent.quit.timestamp = SDL_GetTicks();
    SDL_PushEvent(&quitEvent);
}

#define RENGINE_BACKEND rengine::SDLBackend

RENGINE_END_NAMESPACE
