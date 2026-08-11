#include "FRIK.h"

#include "Config.h"
#include "ExternalAuthority.h"
#include "GameHooks.h"
#include "PapyrusApi.h"
#include "api/ApiCore.h"
#include "api/FRIKApiV2.h"
#include "api/RecoilControllerRuntime.h"
#include "common/PerfMonitor.h"
#include "config-mode/PipboyConfigMode.h"
#include "f4vr/DebugDump.h"
#include "f4vr/F4VRSkelly.h"
#include "f4vr/F4VRUtils.h"
#include "pipboy/Pipboy.h"
#include "skeleton/HandPose.h"
#include "skeleton/Skeleton.h"
#include "smooth-movement/SmoothMovementVR.h"
#include "utils.h"
#include "vrcf/VRControllersManager.h"
#include "vrui/UIManager.h"
#include "vrui/UIModAdapter.h"

using namespace common;

// This is the entry point to the mod.
extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Query(const F4SE::QueryInterface* a_skse, F4SE::PluginInfo* a_info)
{
    if (a_skse->IsEditor() || !REL::Module::IsVR() || REL::Module::get().version() != F4SE::RUNTIME_VR_1_2_72) {
        return false;
    }

    return g_mod->onF4SEPluginQuery(a_skse, a_info);
}

// This is the entry point to the mod.
extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* a_f4se)
{
    return g_mod->onF4SEPluginLoad(a_f4se);
}

namespace frik
{
    namespace
    {
        constexpr std::uint32_t kSkeletonInitDelayFramesAfterRelease = 1;
    }

    /**
     * TODO: think about it, is it the best way to handle this dependency indirection.
     */
    class FrameUpdateContext : public vrui::UIModAdapter
    {
    public:
        explicit FrameUpdateContext(Skeleton* skelly)
            : _skelly(skelly)
        {}

        virtual RE::NiPoint3 getInteractionBoneWorldPosition() override
        {
            return f4vr::Skelly::getIndexFingerTipWorldPosition(vrcf::Hand::Offhand);
        }

        virtual void setInteractionHandPointing(const bool primaryHand, const bool toPoint) override
        {
            HandPose::setForceHandPointingPose(primaryHand, toPoint);
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
        initForFalloutLondonVR();

        logger::info("Register papyrus native functions...");
        api::initPapyrusApis();
        _boneSpheres.init();

        _gameMenusHandler.init([this](const std::string& name, const bool isOpened) {
            if (isOpened) {
                onGameMenuOpened(name, isOpened);
            }
        });

        addEmbeddedFlashlightKeywordIfNeeded();

        if (isBetterScopesVRModLoaded()) {
            logger::info("BetterScopesVR mod detected, registering for messages...");
            _messaging->Dispatch(15, static_cast<void*>(nullptr), sizeof(bool), BETTER_SCOPES_VR_MOD_NAME);
            _messaging->RegisterListener(onBetterScopesMessage, BETTER_SCOPES_VR_MOD_NAME);
        }
    }

    /**
     * Check if the Fallout London VR mod is loaded and initialize compatibility mode if so.
     * Special flag to ignore the mod can be used to use regular Pipboy instead of the Attaboy.
     */
    void FRIK::initForFalloutLondonVR()
    {
        g_config.isFalloutLondonVR = isFalloutLondonVRModLoaded();
        if (g_config.isFalloutLondonVR) {
            logger::info("Fallout London VR mod detected, enabling compatibility mode...");
            g_config.reloadForFalloutLondonVR();
            if (g_config.ignoreFalloutLondonVR) {
                logger::warn("Ignore Fallout London VR flag set, do not treat the game as Fallout London!");
                g_config.isFalloutLondonVR = false;
            }
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
        }

        _smoothMovement.reset();
        configureGameVars();

        _playerControlsHandler.reset();
    }

    /**
     * Called on every game frame via hooks into the game engine.
     * This is where all the magic happens by updating game state and nodes.
     */
    void FRIK::onFrameUpdate()
    {
        static PerfMonitor perf("FRIK::onFrameUpdate");
        const auto timer = perf.scope();

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
            if (_gameMenusHandler.isLoadingMenuOpen()) {
                return;
            }

            if (_skeletonInitDelayFrames > 0) {
                --_skeletonInitDelayFrames;
                return;
            }

            if (!isGameReadyForSkeletonInitialization()) {
                return;
            }

            initSkeleton();
            if (!_skelly) {
                return;
            }
        }

