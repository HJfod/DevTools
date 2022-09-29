#pragma once

#include "DevTools.hpp"
#include <imgui_internal.h>
#include "fonts/FeatherIcons.hpp"
#include "RTTI/gdrtti.hpp"
#include <Geode/utils/WackyGeodeMacros.hpp>
#include <Geode/utils/operators.hpp>

const char* getNodeName(CCObject* node) {
    return typeid(*node).name() + 6;
}

#define CHECK_IS(var, newName, type) \
    type* newName = nullptr; if ((newName = dynamic_cast<type*>(var)))

const char* hashlog(LogPtr* log) {
    return CCString::createWithFormat("##%p", log)->getCString();
}

ImVec2 toVec2(const CCPoint& a) {
    const auto size = ImGui::GetMainViewport()->Size;
    const auto winSize = CCDirector::sharedDirector()->getWinSize();
    return {
        a.x / winSize.width * size.x,
        (1.f - a.y / winSize.height) * size.y
    };
}

ImVec2 toVec2(const CCSize& a) {
    const auto size = ImGui::GetMainViewport()->Size;
    const auto winSize = CCDirector::sharedDirector()->getWinSize();
    return {
        a.width / winSize.width * size.x,
        -a.height / winSize.height * size.y
    };
}

void DevTools::hoverableNodeName(CCNode* node) {
    ImGui::Text("%s", getNodeName(node));
}

void DevTools::highlightNode(CCNode* node) {
	auto& foreground = *ImGui::GetForegroundDrawList();
	auto parent = node->getParent();
	auto bounding_box = node->boundingBox();
	CCPoint bb_min(bounding_box.getMinX(), bounding_box.getMinY());
	CCPoint bb_max(bounding_box.getMaxX(), bounding_box.getMaxY());

	auto cameraParent = node;
	while (cameraParent) {
		auto camera = cameraParent->getCamera();

		float off_x, off_y, off_z;
		camera->getEyeXYZ(&off_x, &off_y, &off_z);
		const CCPoint offset(off_x, off_y);
		bb_min -= offset;
		bb_max -= offset;

		cameraParent = cameraParent->getParent();
	}

	auto min = toVec2(parent ? parent->convertToWorldSpace(bb_min) : bb_min);
	auto max = toVec2(parent ? parent->convertToWorldSpace(bb_max) : bb_max);

    auto wsize = ImGui::GetMainViewport()->Size;
    auto rect = getGDWindowRect();

    auto tmin = ImVec2(
        min.x / wsize.x * rect.z + rect.x,
        min.y / wsize.y * rect.w + rect.y
    );
    auto tmax = ImVec2(
        max.x / wsize.x * rect.z + rect.x,
        max.y / wsize.y * rect.w + rect.y
    );

	foreground.AddRectFilled(
        tmin, tmax,
        m_selectedNode == node ?
            IM_COL32(200, 200, 255, 60) :
            IM_COL32(255, 255, 255, 70)
    );
}

void DevTools::recurseUpdateList(CCNode* node, unsigned int i) {
    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
    if (m_selectedNode == node) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }
    if (ImGui::TreeNodeEx(
        node, flags, "(%d) %s", i, getNodeName(node)
    )) {
        if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0)) {
            if (m_selectedNode == node) {
                this->selectNode(nullptr);
            } else {
                this->selectNode(node);
            }
        }
        if (
            m_selectedNode == node ||
            (ImGui::IsItemHovered() && ImGui::IsKeyDown(ImGuiKey_ModShift))
        ) {
            this->highlightNode(node);
        }
        CCARRAY_FOREACH_B_BASE(node->getChildren(), child, CCNode*, ix) {
            this->recurseUpdateList(dynamic_cast<CCNode*>(child), ix);
        }
        ImGui::TreePop();
    }
    else if (
        m_selectedNode == node ||
        (ImGui::IsItemHovered() && ImGui::IsKeyDown(ImGuiKey_ModShift))
    ) {
        this->highlightNode(node);
    }
}

