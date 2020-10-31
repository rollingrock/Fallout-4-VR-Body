#pragma once
#include "f4se/GameReferences.h"
#include "f4se/NiNodes.h"

namespace F4VRBody
{

	class Matrix44 {
	public:
		Matrix44() {
			for (auto i = 0; i < 4; i++) {
				for (auto j = 0; j < 4; j++) {
					data[i][j] = 0.0;
				}
			}
		}

		float data[4][4];
	};

	class Skeleton {
	public:
		Skeleton() : _root(nullptr)
		{}

		Skeleton(BSFadeNode* a_node) : _root(a_node)
		{}

		BSFadeNode* getRoot() {
			return _root;
		}

		void printChildren(NiNode* child, std::string padding);
		void printNodes();
		void rotateWorld(NiNode* nde);
		void updatePos(NiNode* nde, NiPoint3 offset);
		void projectSkelly(float offsetOutFront);
		void updateDown(NiNode* nde);
		NiNode* getNode(const char* nodeName, NiNode *nde);
		void setupHead(NiNode* headNode);

		// Fallout Function Hooking
		static Matrix44 *matrixMultiply(NiMatrix43* worldMat, Matrix44* retMat, Matrix44* localMat);

	private:
		BSFadeNode* _root;
		NiPoint3   _lastPos;

		Vector2 _lookOut;

		std::vector<NiNode> _nodes;
	};
}
