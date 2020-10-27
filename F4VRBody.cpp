#include "F4VRBody.h"


namespace F4VRBody {

	bool isLoaded = false;

	class Skeleton {
	public:
		Skeleton() : _root(nullptr)
		{}

		Skeleton(BSFadeNode *a_node) : _root(a_node)
		{}

		BSFadeNode* getRoot() {
			return _root;
		}

		void printChildren(NiNode *child, std::string padding) {
			padding += "....";
			_MESSAGE("%s%s : children = %d : local z = %f : world z = %f", padding.c_str(), child->m_name.c_str(), child->m_children.m_emptyRunStart, child->m_localTransform.pos.z, child->m_worldTransform.pos.z);

			for (auto i = 0; i < child->m_children.m_emptyRunStart; ++i) {
				auto nextNode = child->m_children.m_data[i] ? child->m_children.m_data[i]->GetAsNiNode() : nullptr;
				if (nextNode) {
					this->printChildren(nextNode, padding);
				}
			}
		}

		void printNodes() {
			// print root node info first
			_MESSAGE("%s : children = %d : local z = %f : world z = %f", _root->m_name.c_str(), _root->m_children.m_emptyRunStart, _root->m_localTransform.pos.z, _root->m_worldTransform.pos.z);

			std::string padding = "";

			for (auto i = 0; i < _root->m_children.m_emptyRunStart; ++i) {
				auto nextNode = _root->m_children.m_data[i] ? _root->m_children.m_data[i]->GetAsNiNode() : nullptr;
				if (nextNode) {
					this->printChildren(nextNode, padding);
				}
			}
		}




	private:
		BSFadeNode *_root;
		NiPoint3   _lastPos;

		std::vector<NiNode> _nodes;
	};

	Skeleton *playerSkelly = nullptr;

	bool setSkelly() {
		if ((*g_player)->unkF0 && (*g_player)->unkF0->rootNode) {
			playerSkelly = new Skeleton((BSFadeNode*)(*g_player)->unkF0->rootNode);
			_MESSAGE("skeleton = %016I64X", playerSkelly->getRoot());
			return true;
		}
		else {
			return false;
		}
	}

	void update() {
		if (!isLoaded) {
			return;
		}

		if (!playerSkelly) {
			if (!setSkelly()) {
				return;
			}
			playerSkelly->printNodes();
			
		}

		// do stuff now
	}


	void startUp() {
		_MESSAGE("Starting up F4Body");
		isLoaded = true;
		return;
	}
}