void DevTools::recurseUpdateListOdd(CCNode* node, unsigned int i) {
    std::stringstream stream;

    stream << "<" << getNodeName(node);
    if (dynamic_cast<CCScene*>(node)) {
        stream << " type=" << node->getObjType();
    }
    if (node->getTag() != -1) {
        stream << " tag=" << node->getTag();
    }
    if (node->getPositionX() != 0.f) {
        stream << " x=" << node->getPositionX();
    }
    if (node->getPositionY() != 0.f) {
        stream << " y=" << node->getPositionY();
    }
    if (node->getScale() != 1.f) {
        stream << " scale=" << node->getScale();
    }
    stream << ">";

    if (ImGui::TreeNode(node, stream.str().c_str())) {
        CCARRAY_FOREACH_B_BASE(node->getChildren(), child, CCNode*, ix) {
            this->recurseUpdateListOdd(dynamic_cast<CCNode*>(child), ix);
        }
        ImGui::TreePop();
        ImGui::Text("</%s>", getNodeName(node));
    }
}

void DevTools::generateModInfo(Mod* mod) {
    if (ImGui::TreeNode(mod, mod->getName().c_str())) {
        ImGui::TextWrapped("Name: %s",         mod->getName().c_str());
        ImGui::TextWrapped("ID: %s",           mod->getID().c_str());
        ImGui::TextWrapped("Description: %s",  mod->getDescription().value_or("Not provided").c_str());
        ImGui::TextWrapped("Developer: %s",    mod->getDeveloper().c_str());

        if (ImGui::TreeNode(CCString::createWithFormat("Hooks: %d", mod->getHooks().size())->getCString())) {
            for (auto const& hook : mod->getHooks()) {
                if (ImGui::TreeNode(hook->getDisplayName().data())) {
                    ImGui::TextWrapped(
                        "Address: %d\n%s",
                        hook->getAddress(),
                        (hook->isEnabled() ? "Enabled" : "Not enabled")
                    );
                    if (hook->isEnabled()) {
                        if (ImGui::Button("Disable")) {
                            mod->disableHook(hook);
                        }
                    } else {
                        if (ImGui::Button("Enable")) {
                            mod->enableHook(hook);
                        }
                    }
                    ImGui::TreePop();
                }
            }
            ImGui::TreePop();
        }

        if (mod->supportsDisabling()) {
            ImGui::Button("Reload");
            ImGui::SameLine();
            ImGui::Button("Unload");
        }

        ImGui::TreePop();
    }
}

void DevTools::generateTree() {
    if (m_oddHtmlStyleSetting) {
        this->recurseUpdateListOdd(CCDirector::sharedDirector()->getRunningScene());
    } else {
        this->recurseUpdateList(CCDirector::sharedDirector()->getRunningScene());
    }
}

