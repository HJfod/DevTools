#include "../GLEW/glew.h"
#include "DevTools.hpp"
#include <windowsx.h>
#include <Geode/Modify.hpp>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
static bool g_renderInSwapBuffers = false;
static bool g_shouldPassEventsToGDButTransformed = false;
static bool g_updateBuffer = false;
static ImVec4 g_GDWindowRect;

ImVec4 getGDWindowRect() {
    return g_GDWindowRect;
}

ImVec2 operator-(ImVec2 const& v1, ImVec2 const& v2) {
    return { v1.x - v2.x, v1.y - v2.y };
}

ImVec2 operator*(ImVec2 const& v1, float multi) {
    return { v1.x * multi, v1.y * multi };
}

bool operator!=(ImVec2 const& v1, ImVec2 const& v2) {
    return v1.x == v2.x && v1.y == v2.y;
}

class $modify(CCEGLView) {
    virtual void swapBuffers() {
        static bool g_init = [this]() -> bool {
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            auto& io = ImGui::GetIO();
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
            io.ConfigDockingWithShift = true;
            // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
            DevTools::get()->initFonts();
            auto hwnd = WindowFromDC(*as<HDC*>(as<uintptr_t>(this->getWindow()) + 0x244));
            ImGui_ImplWin32_Init(hwnd);
            ImGui_ImplOpenGL3_Init();

            return true;
        }();

        if (g_renderInSwapBuffers) {
            ImGui_ImplOpenGL3_NewFrame();
            ImGui_ImplWin32_NewFrame();
            ImGui::NewFrame();

            DevTools::get()->draw();

            ImGui::EndFrame();
            ImGui::Render();

            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

            glFlush();
        }

        CCEGLView::swapBuffers();
    }

    void updateWindow(int width, int height) {
        g_updateBuffer = true;
        CCEGLView::updateWindow(width, height);
    }

    void pollEvents() {
        auto& io = ImGui::GetIO();

        bool blockInput = false;
        MSG msg;
        while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);

            if (io.WantCaptureMouse) {
                switch(msg.message) {
                    case WM_LBUTTONDBLCLK:
                    case WM_LBUTTONDOWN:
                    case WM_LBUTTONUP:
                    case WM_MBUTTONDBLCLK:
                    case WM_MBUTTONDOWN:
                    case WM_MBUTTONUP:
                    case WM_MOUSEACTIVATE:
                    case WM_MOUSEHOVER:
                    case WM_MOUSEHWHEEL:
                    case WM_MOUSELEAVE:
                    case WM_MOUSEMOVE:
                    case WM_MOUSEWHEEL:
                    case WM_NCLBUTTONDBLCLK:
                    case WM_NCLBUTTONDOWN:
                    case WM_NCLBUTTONUP:
                    case WM_NCMBUTTONDBLCLK:
                    case WM_NCMBUTTONDOWN:
                    case WM_NCMBUTTONUP:
                    case WM_NCMOUSEHOVER:
                    case WM_NCMOUSELEAVE:
                    case WM_NCMOUSEMOVE:
                    case WM_NCRBUTTONDBLCLK:
                    case WM_NCRBUTTONDOWN:
                    case WM_NCRBUTTONUP:
                    case WM_NCXBUTTONDBLCLK:
                    case WM_NCXBUTTONDOWN:
                    case WM_NCXBUTTONUP:
                    case WM_RBUTTONDBLCLK:
                    case WM_RBUTTONDOWN:
                    case WM_RBUTTONUP:
                    case WM_XBUTTONDBLCLK:
                    case WM_XBUTTONDOWN:
                    case WM_XBUTTONUP:
                        blockInput = true;
                }
            }

            auto msgToGD = msg;
            if (g_shouldPassEventsToGDButTransformed && msg.message == WM_MOUSEMOVE) {
                auto win = ImGui::GetMainViewport()->Size;
                auto mpos = ImVec2(
                    GET_X_LPARAM(msg.lParam) - g_GDWindowRect.x,
                    GET_Y_LPARAM(msg.lParam) - g_GDWindowRect.y
                );
                auto x = (mpos.x / g_GDWindowRect.z) * win.x;
                auto y = (mpos.y / g_GDWindowRect.w) * win.y;
                msgToGD.lParam = MAKELPARAM(x, y);
            }

