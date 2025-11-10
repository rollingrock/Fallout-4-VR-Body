#include "FRIK.h"

#include "Config.h"
#include "Debug.h"
#include "GameHooks.h"
#include "PapyrusApi.h"
#include "PapyrusGateway.h"
#include "utils.h"
#include "common/Logger.h"
#include "config-mode/ConfigurationMode.h"
#include "f4vr/F4VRUtils.h"
#include "f4vr/VRControllersManager.h"
#include "pipboy/Pipboy.h"
#include "skeleton/HandPose.h"
#include "skeleton/Skeleton.h"
#include "smooth-movement/SmoothMovementVR.h"
#include "ui/UIManager.h"
#include "ui/UIModAdapter.h"

using namespace common;

// This is the entry point to the mod.
extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface* a_skse, F4SE::PluginInfo* a_info)
{
    return f4cf::g_mod->onF4SEPluginQuery(a_skse, a_info);
}

// This is the entry point to the mod.
extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* a_f4se)
{
    return f4cf::g_mod->onF4SEPluginLoad(a_f4se);
}

namespace frik
{
    /**
     * TODO: think about it, is it the best way to handle this dependency indirection.
     */
    class FrameUpdateContext : public vrui::UIModAdapter
    {
    public:
        explicit FrameUpdateContext(Skeleton* skelly) :
            _skelly(skelly) {}

        virtual RE::NiPoint3 getInteractionBoneWorldPosition() override
        {
            return _skelly->getIndexFingerTipWorldPosition(false);
        }

        virtual void fireInteractionHeptic() override
        {
            f4vr::VRControllers.triggerHaptic(f4vr::Hand::Offhand);
        }

        virtual void setInteractionHandPointing(const bool primaryHand, const bool toPoint) override
        {
            setForceHandPointingPose(primaryHand, toPoint);
        }

    private:
        Skeleton* _skelly;
    };

    /**
     * On mod loaded by F4SE.
     */
    void FRIK::onModLoaded(const F4SE::LoadInterface*)
    {
        std::srand(static_cast<unsigned int>(time(nullptr)));

        logger::info("Run patches...");
        hook::patchAll();

        logger::info("Hook main...");
        hook::hookMain();
    }

    /**
     * On game fully loaded initialize things that should be initialized only once.
     */
    void FRIK::onGameLoaded()
    {
        g_config.isFalloutLondonVR = isFalloutLondonVRModLoaded();
        if (g_config.isFalloutLondonVR) {
            logger::info("Fallout London VR mod detected, enabling compatibility mode...");
        }

        logger::info("Register papyrus native functions...");
        initPapyrusApis();
        PapyrusGateway::init();
        _boneSpheres.init();

        _gameMenusHandler.init([this](const std::string&, const bool isOpened) {
            if (isOpened) {
                onGameMenuOpened();
            }
        });

        removeEmbeddedFlashlight();

        if (isBetterScopesVRModLoaded()) {
            logger::info("BetterScopesVR mod detected, registering for messages...");
            _messaging->Dispatch(15, static_cast<void*>(nullptr), sizeof(bool), BETTER_SCOPES_VR_MOD_NAME);
            _messaging->RegisterListener(onBetterScopesMessage, BETTER_SCOPES_VR_MOD_NAME);
        }
    }

    /**
     * Game session can be initialized multiple times as it is fired on new game and save loaded events.
     * We should clear and reload as much of the game state as we can.
     */
    void FRIK::onGameSessionLoaded()
    {
        if (_skelly) {
            logger::info("Resetting skeleton for new game session...");
            releaseSkeleton();
            logger::info("Reload config...");
            g_config.load();
        }

        configureGameVars();
    }