void DevTools::logMessage(LogPtr* log) {
    ImU32 color = 0;
    int level = 0;
    if (log->getSeverity() == Severity::Warning) {
        color = ImGui::ColorConvertFloat4ToU32(*m_colorWarning);
        color = (color & 0x00ffffff) + 0x27000000; // set alpha
        level = 1;
    }
    if (log->getSeverity() >= Severity::Error) {
        color = ImGui::ColorConvertFloat4ToU32(*m_colorNo);
        color = (color & 0x00ffffff) + 0x27000000; // set alpha
        level = 2;
    }
    ImGui::PushFont(m_monoFont);
    auto draw_list = ImGui::GetWindowDrawList();
    draw_list->ChannelsSplit(2);
    draw_list->ChannelsSetCurrent(1);
    ImGui::BeginGroup();
    {
        ImGui::SetNextItemWidth(30.f);
        if (level == 0) {
            ImGui::LabelText("", "");
        } else if (level == 1) {
            ImGui::LabelText("", U8(" " FEATHER_ALERT_TRIANGLE));
        } else if (level == 2) {
            ImGui::LabelText("", U8(" " FEATHER_ALERT_OCTAGON));
        }
        auto msgs = log->getData();
        if (!msgs.size()) {
            ImGui::SameLine();
            this->generateModInfo(log->getSender());
        } else {
            for (auto const& msg : msgs) {
                ImGui::SameLine();
                auto ccobject = dynamic_cast<CCObjectMeta*>(msg);
                if (ccobject) {
                    auto node = dynamic_cast<CCNode*>(ccobject->m_obj);
                    if (node) {
                        this->recurseUpdateList(node);
                    } else {
                        ImGui::Text(ccobject->m_repr.c_str());
                    }
                } else {
                    auto asmod = dynamic_cast<ModMeta*>(msg);
                    if (asmod) {
                        this->generateModInfo(asmod->m_mod);
                    } else {
                        ImGuiWindow* window = ImGui::GetCurrentWindow();
                        ImGui::PushTextWrapPos(ImGui::GetWindowWidth() - 20.f);
                        ImGui::TextWrapped(msg->m_repr.c_str());
                        ImGui::PopTextWrapPos();
                    }
                }
            }
        }
    }
    ImGui::EndGroup();
    draw_list->ChannelsSetCurrent(0);
    auto min = ImGui::GetItemRectMin();
    min.y -= ImGui::GetStyle().ItemSpacing.y;
    auto max = ImGui::GetItemRectMax();
    max.y += ImGui::GetStyle().ItemSpacing.y;
    max.x = ImGui::GetWindowPos().x + ImGui::GetWindowWidth();
    ImGui::GetWindowDrawList()->AddRectFilled(
        min, max, color
    );
    draw_list->ChannelsMerge();
    ImGui::PopFont();
    if (ImGui::IsMouseHoveringRect(min, max) && ImGui::BeginPopupContextWindow()) {
        if (ImGui::MenuItem(U8(FEATHER_TRASH_2 " Delete"))) {
            Loader::get()->popLog(log);
        }
        ImGui::EndPopup();
    }
    ImGui::Separator();
}

void DevTools::recurseGetParents(std::vector<CCNode*>& vec, CCNode* node) {
    if (node->getParent()) {
        this->recurseGetParents(vec, node->getParent());
    }
    vec.push_back(node);
}

template<>
void DevTools::generateTab<"Tree"_h>() {
    this->generateTree();
}

template<>
void DevTools::generateTab<"Console"_h>() {
    ImGui::SetCursorPosY(30);
    if (ImGui::BeginChild(
        0xB00B, { ImGui::GetWindowWidth(), ImGui::GetWindowHeight() - 85 }, true,
        ImGuiWindowFlags_HorizontalScrollbar
    )) {
        for (auto const& log : Loader::get()->getLogs()) {
            this->logMessage(log);
        }
    }
    if (m_lastLogCount != Loader::get()->getLogs().size()) {
        ImGui::SetScrollHereY(1.0f);
        m_lastLogCount = Loader::get()->getLogs().size();
    }
    ImGui::EndChild();

    ImGui::Separator();
    static char command_buf[255] = { 0 };
    if (m_commandSuccess) {
        memset(command_buf, 0, sizeof command_buf);
        m_commandSuccess = false;
    }
    ImGui::SetCursorPosX(10);
    ImGui::SetNextItemWidth(ImGui::GetWindowWidth() - 95);
    if (ImGui::InputText(
        "##dev.run_command", command_buf, IM_ARRAYSIZE(command_buf),
        ImGuiInputTextFlags_EnterReturnsTrue
    )) {
        this->executeConsoleCommand(command_buf);
        ImGui::SetKeyboardFocusHere();
    }
    ImGui::SameLine(ImGui::GetWindowWidth() - 70);
    if (ImGui::Button(U8(FEATHER_PLAY " Run"))) {
        this->executeConsoleCommand(command_buf);
    }
}

template<>
void DevTools::generateTab<"Mods"_h>() {
    this->generateModInfo(Loader::get()->getInternalMod());
    for (auto const& mod : Loader::get()->getAllMods()) {
        this->generateModInfo(mod);
    }
}

