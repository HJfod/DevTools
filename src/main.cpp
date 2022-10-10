#include <Geode/Geode.hpp>
#include "DevTools.hpp"
#include <Geode/Modify.hpp>

USE_GEODE_NAMESPACE();

// todo: use shortcuts api once Geode API has those
class $modify(CCKeyboardDispatcher) {
    bool dispatchKeyboardMSG(enumKeyCodes key, bool down) {
        if (down && key == KEY_F11) {
            DevTools::get()->toggle();
            return true;
        }
        return CCKeyboardDispatcher::dispatchKeyboardMSG(key, down);
    }
};
