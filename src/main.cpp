#include <Geode.hpp>
#include <GeodeAPI.hpp>
#include "DevTools.hpp"

USE_GEODE_NAMESPACE();

GEODE_API bool GEODE_CALL geode_load(Mod* mod) {
	Interface::get()->init(mod);
    
    mod->with<GeodeAPI>()->addKeybindAction(TriggerableAction {
        "Open Dev Tools",
        "dev_tools.open",
        KB_GLOBAL_CATEGORY,
        [](CCNode* node, bool down) -> bool {
            if (down) {
                DevTools::get()->toggle();
            }
            return false;
        }
    }, {{ KEY_I, Keybind::Modifiers::Control | Keybind::Modifiers::Shift }});

    return true;
}