template<>
void DevTools::generateTab<"Class Data"_h>() {
    if (!m_selectedNode) {
        return ImGui::TextWrapped("Select a Node to Edit in the Scene or Tree");
    }
    ImGui::PushFont(m_monoFont);
    ImGui::TextWrapped("Address: 0x%p", m_selectedNode);
    ImGui::SameLine();
    if (ImGui::Button(U8(FEATHER_COPY " Copy"))) {
        clipboard::write(CCString::createWithFormat("%p", m_selectedNode)->getCString());
    }
   
    static int s_read_count_buf = 0xec;
    static int s_read_count = s_read_count_buf;
    ImGui::Text("Size: 0x");
    ImGui::SameLine();
    if (ImGui::InputInt(
        "##dev.class.read_count",
        &s_read_count_buf, 4, 0x20,
        ImGuiInputTextFlags_CharsHexadecimal |
        ImGuiInputTextFlags_EnterReturnsTrue
    )) {
        s_read_count_buf -= s_read_count_buf % 4;
        s_read_count = s_read_count_buf;
    }
   
    auto& rtti = GDRTTI::get();
    auto  info = rtti.read_rtti(m_selectedNode);
    if (info.size()) {
        auto base = info[0];
        info.erase(info.begin());
        if (ImGui::TreeNode(base.get_name().c_str())) {
            ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { .0f, .5f });
            std::vector<uint32_t> indent;
            indent.push_back(base.get_bases());
            for (auto const& r : info) {
                indent.push_back(r.get_bases());
                for (auto& i : indent) {
                    i--;
                }
                while (indent.back() <= 0) {
                    indent.pop_back();
                }
                ImGui::PushFont(m_boxFont);
                std::string is = " ";
                for (auto ix = 0u; ix < indent.size() - 1; ix++) {
                    if (indent[ix] > 1) {
                        is += U8(u8"\u2502 ");
                    } else {
                        is += U8(u8"  ");
                    }
                }
                if (indent.back() < r.get_bases() + 1) {
                    is += U8(u8"\u2514\u2500");
                } else {
                    is += U8(u8"\u251C\u2500");
                }
                ImGui::TextWrapped(is.c_str());
                ImGui::SameLine();
                ImGui::PopFont();
                ImGui::TextWrapped(
                    U8("[0x%x] %s"),
                    r.get_offset(),
                    r.get_name().c_str()
                );
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip(
                        "%s (+ 0x%x)",
                        r.get_mangled_name().c_str(),
                        r.get_offset()
                    );
                }
            }
            ImGui::PopStyleVar();
        }
    } else {
        ImGui::TextWrapped(U8(FEATHER_ALERT_TRIANGLE " No RTTI Found"));
    }
    
    for (int i = 0; i < s_read_count; i += 4) {
        auto addr = as<uintptr_t>(m_selectedNode) + i;
        if (rtti.valid(addr)) {
            auto data = *as<uintptr_t*>(addr);
            ImGui::Text(
                "+ %x : %x, %.6f",
                i, data, union_cast<float>(data)
            );
        } else {
            ImGui::Text(
                U8("+ %x : " FEATHER_ALERT_OCTAGON " <Unreadable Address>"), i
            );
        }
    }

    ImGui::PopFont();
}

template<>
void DevTools::generateTab<"Settings"_h>() {
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, { 0.f, 0.f });
    ImGui::Checkbox("GD in Window",             &m_GDInWindow);
    ImGui::Checkbox("Attributes in Node Tree",  &m_attributesInTree);
    ImGui::Checkbox("Weird XML Node Tree",      &m_oddHtmlStyleSetting);
    ImGui::Checkbox("Dock With Shift",          &ImGui::GetIO().ConfigDockingWithShift);
    ImGui::PopStyleVar();

    ImGui::Separator();

    static int selected_theme = static_cast<int>(m_theme);
    ImGui::PushItemWidth((ImGui::GetWindowWidth() - 10.f) / 2);
    if (ImGui::Combo("##dev.theme", &selected_theme,
        U8(
            FEATHER_SUN      " Light Theme\0"
            FEATHER_UMBRELLA " Dark Theme\0"
        )
    )) {
        m_theme = static_cast<DevToolsTheme>(selected_theme);
        this->reloadStyle();
    }

    ImGui::Separator();

    ImGui::TextWrapped(
        "Running Geode version %s",
        Loader::get()->getVersion().toString().c_str()
    );
}

