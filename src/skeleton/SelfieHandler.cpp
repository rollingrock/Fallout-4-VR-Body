#include "SelfieHandler.h"

#include "FRIK.h"

using namespace common;

namespace frik
{
    void SelfieHandler::onFrameUpdate() const
    {
        basicSelfie();
    }

    /**
     * Projects the 3rd person body out in front of the player by offset amount
     */
    void SelfieHandler::basicSelfie() const
    {
        if (!g_frik.isSelfieModeOn()) {
            return;
        }

        const auto root = f4vr::getRootNode();
        if (!root) {
            return;
        }

        const float z = root->local.translate.z;
        const auto body = root->parent;

        const auto back = MatrixUtils::vec3Norm(RE::NiPoint3(-_forwardDir.x, -_forwardDir.y, 0));
        const auto bodyDir = RE::NiPoint3(0, 1, 0);

        root->local.rotate = MatrixUtils::getMatrixFromRotateVectorVec(back, bodyDir) * body->world.rotate.Transpose();
        root->local.translate = body->world.translate - f4vr::getCameraPosition();
        root->local.translate.y += g_config.selfieOutFrontDistance;
        root->local.translate.z = z;

        f4vr::updateTransformsDown(root, true);
    }

    void SelfieHandler::testSelfie()
    {
        if (!g_frik.isSelfieModeOn()) {
            if (_selfieActive) {
                _selfieActive = false;
                exitSelfieMode();
            }
            return;
        }

        if (!_selfieActive) {
            _selfieActive = true;
            enterSelfieMode();
        }

        const auto root = f4vr::getRootNode();

        const auto back = MatrixUtils::vec3Norm(RE::NiPoint3(-_forwardDir.x, -_forwardDir.y, 0));
        root->local.rotate = MatrixUtils::getMatrixFromRotateVectorVec(back, RE::NiPoint3(0, 1, 0)) * root->parent->world.rotate.Transpose();

        // rotate the skeleton
        // root->local.rotate = root->local.rotate * getMatrixFromDebugFlowFlags();

        const auto diff = root->parent->world.translate - _rootWorldPos;
        logger::sample("Diff: {:.2f}, {:.2f}, {:.2f}", diff.x, diff.y, diff.z);
        root->parent->local.translate -= diff;

        // f4vr::updateDown(pWorldNode, true);
        f4vr::updateDown(f4vr::getRootNode()->parent, true);
        // f4vr::updateDownFromRoot();
    }

    void SelfieHandler::enterSelfieMode()
    {
        const auto hmdRot = f4vr::getPlayerNodes()->HmdNode->local.rotate;
        _forwardDir = RE::NiPoint3(hmdRot.entry[1][0], hmdRot.entry[1][1], 0);

        _rootWorldPos = f4vr::getRootNode()->parent->world.translate;

        const auto player = RE::PlayerCharacter::GetSingleton();
        _playerStartPosition = player->GetPosition();

        const auto back = MatrixUtils::vec3Norm(RE::NiPoint3(-_forwardDir.x, -_forwardDir.y, 0));
        player->SetPosition(_playerStartPosition + back * g_config.selfieOutFrontDistance, true);
    }

    void SelfieHandler::exitSelfieMode() const
    {
        const auto player = RE::PlayerCharacter::GetSingleton();
        player->SetPosition(_playerStartPosition, true);
    }

    void SelfieHandler::experimental()
    {
        // const auto pWorldNode = f4vr::getPlayerNodes()->playerworldnode;
        //
        // const RE::NiNode* body = _root->parent;
        // const auto back = vec3Norm(RE::NiPoint3(-_forwardDir.x, -_forwardDir.y, 0));
        //
        // _root->local.rotate = getMatrixFromRotateVectorVec(back, RE::NiPoint3(0, 1, 0)) * body->world.rotate.Transpose();
        //
        // // pWorldNode->local.translate += back * g_config.selfieOutFrontDistance;
        //
        // pWorldNode->local.translate += RE::NiPoint3(0, 1, 0) * g_config.selfieOutFrontDistance;
        //
        // // getPlayerCamera()->cameraNode->local.translate += RE::NiPoint3(0, 1, 0) * g_config.selfieOutFrontDistance;
        // // updateDown(getPlayerCamera()->cameraNode, true);
        //
        // const auto playerCamera = RE::PlayerCamera::GetSingleton();
        // const auto cameraNode = playerCamera->cameraRoot.get();
        // cameraNode->local.translate += getPointFromDebugFlowFlags();
        // cameraNode->world.translate += getPointFromDebugFlowFlags();
        //
        // auto& playerLocalTransformPos = pWorldNode->local.translate;
        //
        // // updateUpTo(cameraNode->parent->parent->parent, cameraNode, true);
        //
        // // RE::NiUpdateData* ud = nullptr;
        // // cameraNode->Update(*ud);
        //
        // const auto player = RE::PlayerCharacter::GetSingleton();
        // player->SetPosition(player->GetPosition() + getPointFromDebugFlowFlags(), true);
        //
        // logger::sample("Camera: {} ({:.2f},{:.2f}) ({:.2f},{:.2f})", playerCamera->enabled, playerCamera->rotationInput.x, playerCamera->rotationInput.x,
        //     playerCamera->translationInput.x, playerCamera->translationInput.x);
        //
        // if (g_config.checkDebugDumpDataOnceFor("selfie_update")) {
        //     playerCamera->Update();
        // }
        //
        // if (g_config.checkDebugDumpDataOnceFor("selfie_free")) {
        //     playerCamera->ToggleFreeCameraMode(true);
        // }
    }
}
