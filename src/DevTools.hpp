#pragma once

#include <Geode.hpp>
#include <imgui.h>
#include <backends/imgui_impl_win32.h>
#include <backends/imgui_impl_opengl3.h>
#include "RTTI/address.hpp"

USE_GEODE_NAMESPACE();

enum class DevToolsTheme {
    Light,
    Dark,
};

class DevTools {
    protected:
        bool m_visible         = false;
        bool m_loadedStyle     = false;
        bool m_GDInWindow      = true;
        bool m_attributesInTree= false;
        bool m_commandSuccess  = false;
        size_t m_lastLogCount  = 0;
        bool m_oddHtmlStyleSetting=false;
        DevToolsTheme m_theme  = DevToolsTheme::Dark;
        ImFont* m_defaultFont  = nullptr;
        ImFont* m_smallFont    = nullptr;
        ImFont* m_monoFont     = nullptr;
        ImFont* m_boxFont      = nullptr;
        ImVec4* m_colorNo      = nullptr;
        ImVec4* m_colorYes     = nullptr;
        ImVec4* m_colorWarning = nullptr;
        CCNode* m_selectedNode = nullptr;
        ImGuiID m_dockSpaceID  = 0;
        // AddressManager* m_addresses = nullptr;

        void executeConsoleCommand(std::string const&);
        
        void initFonts();
        void loadStyle();
        void reloadStyle();
        void loadTheme(DevToolsTheme theme);

        void recurseUpdateList(CCNode* parent, unsigned int = 0);
        void recurseUpdateListOdd(CCNode* parent, unsigned int = 0);
        void recurseGetParents(std::vector<CCNode*>& vec, CCNode* node);
        void logMessage(LogMessage* msg);
        void generateModInfo(Mod* mod);
        void hoverableNodeName(CCNode* node);
        void generateTree();
        void generateTabs();
        template<int TabID>
        void generateTab();
        void addressData(uintptr_t address);
        void selectNode(CCNode* node);

        void draw();
        
        friend void CCEGLView_swapBuffers(CCEGLView*);
        friend void CCDirector_drawScene(CCDirector*);

        DevTools();
        ~DevTools();

    public:
        static DevTools* get();

        void fixSceneScale(CCScene* scene);
        void willSwitchToScene(CCScene* scene);

        bool isVisible() const;
        bool shouldPopGame() const;
        void show();
        void hide();
        void toggle();
};