template<>
void DevTools::generateTab<"Attributes"_h>() {
    if (!m_selectedNode) {
        return ImGui::TextWrapped("Select a Node to Edit in the Scene or Tree");
    }
    if (ImGui::Button("Deselect")) {
        return this->selectNode(nullptr);
    }
    ImGui::Text("Address: 0x%p", m_selectedNode);
    ImGui::SameLine();
    if (ImGui::Button(U8(FEATHER_COPY " Copy"))) {
        clipboard::write(CCString::createWithFormat("%X", as<uintptr_t>(m_selectedNode))->getCString());
    }
    if (m_selectedNode->getUserData()) {
        ImGui::Text("User data: 0x%p", m_selectedNode->getUserData());
    }

    float pos[2] = {
        m_selectedNode->getPositionX(),
        m_selectedNode->getPositionY()
    };
    ImGui::DragFloat2("Position", pos);
    m_selectedNode->setPosition(pos[0], pos[1]);

    float scale[3] = { m_selectedNode->getScale(), m_selectedNode->getScaleX(), m_selectedNode->getScaleY() };
    ImGui::DragFloat3("Scale", scale, 0.025f);
    if (m_selectedNode->getScale() != scale[0]) {
        m_selectedNode->setScale(scale[0]);
    } else {
        m_selectedNode->setScaleX(scale[1]);
        m_selectedNode->setScaleY(scale[2]);
    }

    float rot[3] = { m_selectedNode->getRotation(), m_selectedNode->getRotationX(), m_selectedNode->getRotationY() };
    ImGui::DragFloat3("Rotation", rot);
    if (m_selectedNode->getRotation() != rot[0]) {
        m_selectedNode->setRotation(rot[0]);
    } else {
        m_selectedNode->setRotationX(rot[1]);
        m_selectedNode->setRotationY(rot[2]);
    }

    float _skew[2] = { m_selectedNode->getSkewX(), m_selectedNode->getSkewY() };
    ImGui::DragFloat2("Skew", _skew);
    m_selectedNode->setSkewX(_skew[0]);
    m_selectedNode->setSkewY(_skew[1]);

    auto anchor = m_selectedNode->getAnchorPoint();
    ImGui::DragFloat2("Anchor Point", &anchor.x, 0.05f, 0.f, 1.f);
    m_selectedNode->setAnchorPoint(anchor);

    auto contentSize = m_selectedNode->getContentSize();
    ImGui::DragFloat2("Content Size", &contentSize.width);
    if (contentSize != m_selectedNode->getContentSize())
        m_selectedNode->setContentSize(contentSize);

    int zOrder = m_selectedNode->getZOrder();
    ImGui::InputInt("Z", &zOrder);
    if (m_selectedNode->getZOrder() != zOrder)
        m_selectedNode->setZOrder(zOrder);
    
    auto visible = m_selectedNode->isVisible();
    ImGui::Checkbox("Visible", &visible);
    if (visible != m_selectedNode->isVisible())
        m_selectedNode->setVisible(visible);

    if (dynamic_cast<CCRGBAProtocol*>(m_selectedNode) != nullptr) {
        auto rgbaNode = dynamic_cast<CCRGBAProtocol*>(m_selectedNode);
        auto color = rgbaNode->getColor();
        float _color[4] = { color.r / 255.f, color.g / 255.f, color.b / 255.f, rgbaNode->getOpacity() / 255.f };
        ImGui::ColorEdit4("Color", _color);
        rgbaNode->setColor({
            static_cast<GLubyte>(_color[0] * 255),
            static_cast<GLubyte>(_color[1] * 255),
            static_cast<GLubyte>(_color[2] * 255)
        });
        rgbaNode->setOpacity(static_cast<GLubyte>(_color[3] * 255));
    }
    if (dynamic_cast<CCLabelProtocol*>(m_selectedNode) != nullptr) {
        auto labelNode = dynamic_cast<CCLabelProtocol*>(m_selectedNode);
        auto labelStr = labelNode->getString();
        char text[256];
        strcpy_s(text, labelStr);
        ImGui::InputText("Text", text, 256);
        if (strcmp(text, labelStr)) {
            labelNode->setString(text);
        }
    }
}

