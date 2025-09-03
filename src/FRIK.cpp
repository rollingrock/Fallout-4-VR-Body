#include "FRIK.h"

#include <cpptrace/from_current.hpp>

#include "Config.h"
#include "ConfigurationMode.h"
#include "Debug.h"
#include "HandPose.h"
#include "hook.h"
#include "PapyrusApi.h"
#include "PapyrusGateway.h"
#include "patches.h"
#include "Pipboy.h"
#include "Skeleton.h"
#include "SmoothMovementVR.h"
#include "utils.h"
#include "common/Logger.h"
#include "f4vr/F4VRUtils.h"
#include "f4vr/VRControllersManager.h"
#include "ui/UIManager.h"
#include "ui/UIModAdapter.h"

using namespace common;

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
     * On load of FRIK plugin by F4VRSE setup messaging and papyrus handling once.
     */
    void FRIK::initialize(const F4SE::LoadInterface*)
    {
        CPPTRACE_TRY
            {
                logger::info("Initialize FRIK...");
                std::srand(static_cast<unsigned int>(time(nullptr)));

                logger::info("Init config...");
                g_config.load();

                // allocate enough space for patches and hooks
                F4SE::AllocTrampoline(4096);

                logger::info("Run patches...");
                patches::patchAll();

                logger::info("Hook main...");
                hookMain();

                _messaging = F4SE::GetMessagingInterface();
                _messaging->RegisterListener(onF4VRSEMessage);
            }
        CPPTRACE_CATCH(const std::exception& ex) {
            const auto stacktrace = cpptrace::from_current_exception().to_string();
            logger::critical("Error in initialize: {}\n{}", ex.what(), stacktrace);
            throw;
        }
    }

    /**
     * F4VRSE messages listener to handle game loaded, new game, and save loaded events.
     */
    void FRIK::onF4VRSEMessage(F4SE::MessagingInterface::Message* msg)
    {
        if (!msg) {
            return;
        }

        if (msg->type == F4SE::MessagingInterface::kGameLoaded) {
            // One time event fired after all plugins are loaded and game is full in main menu
            logger::info("F4VRSE On Game Loaded Message...");
            g_frik.initOnGameLoaded();
        }

        if (msg->type == F4SE::MessagingInterface::kPostLoadGame || msg->type == F4SE::MessagingInterface::kNewGame) {
            // If a game is loaded or a new game started re-initialize FRIK for clean slate
            logger::info("F4VRSE On Post Load Message...");
            g_frik.initOnGameSessionLoaded();
        }
    }

    /**
     * On game fully loaded initialize things that should be initialized only once.
     */
    void FRIK::initOnGameLoaded()
    {
        CPPTRACE_TRY
            {
                logger::info("Register papyrus native functions...");
                initPapyrusApis();
                PapyrusGateway::init();
                _boneSpheres.init();

                vrui::initUIManager();

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
        CPPTRACE_CATCH(const std::exception& ex) {
            const auto stacktrace = cpptrace::from_current_exception().to_string();
            logger::critical("Error in initOnGameLoaded: {}\n{}", ex.what(), stacktrace);
            throw;
        }
    }

    /**
     * Game session can be initialized multiple times as it is fired on new game and save loaded events.
     * We should clear and reload as much of the game state as we can.
     */
    void FRIK::initOnGameSessionLoaded()
    {
        if (_skelly) {
            logger::info("Resetting skeleton for new game session...");
            releaseSkeleton();
            logger::info("Reload config...");
            g_config.load();
        }

        configureGameVars();

        f4vr::VRControllers.reset();
    }

    /**
     * Called on every game frame via hooks into the game engine.
     * This is where all the magic happens by updating game state and nodes.
     */
    void FRIK::onFrameUpdate()
    {
        CPPTRACE_TRY
            {
                onFrameUpdateInner();
            }
        CPPTRACE_CATCH(const std::exception& e) {
            const auto stacktrace = cpptrace::from_current_exception().to_string();
            logger::error("Error in FRIK::onFrameUpdate: {}\n{}", e.what(), stacktrace);
        }
    }

    void FRIK::onFrameUpdateInner()
    {
        if (!RE::PlayerCharacter::GetSingleton()) {
            // game not loaded or existing
            return;
        }

        f4vr::VRControllers.update(f4vr::isLeftHandedMode());

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

        checkDebugDump();
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

    void FRIK::checkDebugDump()
    {
        if (g_config.checkDebugDumpDataOnceFor("all_nodes")) {
            printAllNodes();
        }
        if (g_config.checkDebugDumpDataOnceFor("pipboy")) {
            printNodes(f4vr::getPlayerNodes()->PipboyRoot_nif_only_node);
        }
        if (g_config.checkDebugDumpDataOnceFor("world")) {
            printNodes(f4vr::getPlayerNodes()->primaryWeaponScopeCamera->parent->parent->parent->parent->parent->parent);
        }
        if (g_config.checkDebugDumpDataOnceFor("fp_skelly")) {
            printNodes(f4vr::getFirstPersonSkeleton());
        }
        if (g_config.checkDebugDumpDataOnceFor("skelly")) {
            printNodes(f4vr::getRootNode()->parent);
        }
        if (g_config.checkDebugDumpDataOnceFor("menus")) {
            _gameMenusHandler.debugDumpAllMenus();
        }
        if (g_config.checkDebugDumpDataOnceFor("weapon_muzzle")) {
            if (const auto muzzle = getMuzzleFlashNodes()) {
                printNodes(muzzle->fireNode);
                printNodes(muzzle->projectileNode);
            }
        }
    }
}
