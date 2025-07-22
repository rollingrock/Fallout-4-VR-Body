#include "Debug.h"

#include "common/CommonUtils.h"
#include "common/Logger.h"
#include "f4vr/PlayerNodes.h"

using namespace common;
using namespace RE::Scaleform;

namespace
{
    void printNode(const RE::NiAVObject* node, const std::string& padding)
    {
        const auto niNode = node->IsNode();
        const auto scale = std::fabs(node->local.scale - node->world.scale) < 0.001f
            ? std::format("{:.2f}", node->local.scale)
            : std::format("{:.2f}/{:.2f}", node->local.scale, node->world.scale);
        logger::infoRaw("{}{} : children({}), hidden({}), Local:({:.2f}, {:.2f}, {:.2f}), World:({:.2f}, {:.2f}, {:.2f}), Scale:({})",
            padding,
            node->name.c_str(),
            niNode ? niNode->children.size() : 0,
            node->flags.flags & 0x1,
            node->local.translate.x, node->local.translate.y, node->local.translate.z,
            node->world.translate.x, node->world.translate.y, node->world.translate.z,
            scale);
    }

    void printNodeChildren(RE::NiAVObject* node, std::string padding)
    {
        printNode(node, padding);
        if (const auto niNode = node->IsNode()) {
            padding += "..";
            for (const auto& child : niNode->children) {
                if (child) {
                    printNodeChildren(child.get(), padding);
                }
            }
        }
    }

    void printNodeAncestors(const RE::NiAVObject* node, std::string padding)
    {
        while (node) {
            printNode(node, padding);
            padding += "..";
            node = node->parent;
        }
    }
}

namespace frik
{
    void printMatrix(const RE::NiMatrix3* mat)
    {
        logger::info("Dump matrix:");
        std::string row;
        for (const auto i : mat->entry) {
            for (auto j = 0; j < 3; j++) {
                row += std::to_string(i[j]);
                row += " ";
            }
            logger::info("{}", row);
            row = "";
        }
    }

    void positionDiff()
    {
        const auto firstpos = f4vr::getPlayerNodes()->HmdNode->world.translate;
        const auto skellypos = f4vr::getRootNode()->world.translate;
        logger::info("difference = {} {} {}", firstpos.x - skellypos.x, firstpos.y - skellypos.y, firstpos.z - skellypos.z);
    }

    void printAllNodes()
    {
        auto* node = f4vr::getWorldRootNode();
        while (node->parent) {
            node = node->parent;
        }
        printNodes(node, false);
    }

    void printNodes(RE::NiAVObject* node, const bool printAncestors)
    {
        logger::info("Children of '{}':", node->name.c_str());
        printNodeChildren(node, "");
        if (printAncestors) {
            logger::info("Ancestors of '{}':", node->name.c_str());
            printNodeAncestors(node, "");
        }
    }

    void printNodes(RE::NiAVObject* node, const long long curTime)
    {
        const auto niNode = node->IsNode();
        logger::info("{} {} : children = {} {}: local {} {} {} {} {} {} {} {} {} ({} {} {})", curTime, node->name.c_str(), niNode ? niNode->children.size() : 0,
            node->flags.flags & 0x1, node->local.rotate.entry[0][0], node->local.rotate.entry[1][0], node->local.rotate.entry[2][0], node->local.rotate.entry[0][1],
            node->local.rotate.entry[1][1], node->local.rotate.entry[2][1], node->local.rotate.entry[0][2], node->local.rotate.entry[1][2], node->local.rotate.entry[2][2],
            node->local.translate.x, node->local.translate.y, node->local.translate.z);

        if (niNode) {
            for (const auto& child : niNode->children) {
                if (child) {
                    printNodes(child.get(), curTime);
                }
            }
        }
    }

