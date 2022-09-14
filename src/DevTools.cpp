#define geode_INCLUDE_IMGUI
#include "DevTools.hpp"
#include <limits>
#include "ArgParser.hpp"

#include "fonts/OpenSans.hpp"
#include "fonts/GeodeIcons.hpp"
#include "fonts/FeatherIcons.hpp"
#include "fonts/RobotoMono.hpp"
#include "fonts/SourceCodeProLight.hpp"
#undef max

template<typename T, typename O>
constexpr T enum_cast(O o) {
    return static_cast<T>(reinterpret_cast<int>(o));
}

bool DevTools::isVisible() const {
    return m_visible;
}

bool DevTools::shouldPopGame() const {
    return m_visible && m_GDInWindow;
}

void DevTools::draw() {
    if (m_visible) {
        auto& style = ImGui::GetStyle();
        style.ColorButtonPosition = ImGuiDir_Left;

        this->loadStyle();

        ImGuiWindowFlags flags = 
            ImGuiWindowFlags_NoScrollbar;
        
        m_dockSpaceID = ImGui::DockSpaceOverViewport(
            nullptr, ImGuiDockNodeFlags_PassthruCentralNode
        );
        auto& fonts = ImGui::GetIO().Fonts->Fonts;
        ImGui::PushFont(m_defaultFont);
        this->generateTabs();
        ImGui::PopFont();
        this->fixSceneScale(CCDirector::sharedDirector()->getRunningScene());
    }
}

void DevTools::initFonts() {
    static const ImWchar icon_ranges[] = { FEATHER_MIN_FA, FEATHER_MAX_FA, 0 };
    static const ImWchar box_ranges[]  = { BOX_DRAWING_MIN_FA, BOX_DRAWING_MAX_FA, 0 };
    static const ImWchar* def_ranges   = ImGui::GetIO().Fonts->GetGlyphRangesDefault();
    
    static const auto add_font = +[](
        ImFont** member, void* font, float size, const ImWchar* range
    ) -> void {
        auto& io = ImGui::GetIO();
        ImFontConfig config;
        config.MergeMode = true;
        *member = io.Fonts->AddFontFromMemoryTTF(
            font, sizeof font, size, nullptr, range
        );
        io.Fonts->AddFontFromMemoryTTF(
            Font_FeatherIcons, sizeof Font_FeatherIcons, size - 4.f, &config, icon_ranges
        );
        io.Fonts->Build();
    };

    add_font(&m_defaultFont, Font_OpenSans,           18.f, def_ranges);
    add_font(&m_smallFont,   Font_OpenSans,           10.f, def_ranges);
    add_font(&m_monoFont,    Font_RobotoMono,         18.f, def_ranges);
    add_font(&m_boxFont,     Font_SourceCodeProLight, 23.f, box_ranges);
}

DevTools::DevTools() {
    m_colorYes     = new ImVec4;
    m_colorNo      = new ImVec4;
    m_colorWarning = new ImVec4;
    // m_addresses      = new AddressManager;
}

DevTools::~DevTools() {
    delete m_colorYes;
    delete m_colorNo;
    delete m_colorWarning;
    // delete m_addresses;
}

DevTools* DevTools::get() {
    static auto g_dev = new DevTools;
    return g_dev;
}

class AccessSpecifiersAreForNerds : public CCTransitionScene {
public:
    CCScene* getIn()  { return m_pInScene; }
    CCScene* getOut() { return m_pOutScene; }
};

void DevTools::selectNode(CCNode* node) {
    CC_SAFE_RELEASE(m_selectedNode);
    m_selectedNode = node;
    CC_SAFE_RETAIN(m_selectedNode);
}

void DevTools::reloadStyle() {
    m_loadedStyle = false;
    this->loadStyle();
}

void DevTools::fixSceneScale(CCScene* scene) {
    auto t = as<AccessSpecifiersAreForNerds*>(
        dynamic_cast<CCTransitionScene*>(scene)
    );
    if (t) {
        scene = t->getIn();
    }
}

void DevTools::willSwitchToScene(CCScene* scene) {
    this->selectNode(nullptr);
    this->fixSceneScale(scene);
}

void DevTools::show() {
    if (!m_visible) {
        auto scene = CCDirector::sharedDirector()->getRunningScene();
        m_visible = true;
    }
}

void DevTools::hide() {
    if (m_visible) {
        auto scene = CCDirector::sharedDirector()->getRunningScene();
        m_visible = false;
    }
}

void DevTools::toggle() {
    if (m_visible) {
        this->hide();
    } else {
        this->show();
    }
}

void split_in_args(std::vector<std::string>& qargs, std::string command){
    int len = command.length();
    bool qot = false, sqot = false;
    int arglen;
    for(int i = 0; i < len; i++) {
            int start = i;
            if(command[i] == '\"') {
                    qot = true;
            }
            else if(command[i] == '\'') sqot = true;

            if(qot) {
                    i++;
                    start++;
                    while(i<len && command[i] != '\"')
                            i++;
                    if(i<len)
                            qot = false;
                    arglen = i-start;
                    i++;
            }
            else if(sqot) {
                    i++;
                    while(i<len && command[i] != '\'')
                            i++;
                    if(i<len)
                            sqot = false;
                    arglen = i-start;
                    i++;
            }
            else{
                    while(i<len && command[i]!=' ')
                            i++;
                    arglen = i-start;
            }
            qargs.push_back(command.substr(start, arglen));
    }
}

CCScene* createSceneByLayerName(std::string const& name, void* param = nullptr) {
    auto scene = CCScene::create();
    switch (hash(name.c_str())) {
        case "MenuLayer"_h:
            scene->addChild(MenuLayer::create());
            break;

        case "GJGarageLayer"_h:
            scene->addChild(GJGarageLayer::create());
            break;

        case "LevelSelectLayer"_h:
            scene->addChild(LevelSelectLayer::create(as<int>(param)));
            break;

        case "LevelBrowserLayer"_h:
            scene->addChild(LevelBrowserLayer::create(GJSearchObject::create(enum_cast<SearchType>(param))));
            break;

        default:
            CC_SAFE_RELEASE(scene);
            return nullptr;
    }
    return scene;
}

void DevTools::executeConsoleCommand(std::string const& cmd) {
    if (!cmd.size()) return;

    auto parser = ArgParser().parse(cmd);

    if (!parser) {
        Log::get()
            << Severity::Error
            << "Error Parsing Command: " << parser.error();
        return;
    }
    
    auto args = parser.value();

    SWITCH_ARGS {
        HANDLER("goto") {
            if (!args.hasArg(1)) {
                Mod::get()->logInfo(
                    "Invalid Command: \"goto\" requires a second parameter of type SceneID",
                    Severity::Error
                );
            } else {
                auto scene = createSceneByLayerName(
                    args.at(1), as<void*>(std::stoi(args.at(2, "0")))
                );
                if (scene) {
                    Log::get() << "Moving to scene " << args.at(1);
                    CCDirector::sharedDirector()->replaceScene(CCTransitionFade::create(.5f, scene));
                    m_commandSuccess = true;
                } else {
                    Log::get() << Severity::Error <<
                        "Invalid Command: \"" << args.at(1) << "\" is not a valid SceneID";
                }
            }
        }
    
        SWITCH_SUB("test") {
            HANDLER("warn") {
                Log::get()
                    << Severity::Warning
                    << "Example warning";
            }

            UNKNOWN_HANDLER();
        }
    
        UNKNOWN_HANDLER();
    }
}