    /**
     * Called on every game frame via hooks into the game engine.
     * This is where all the magic happens by updating game state and nodes.
     */
    void FRIK::onFrameUpdate()
    {
        if (!RE::PlayerCharacter::GetSingleton()) {
            // game not loaded or existing
            return;
        }

        if (_skelly) {
            if (!isRootNodeValid()) {
                logger::warn("Root node released, reset skelly... PowerArmorChange?({})", _inPowerArmor != f4vr::isInPowerArmor());
                releaseSkeleton();
            } else if (_inPowerArmor != f4vr::isInPowerArmor()) {
                logger::info("Power Armor state changed, reset skeleton...");
                releaseSkeleton();
            }
        }

        if (!_skelly) {
            if (!isGameReadyForSkeletonInitialization()) {
                return;
            }

            initSkeleton();
        }

        logger::trace("Update Skeleton...");
        _skelly->onFrameUpdate();

        logger::trace("Update Bone Sphere...");
        _boneSpheres.onFrameUpdate();

        logger::trace("Update Weapon Position...");
        _weaponPosition->onFrameUpdate();

        logger::trace("Update Pipboy...");
        _pipboy->onFrameUpdate();

        _configurationMode->onFrameUpdate();

        FrameUpdateContext context(_skelly);
        vrui::g_uiManager->onFrameUpdate(&context);

        _playerControlsHandler.onFrameUpdate(_pipboy, _weaponPosition, &_gameMenusHandler);

        updateWorldFinal();
    }

    void FRIK::smoothMovement()
    {
        try {
            _smoothMovement.onFrameUpdate();
        } catch (const std::exception& e) {
            logger::error("Error in FRIK::smoothMovement: {}", e.what());
        }
    }

    void FRIK::initSkeleton()
    {
        _inPowerArmor = f4vr::isInPowerArmor();

        const auto player = f4vr::getPlayer();
        logger::info("Initialize Skeleton ({}) ; Nodes: Player={}, Data={}, Root={}, Skeleton={}, Common={}",
            _inPowerArmor ? "PowerArmor" : "Regular",
            static_cast<const void*>(player),
            static_cast<const void*>(player->unkF0),
            static_cast<const void*>(player->unkF0->rootNode),
            static_cast<const void*>(f4vr::getRootNode()),
            static_cast<const void*>(f4vr::getCommonNode()));

        // init skeleton
        _workingRootNode = f4vr::getRootNode();
        _skelly = new Skeleton(f4vr::getRootNode(), _inPowerArmor);

        // init handlers depending on skeleton
        _pipboy = new Pipboy(_skelly);
        _configurationMode = new ConfigurationMode(_skelly);
        _weaponPosition = new WeaponPositionAdjuster(_skelly);
    }

    /**
     * Check if game all nodes exist and ready for skeleton handling flow.
     * Based on random crashes and the objects that were missing.
     * Probably not all checks are required, but it's cheap and only happens when skeleton is not initialized.
     */
    bool FRIK::isGameReadyForSkeletonInitialization()
    {
        const auto player = f4vr::getPlayer();
        if (!player || !player->unkF0) {
            logger::sample(3000, "Player global not set yet!");
            return false;
        }
        if (!player->unkF0->rootNode || !f4vr::getRootNode() || !f4vr::getWorldRootNode()) {
            logger::sample("Player root nodes not set yet!");
            return false;
        }
        if (!f4vr::getCommonNode() || !f4vr::getPlayerNodes() || !f4vr::getFlattenedBoneTree()) {
            logger::sample("Common or Player nodes not set yet!");
            return false;
        }
        if (!f4vr::findNode(f4vr::getFirstPersonSkeleton(), "RArm_Hand")) {
            logger::sample("Arm node not set yet!");
            return false;
        }
        if (!f4vr::getWeaponNode()) {
            logger::sample("Weapon node not set yet!");
            return false;
        }
        const auto camera = f4vr::getPlayerCamera();
        if (!camera || !camera->cameraNode) {
            logger::sample("Camera node not set yet!");
            return false;
        }
        return true;
    }

    /**
     * The game can change the basic root object under us.
     * It doesn't happen often but when it does, we should reinitialize the skeleton.
     * Known root release: entering/exiting power armor, after character creation in new game.
     */
    bool FRIK::isRootNodeValid() const
    {
        if (!_workingRootNode) {
            return false;
        }
        if (_workingRootNode != f4vr::getRootNode()) {
            return false;
        }
        if (_workingRootNode->parent == nullptr) {
            return false;
        }
        return true;
    }

    /**
     * On game menu open check is loading menu is open and reset skelly if it does.
     * We want to reinit skeleton after player moves to a new location by fast travel or other.
     */
    void FRIK::onGameMenuOpened()
    {
        if (_skelly && _gameMenusHandler.isLoadingMenuOpen()) {
            logger::info("Loading menu is open, reset skeleton...");
            releaseSkeleton();
        }
    }