    /**
     * Print the local transform data of nodes tree.
     */
    void printNodesTransform(RE::NiAVObject* node, std::string padding)
    {
        const auto niNode = node->IsNode();
        logger::info("{}{} child={}, {}, Pos:({:2.3f}, {:2.3f}, {:2.3f}), Rot:[[{:2.4f}, {:2.4f}, {:2.4f}][{:2.4f}, {:2.4f}, {:2.4f}][{:2.4f}, {:2.4f}, {:2.4f}]]", padding,
            node->name.c_str(), niNode ? niNode->children.size() : 0, node->flags.flags & 0x1 ? "hidden" : "visible", node->local.translate.x, node->local.translate.y,
            node->local.translate.z, node->local.rotate.entry[0][0], node->local.rotate.entry[1][0], node->local.rotate.entry[2][0], node->local.rotate.entry[0][1],
            node->local.rotate.entry[1][1], node->local.rotate.entry[2][1], node->local.rotate.entry[0][2], node->local.rotate.entry[1][2], node->local.rotate.entry[2][2]);

        if (niNode) {
            padding += "..";
            for (const auto& child : niNode->children) {
                if (child) {
                    printNodesTransform(child.get(), padding);
                }
            }
        }
    }

    void printTransform(const std::string& name, const RE::NiTransform& transform, const bool sample)
    {
        const auto frm = "Transform '" + name + "' Pos:({:.2f}, {:.2f}, {:.2f}), Rot:[[{:.2f}, {:.2f}, {:.2f}][{:.2f}, {:.2f}, {:.2f}][{:.2f}, {:.2f}, {:.2f}]], Scale:({:.2f})";
        if (sample) {
            logger::sample(fmt::runtime(frm), transform.translate.x, transform.translate.y, transform.translate.z, transform.rotate.entry[0][0], transform.rotate.entry[1][0],
                transform.rotate.entry[2][0], transform.rotate.entry[0][1], transform.rotate.entry[1][1], transform.rotate.entry[2][1], transform.rotate.entry[0][2],
                transform.rotate.entry[1][2], transform.rotate.entry[2][2], transform.scale);
        } else {
            logger::info(fmt::runtime(frm), transform.translate.x, transform.translate.y, transform.translate.z, transform.rotate.entry[0][0], transform.rotate.entry[1][0],
                transform.rotate.entry[2][0], transform.rotate.entry[0][1], transform.rotate.entry[1][1], transform.rotate.entry[2][1], transform.rotate.entry[0][2],
                transform.rotate.entry[1][2], transform.rotate.entry[2][2], transform.scale);
        }
    }

    void printPosition(const std::string& name, const RE::NiPoint3& pos, const bool sample)
    {
        const auto frm = "Transform '" + name + "' Pos: ({:.2f}, {:.2f}, {:.2f})";
        if (sample) {
            logger::sample(fmt::runtime(frm), pos.x, pos.y, pos.z);
        } else {
            logger::info(fmt::runtime(frm), pos.x, pos.y, pos.z);
        }
    }

    /**
     * Dump the player body parts and whatever they are hidden.
     */
    void dumpPlayerGeometry(RE::BSFadeNode* rn)
    {
        for (auto i = 0; i < rn->geomArray.size(); ++i) {
            const auto& geometry = rn->geomArray[i].geometry;
            logger::info("Geometry[{}] = '{}' ({})", i, geometry->name.c_str(), geometry->flags.flags & 0x1 ? "Hidden" : "Visible");
        }
    }