            if (io.WantCaptureKeyboard) {
                switch(msg.message) {
                    case WM_HOTKEY:
                    case WM_KEYDOWN:
                    case WM_KEYUP:
                    case WM_KILLFOCUS:
                    case WM_SETFOCUS:
                    case WM_SYSKEYDOWN:
                    case WM_SYSKEYUP:
                        blockInput = true;
                }
            }

            if (g_shouldPassEventsToGDButTransformed) {
                blockInput = false;
            }

            if (!blockInput) {
                DispatchMessage(&msgToGD);
            }
            ImGui_ImplWin32_WndProcHandler(msg.hwnd, msg.message, msg.wParam, msg.lParam);
        }

        CCEGLView::pollEvents();
    }
};

class $modify(CCDirector) {
    void drawScene() {
        static GLuint s_buffer  = 0;
        static GLuint s_texture = 0;
        static GLuint s_depth   = 0;
        static auto s_free = +[]() -> void {
            if (s_depth) {
                glDeleteRenderbuffers(1, &s_depth);
                s_depth = 0;
            }
            if (s_texture) {
                glDeleteTextures(1, &s_texture);
                s_texture = 0;
            }
            if (s_buffer) {
                glDeleteFramebuffers(1, &s_buffer);
                s_buffer = 0;
            }
        };

        if (!DevTools::get()->shouldPopGame()) {
            s_free();
            g_renderInSwapBuffers = true;
            g_shouldPassEventsToGDButTransformed = false;
            return CCDirector::drawScene();
        }
        g_renderInSwapBuffers = false;

        if (g_updateBuffer) {
            s_free();
            g_updateBuffer = false;
        }

        auto winSize = this->getOpenGLView()->getViewPortRect();

        if (!s_buffer) {
            glGenFramebuffers(1, &s_buffer);
            glBindFramebuffer(GL_FRAMEBUFFER, s_buffer);
        }

        if (!s_texture) {
            glGenTextures(1, &s_texture);
            glBindTexture(GL_TEXTURE_2D, s_texture);

            glTexImage2D(
                GL_TEXTURE_2D, 0,GL_RGB,
                static_cast<GLsizei>(winSize.size.width),
                static_cast<GLsizei>(winSize.size.height),
                0,GL_RGB, GL_UNSIGNED_BYTE, 0
            );

            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        }

        if (!s_depth) {
            glGenRenderbuffers(1, &s_depth);
            glBindRenderbuffer(GL_RENDERBUFFER, s_depth);
            glRenderbufferStorage(
                GL_RENDERBUFFER, GL_DEPTH_COMPONENT,
                static_cast<GLsizei>(winSize.size.width),
                static_cast<GLsizei>(winSize.size.height)
            );
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, s_depth);

            glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, s_texture, 0);
        }

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            log::error("Unable to Render to Framebuffer");
            s_free();
            CCDirector::drawScene();
            return;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, s_buffer);

        CCDirector::drawScene();

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame();

        glClear(0x4100);

        DevTools::get()->draw();

        // if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        //     auto backup_current_context = this->getOpenGLView()->getWindow();
        //     ImGui::UpdatePlatformWindows();
        //     ImGui::RenderPlatformWindowsDefault();
        //     glfwMakeContextCurrent(backup_current_context);
        // }
        
        glFlush();

        if (ImGui::Begin("Geometry Dash")) {
            auto ratio = winSize.size.width / winSize.size.height;
            ImVec2 imgSize = {
                (ImGui::GetWindowHeight() - 35) * ratio,
                (ImGui::GetWindowHeight() - 35)
            };
            if (ImGui::GetWindowWidth() - 20 < imgSize.x) {
                imgSize = {
                    (ImGui::GetWindowWidth() - 20),
                    (ImGui::GetWindowWidth() - 20) / ratio
                };
            }
            auto pos = (ImGui::GetWindowSize() - imgSize) * .5f;
            pos.y += 10.f;
            ImGui::SetCursorPos(pos);
            ImGui::Image(as<ImTextureID>(s_texture),
                imgSize, { 0, 1 }, { 1, 0 }
            );
            g_GDWindowRect = {
                ImGui::GetWindowPos().x + pos.x,
                ImGui::GetWindowPos().y + pos.y,
                imgSize.x, imgSize.y
            };
            g_shouldPassEventsToGDButTransformed = ImGui::IsItemHovered();
        }
        ImGui::End();

        ImGui::EndFrame();
        ImGui::Render();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
};
