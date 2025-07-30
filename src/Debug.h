#pragma once

namespace frik
{
    void printMatrix(const RE::NiMatrix3* mat);
    void positionDiff();
    void printAllNodes();
    void printNodes(RE::NiAVObject* node, bool printAncestors = true);
    void printNodesTransform(RE::NiAVObject* node, bool printAncestors = true);
    void printTransform(const std::string& name, const RE::NiTransform& transform, bool sample = false);
    void printPosition(const std::string& name, const RE::NiPoint3& pos, bool sample);
    void dumpPlayerGeometry(RE::BSFadeNode* rn);
    void debug();
    void printScaleFormElements(RE::Scaleform::GFx::Value* elm, const std::string& padding = "");
}