    void debug()
    {
        static std::uint64_t fc = 0;

        //Offsets::ForceGamePause(*g_menuControls);

        auto rn = static_cast<RE::BSFadeNode*>(f4vr::getRootNode()->parent);
        //logger::info("newrun");

        //for (int i = 0; i < 44; i++) {
        //	if ((*g_player)->equipData->slots[i].item != nullptr) {
        //		std::string name = (*g_player)->equipData->slots[i].item->GetFullName();
        //		auto form_type = (*g_player)->equipData->slots[i].item->GetFormType();
        //		logger::info("{} formType = {}", name, form_type);
        //		if (form_type == FormType::kFormType_ARMO) {
        //			auto form = reinterpret_cast<TESObjectARMO*>((*g_player)->equipData->slots[i].item);
        //			auto bipedslot = form->bipedObject.entry.parts;
        //			logger::info("biped slot = {}", bipedslot);
        //		}
        //	}
        //}

        //static bool runTimer = false;
        //static auto startTime = std::chrono::high_resolution_clock::now();

        //if (fc > 400) {
        //	fc = 0;
        //	RE::BSFixedString event("reloadStart");
        //	IAnimationGraphManagerHolder_NotifyAnimationGraph(&(*g_player)->animGraphHolder, event);
        //	runTimer = true;
        //	startTime = std::chrono::high_resolution_clock::now();
        //}

        //if (runTimer) {
        //	auto elapsed = std::chrono::high_resolution_clock::now() - startTime;
        //	if (300 < std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count()) {

        //		RE::BSFixedString event("reloadComplete");
        //		IAnimationGraphManagerHolder_NotifyAnimationGraph(&(*g_player)->animGraphHolder, event);
        //		TESObjectREFR_UpdateAnimation((*g_player), 5.0f);

        //		runTimer = false;
        //	}
        //}

        //logger::info("throwing-> {}", Actor_CanThrow(*g_player, g_equipIndex));

        //for (auto i = 0; i < rn->kGeomArray.capacity-1; ++i) {
        //	RE::BSFadeNode::FlattenedGeometryData data = rn->kGeomArray[i];
        //	logger::info("{}", data.spGeometry->name.c_str());
        //}

        //_playerNodes->ScopeParentNode->flags &= 0xfffffffffffffffe;
        //updateDown(dynamic_cast<RE::NiNode*>(_playerNodes->ScopeParentNode), true);

        //	RE::BSFixedString name("LArm_Hand");
        ////	RE::NiAVObject* node = (*g_player)->firstPersonSkeleton->GetObjectByName(&name);
        //	RE::NiAVObject* node = _root->GetObjectByName(&name);
        //	if (!node) { return; }
        //	logger::info("{} {} {} {} {} {} {}", node->flags & 0xF, node->local.translate.x,
        //									 node->local.translate.y,
        //									 node->local.translate.z,
        //									 node->world.translate.x,
        //									 node->world.translate.y,
        //									 node->world.translate.z
        //									);

        //	for (auto i = 0; i < (*g_player)->inventoryList->items.count; i++) {
        //	logger::info("{},{},%x,%x,{}", fc, i, (*g_player)->inventoryList->items[i].form->formID, (*g_player)->inventoryList->items[i].form->formType, (*g_player)->inventoryList->items[i].form->GetFullName());
        //}

        //if (fc < 1) {
        //	tHashSet<ObjectModMiscPair, BGSMod::Attachment::Mod*> *map = g_modAttachmentMap.GetPtr();
        //	map->Dump();
        //}

        //for (auto i = 0; i < rt->numTransforms; i++) {

        //if (rt->transforms[i].refNode) {
        //	logger::info("{},{},{},{}", fc, rt->transforms[i].refNode->name.c_str(), rt->transforms[i].childPos, rt->transforms[i].parPos);
        //}
        //else {
        //	logger::info("{},{},{},{}", fc, "", rt->transforms[i].childPos, rt->transforms[i].parPos);
        //}
        //		logger::info("{},{},{}", fc, i, rt->transforms[i].name);
        //}
        //
        //for (auto i = 0; i < rt->numTransforms; i++) {
        //	int pos = rt->bonePositions[i].translateition;
        //	if (rt->bonePositions[i].name && ((uint64_t)rt->bonePositions[i].name > 0x1000)) {
        //		logger::info("{},{},{}", fc, pos, rt->bonePositions[i].name->data);
        //	}
        //	else {
        //		logger::info("{},{}", fc, pos);
        //	}
        //}

        //	if (rt->bonePositions[i].name && (rt->bonePositions[i].translateition != 0) && ((uint64_t)rt->bonePositions[i].name > 0x1000)) {
        //		int pos = rt->bonePositions[i].translateition;
        //		if (pos > rt->numTransforms) {
        //			continue;
        //		}
        //		logger::info("{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{},{}", fc, rt->bonePositions[i].name->data,
        //							rt->bonePositions[i].translateition,
        //							rt->transforms[pos].local.rotate.entry[0],
        //							rt->transforms[pos].local.rotate.entry[1],
        //							rt->transforms[pos].local.rotate.entry[2],
        //							rt->transforms[pos].local.rotate.entry[3],
        //							rt->transforms[pos].local.rotate.entry[4],
        //							rt->transforms[pos].local.rotate.entry[5],
        //							rt->transforms[pos].local.rotate.entry[6],
        //							rt->transforms[pos].local.rotate.entry[7],
        //							rt->transforms[pos].local.rotate.entry[8],
        //							rt->transforms[pos].local.rotate.entry[9],
        //							rt->transforms[pos].local.rotate.entry[10],
        //							rt->transforms[pos].local.rotate.entry[11],
        //							rt->transforms[pos].local.translate.x,
        //							rt->transforms[pos].local.translate.y,
        //							rt->transforms[pos].local.translate.z,
        //							rt->transforms[pos].world.rotate.entry[0],
        //							rt->transforms[pos].world.rotate.entry[1],
        //							rt->transforms[pos].world.rotate.entry[2],
        //							rt->transforms[pos].world.rotate.entry[3],
        //							rt->transforms[pos].world.rotate.entry[4],
        //							rt->transforms[pos].world.rotate.entry[5],
        //							rt->transforms[pos].world.rotate.entry[6],
        //							rt->transforms[pos].world.rotate.entry[7],
        //							rt->transforms[pos].world.rotate.entry[8],
        //							rt->transforms[pos].world.rotate.entry[9],
        //							rt->transforms[pos].world.rotate.entry[10],
        //							rt->transforms[pos].world.rotate.entry[11],
        //							rt->transforms[pos].world.translate.x,
        //							rt->transforms[pos].world.translate.y,
        //							rt->transforms[pos].world.translate.z
        //		);

        //	if(strstr(rt->bonePositions[i].name->data, "Finger")) {
        //		Matrix44 rot;
        //		rot.makeIdentity();
        //		rt->transforms[pos].local.rotate = rot.make43();

        //		if (rt->transforms[pos].refNode) {
        //			rt->transforms[pos].refNode->local.rotate = rot.make43();
        //		}

        //		rot.makeTransformMatrix(rt->transforms[pos].local.rotate, RE::NiPoint3(0, 0, 0));

        //		short parent = rt->transforms[pos].parPos;
        //		rt->transforms[pos].world.rotate = rot.make43() * rt->transforms[parent].world.rotate;

        //		if (rt->transforms[pos].refNode) {
        //			rt->transforms[pos].refNode->world.rotate = rt->transforms[pos].world.rotate;
        //		}

        //	}
        //	}

        //rt->UpdateWorldBound();
        //}

        fc++;
    }