template<>
void DevTools::generateTab<"Layout"_h>() {
    if (!m_selectedNode) {
        return ImGui::TextWrapped("Select a Node to Edit in the Scene or Tree");
    }
    std::vector<CCNode*> parents;
    this->recurseGetParents(parents, m_selectedNode);
    if (ImGui::BeginTable("dev.node.layout", 4)) {
        CCPoint accumulatedPos  = CCPointZero;
        float   accumulatedRot  = 0.f;
        float   accumulatedScale= 1.f;
        for (auto const& node : parents) {
            accumulatedPos   += node->getPosition();
            accumulatedRot   += node->getRotation();
            accumulatedScale *= node->getScale();
            ImGui::TableNextRow();
            ImGui::TableNextColumn();
            this->hoverableNodeName(node);
            ImGui::TableNextColumn();
            if (node->getPosition() != CCPointZero) {
                ImGui::PushStyleColor(ImGuiCol_Text, *m_colorWarning);
            }
            ImGui::Text(
                "%.2f, %.2f (+ %.2f, %.2f)",
                accumulatedPos.x,
                accumulatedPos.y,
                node->getPositionX(),
                node->getPositionY()
            );
            ImGui::PopStyleColor(node->getPosition() != CCPointZero);
            ImGui::TableNextColumn();
            if (node->getRotation() != 0.f) {
                ImGui::PushStyleColor(ImGuiCol_Text, *m_colorWarning);
            }
            ImGui::Text(
                "%.2f° (+ %.2f°)",
                accumulatedRot,
                node->getRotation()
            );
            ImGui::PopStyleColor(node->getRotation() != 0.f);
            ImGui::TableNextColumn();
            if (node->getScale() != 1.f) {
                ImGui::PushStyleColor(ImGuiCol_Text, *m_colorWarning);
            }
            ImGui::Text(
                "%.2fx (* %.2fx)",
                accumulatedScale,
                node->getScale()
            );
            ImGui::PopStyleColor(node->getScale() != 1.f);
        }
        ImGui::EndTable();
    }
}

void DevTools::generateTabs() {
    if (!ImGui::DockBuilderGetNode(m_dockSpaceID)) {
        // m_dockSpaceID = ImGui::GetID("dev.dockspace");
        // ImGui::DockBuilderRemoveNode(m_dockSpaceID);
        // ImGui::DockBuilderAddNode(m_dockSpaceID,
        //     ImGuiDockNodeFlags_DockSpace | ImGuiDockNodeFlags_PassthruCentralNode
        // );
    }

    #define MAKE_TAB(_icon_, _name_)            \
        if (ImGui::Begin(U8(_icon_ " " _name_))) {  \
            this->generateTab<_name_##_h>();    \
        }                                       \
        ImGui::End();                           \

    MAKE_TAB(FEATHER_GIT_MERGE, "Tree");
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, { 0, 0 });
    MAKE_TAB(FEATHER_TERMINAL,  "Console");
    ImGui::PopStyleVar();
    MAKE_TAB(FEATHER_PACKAGE,   "Mods");
    MAKE_TAB(FEATHER_CPU,       "Class Data");
    MAKE_TAB(FEATHER_SETTINGS,  "Settings");
    MAKE_TAB(FEATHER_EDIT,      "Attributes");
    MAKE_TAB(FEATHER_LAYOUT,    "Layout");
}