    /**
     * On switch from normal and power armor, reset the skelly and all dependencies with persistent data.
     */
    void FRIK::releaseSkeleton()
    {
        _workingRootNode = nullptr;

        delete _skelly;
        _skelly = nullptr;

        delete _pipboy;
        _pipboy = nullptr;

        delete _configurationMode;
        _configurationMode = nullptr;

        delete _weaponPosition;
        _weaponPosition = nullptr;

        _inPowerArmor = false;
        _dynamicCameraHeight = false;
    }

    /**
     * Calling three engine-level functions to update the scene graph state for the player's root node and its children,
     * specifically related to geometry bounds, skeletal bone transforms, and flattened tree data.
     * Without it some cull geometry, Pipboy interaction, and hand fingers position may not work.
     */
    void FRIK::updateWorldFinal()
    {
        const auto worldRootNode = f4vr::getWorldRootNode();
        f4vr::BSFadeNode_MergeWorldBounds(worldRootNode);
        f4vr::BSFlattenedBoneTree_UpdateBoneArray(f4vr::getRootNode());
        // just in case any transforms missed because they are not in the tree do a full flat bone array update
        f4vr::BSFadeNode_UpdateGeomArray(worldRootNode, 1);
    }

    void FRIK::configureGameVars()
    {
        logger::info("Setting VRScale from:({:.3f}) to:({:.3f})", f4vr::getIniSetting("fVrScale:VR")->GetFloat(), g_config.fVrScale);
        f4vr::getIniSetting("fVrScale:VR", true)->SetFloat(g_config.fVrScale);

        f4vr::getIniSetting("fPipboyMaxScale:VRPipboy", true)->SetFloat(3.0000);
        f4vr::getIniSetting("fPipboyMinScale:VRPipboy", true)->SetFloat(0.0100f);
        f4vr::getIniSetting("fVrPowerArmorScaleMultiplier:VR", true)->SetFloat(1.0000);
    }

    /**
     * If to remove the embedded FRIK flashlight from the game.
     * Useful for players to be able to install other flashlight mods.
     */
    void FRIK::removeEmbeddedFlashlight()
    {
        if (!g_config.removeFlashlight) {
            return;
        }
        for (const auto& item : RE::TESDataHandler::GetSingleton()->GetFormArray<RE::TESObjectARMO>()) {
            if (item->formID == 0x21B3B) {
                for (std::uint32_t idx{}; idx < item->numKeywords; idx++) {
                    const auto frmKey = item->keywords[idx];
                    if (frmKey && frmKey->formID == 0xB34A6) {
                        logger::info("Removing embedded FRIK flashlight from: '{}', keyword: 0x{:x}", item->GetFullName(), frmKey->formID);
                        item->RemoveKeyword(frmKey);
                        return;
                    }
                }
                logger::warn("Failed to remove embedded FRIK flashlight, keyword not found in '{}'", item->GetFullName());
                return;
            }
        }
        logger::warn("Failed to remove embedded FRIK flashlight, armor not found");
    }

    /**
     * Send a message to the BetterScopesVR mod.
     */
    void FRIK::dispatchMessageToBetterScopesVR(const std::uint32_t messageType, void* data, const std::uint32_t dataLen) const
    {
        _messaging->Dispatch(messageType, data, dataLen, BETTER_SCOPES_VR_MOD_NAME);
    }

    void FRIK::onBetterScopesMessage(F4SE::MessagingInterface::Message* msg)
    {
        if (!msg) {
            return;
        }

        if (msg->type == 15) {
            logger::info("BetterScopesVR looking through scopes: {}", msg->dataLen);
            g_frik.setLookingThroughScope(static_cast<bool>(msg->data));
        }
    }

    /**
     * Dump game data if requested in "sDebugDumpDataOnceNames" flag in INI config.
     */
    void FRIK::checkDebugDump() const
    {
        ModBase::checkDebugDump();

        if (g_config.checkDebugDumpDataOnceFor("menus")) {
            _gameMenusHandler.debugDumpAllMenus();
        }
        if (g_config.checkDebugDumpDataOnceFor("weapon_muzzle")) {
            if (const auto muzzle = getMuzzleFlashNodes()) {
                f4cf::dump::printNodes(muzzle->fireNode);
                f4cf::dump::printNodes(muzzle->projectileNode);
            }
        }
    }
}