    void printScaleFormElements(GFx::Value* elm, const std::string& padding)
    {
        GFx::Value childrenCountVal;
        elm->GetMember("numChildren", &childrenCountVal);
        const int childrenCount = childrenCountVal.GetType() == GFx::Value::ValueType::kInt ? childrenCountVal.GetInt() : 0;

        GFx::Value nameVal;
        elm->GetMember("name", &nameVal);
        const auto name = nameVal.IsString() ? nameVal.GetString() : "Unknown";

        GFx::Value visibleVal;
        elm->GetMember("visible", &visibleVal);
        const int visible = visibleVal.IsBoolean() ? visibleVal.GetBoolean() : false;

        GFx::Value toStringVal;
        elm->Invoke("toString", &toStringVal, nullptr, 0);
        const auto toString = toStringVal.IsString() ? toStringVal.GetString() : "";

        GFx::Value textVal;
        elm->GetMember("text", &textVal);
        GFx::Value buttonTextVal;
        elm->GetMember("ButtonText", &buttonTextVal);
        const auto text = textVal.IsString() ? textVal.GetString() : "";

        logger::infoRaw("{}{} : children({}), visible({}), toString:({}), text:({})", padding, name, childrenCount, visible, toString, text);

        for (int i = 0; i < childrenCount; ++i) {
            GFx::Value child;
            GFx::Value args[1];
            args[0] = i;
            if (elm->Invoke("getChildAt", &child, args, 1)) {
                printScaleFormElements(&child, padding + "..");
            } else {
                logger::warn("Failed to get child at index {}.", i);
            }
        }
    }
}