        logger::trace("Update Skeleton...");
        _skelly->onFrameUpdate();

        logger::trace("Update Bone Sphere...");
        _boneSpheres.onFrameUpdate();

        logger::trace("Update player controls...");
        _playerControlsHandler.onFrameUpdate(_mainConfigMode, _pipboy, _weaponPosition, _pipboyConfigMode);

        if (_weaponPositionEnabled) {
            logger::trace("Update Weapon Position...");
            _weaponPosition->onFrameUpdate();
        }

        logger::trace("Update Pipboy...");
        _pipboy->onFrameUpdate();

        FrameUpdateContext context(_skelly);
        vrui::g_uiManager->onFrameUpdate(&context);

        _mainConfigMode.onFrameUpdate();

        _pipboyConfigMode->onFrameUpdate();

        updateWorldFinal();

        if (!_skeletonReadyPublished) {
            _skeletonReadyPublished = true;
            broadcastMessage(static_cast<std::uint32_t>(api::FRIKApiV2::LifecycleEvent::kSkeletonReady), nullptr, 0);
        }
    }

    void FRIK::smoothMovement()
    {
        if (!_smoothMovementEnabled) {
            return;
        }
        try {
            _smoothMovement.onFrameUpdate();
        } catch (const std::exception& e) {
            logger::error("Error in FRIK::smoothMovement: {}", e.what());
        }
    }

    void FRIK::initSkeleton()
    {
        _inPowerArmor = f4vr::isInPowerArmor();
        _skeletonInitDelayFrames = 0;

        const auto player = f4vr::getPlayer();
        logger::info("Initialize Skeleton ({}) ; Nodes: Player={}, Data={}, Root={}, Skeleton={}, Common={}",
            _inPowerArmor ? "PowerArmor" : "Regular",
            static_cast<const void*>(player),
            static_cast<const void*>(player->loadedData),
            static_cast<const void*>(player->loadedData->data3D.get()),
            static_cast<const void*>(f4vr::getRootNode()),
            static_cast<const void*>(f4vr::getCommonNode()));

        // init skeleton
        _workingRootNode = f4vr::getRootNode();
        _skeletonReadyPublished = false;

        auto* skelly = new Skeleton(f4vr::getRootNode(), _inPowerArmor);
        if (!skelly->isInitialized()) {
            logger::warn("Skeleton initialization failed after readiness checks; retrying later.");
            delete skelly;
            _workingRootNode = nullptr;
            _skeletonInitDelayFrames = kSkeletonInitDelayFramesAfterRelease;
            return;
        }
        _skelly = skelly;

        // init handlers depending on skeleton
        _pipboy = new Pipboy(_skelly);
        _pipboyConfigMode = new PipboyConfigMode(_skelly);
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
        if (!player) {
            return false;
        }

        const auto playerData = player->loadedData;
        if (!playerData) {
            return false;
        }

        const auto playerRootNode = playerData->data3D.get();
        const auto rootNode = f4vr::getRootNode();
        const auto worldRootNode = f4vr::getWorldRootNode();
        const auto commonNode = f4vr::getCommonNode();
        const auto playerNodes = f4vr::getPlayerNodes();
        const auto flattenedTree = f4vr::getFlattenedBoneTree();
        const auto firstPersonSkeleton = f4vr::getFirstPersonSkeleton();
        const auto rightHand = firstPersonSkeleton ? f4vr::findNode(firstPersonSkeleton, "RArm_Hand") : nullptr;
        const auto leftHand = firstPersonSkeleton ? f4vr::findNode(firstPersonSkeleton, "LArm_Hand") : nullptr;
        const auto weaponNode = f4vr::getWeaponNode();
        const auto camera = f4vr::getPlayerCamera();
        const auto cameraRoot = camera && camera->cameraRoot ? camera->cameraRoot.get() : nullptr;

        if (!playerRootNode || !rootNode || !worldRootNode) {
            return false;
        }

        if (!rootNode->parent) {
            return false;
        }

        if (!commonNode || !playerNodes || !flattenedTree) {
            return false;
        }

        if (!rightHand || !leftHand) {
            return false;
        }

        if (!weaponNode) {
            return false;
        }

        if (!camera || !cameraRoot) {
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
    void FRIK::onGameMenuOpened(const std::string& name, const bool isOpened)
    {
        if (isOpened && _skelly && _gameMenusHandler.isLoadingMenuOpen()) {
            logger::info("Loading menu is open, reset skeleton...");
            releaseSkeleton();
        }
        if (isOpened && _pipboy && _pipboy->isOpen() && name == "TerminalMenu") {
            logger::info("Close Pipboy due to terminal open...");
            _pipboy->openClose(false);
        }
    }

    /**
     * On switch from normal and power armor, reset the skelly and all dependencies with persistent data.
     */
    void FRIK::releaseSkeleton()
    {
        if (_skelly && _skeletonReadyPublished) {
            broadcastMessage(static_cast<std::uint32_t>(api::FRIKApiV2::LifecycleEvent::kSkeletonDestroying), nullptr, 0);
        }
        _skeletonReadyPublished = false;

        // Every external registration was published against nodes that are about to go away, so both
        // registries drop theirs here and clients republish after the next skeleton-ready event.
        g_externalAuthority.clearForSkeletonRelease();
        api::clearWeaponHandRecoilControllersForSkeletonRelease();

        _workingRootNode = nullptr;
        _skeletonInitDelayFrames = kSkeletonInitDelayFramesAfterRelease;

        _playerControlsHandler.reset();
        _smoothMovement.reset();

        delete _skelly;
        _skelly = nullptr;

        delete _pipboy;
        _pipboy = nullptr;

        delete _pipboyConfigMode;
        _pipboyConfigMode = nullptr;

        delete _weaponPosition;
        _weaponPosition = nullptr;

        _inPowerArmor = false;
        _dynamicCameraHeight = 0.0f;
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
     * If to add embedded flashlight to the game.
     */
    void FRIK::addEmbeddedFlashlightKeywordIfNeeded()
    {
        if (g_config.removeFlashlight) {
            F4SE::log::info("Flashlight disabled in config, skipping");
            return;
        }

        if (!g_frik.isFlashlightEnabled()) {
            F4SE::log::info("Flashlight disabled via API, skipping FRIK flashlight");
            return;
        }

        if (isDLLModLoaded("ImmersiveFlashlightVR")) {
            F4SE::log::info("Immersive Flashlight VR mod detected, skipping FRIK flashlight");
            return;
        }

        if (auto* armorObj = RE::TESForm::GetFormByID<RE::TESObjectARMO>(0x21B3B)) {
            if (const auto keywordObj = RE::TESForm::GetFormByID<RE::BGSKeyword>(0xB34A6)) {
                g_config.flashlightEnabled = true;
                if (!armorObj->HasKeyword(keywordObj)) {
                    logger::info("Init embedded flashlight, add keyword to: '{}', keyword: 0x{:x}", armorObj->GetFullName(), keywordObj->formID);
                    armorObj->AddKeyword(keywordObj);
                } else {
                    logger::warn("Init embedded flashlight, keyword already exists in '{}'", armorObj->GetFullName());
                }
            } else {
                logger::error("Failed to add init embedded flashlight, keyword not found");
            }
        } else {
            logger::error("Failed to init embedded flashlight, armor not found");
        }
    }

    /**
     * Send a message to the another mod in the game.
     */
    void FRIK::dispatchMessageToExternalMod(const std::string& receivingModName, const std::uint32_t messageType, void* data, const std::uint32_t dataLen) const
    {
        _messaging->Dispatch(messageType, data, dataLen, receivingModName.c_str());
    }

    void FRIK::broadcastMessage(const std::uint32_t messageType, void* data, const std::uint32_t dataLen) const
    {
        _messaging->Dispatch(messageType, data, dataLen, nullptr);
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
                f4vr::DebugDump::printNodes(muzzle->fireNode);
                f4vr::DebugDump::printNodes(muzzle->projectileNode);
            }
        }
    }
}
