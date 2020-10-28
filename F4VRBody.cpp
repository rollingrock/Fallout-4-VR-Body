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
			_MESSAGE("%s%s : children = %d : local x = %f : world x = %f : local y = %f : world y = %f : local z = %f : world z = %f", padding.c_str(), child->m_name.c_str(), child->m_children.m_emptyRunStart,
				child->m_localTransform.pos.x, child->m_worldTransform.pos.x,
				child->m_localTransform.pos.y, child->m_worldTransform.pos.y,
				child->m_localTransform.pos.z, child->m_worldTransform.pos.z);

			for (auto i = 0; i < child->m_children.m_emptyRunStart; ++i) {
				auto nextNode = child->m_children.m_data[i] ? child->m_children.m_data[i]->GetAsNiNode() : nullptr;
				if (nextNode) {
					this->printChildren(nextNode, padding);
				}
			}
		}

		void printNodes() {
			// print root node info first
			_MESSAGE("%s : children = %d : local x = %f : world x = %f : local y = %f : world y = %f : local z = %f : world z = %f", _root->m_name.c_str(), _root->m_children.m_emptyRunStart, 
				_root->m_localTransform.pos.x, _root->m_worldTransform.pos.x,
				_root->m_localTransform.pos.y, _root->m_worldTransform.pos.y,
				_root->m_localTransform.pos.z, _root->m_worldTransform.pos.z);

			std::string padding = "";

			for (auto i = 0; i < _root->m_children.m_emptyRunStart; ++i) {
				auto nextNode = _root->m_children.m_data[i] ? _root->m_children.m_data[i]->GetAsNiNode() : nullptr;
				if (nextNode) {
					this->printChildren(nextNode, padding);
				}
			}
		}

		void updateZ(NiNode* nde) {
		//	if (!strcmp(nde->m_name.c_str(), "LLeg_Foot") || !strcmp(nde->m_name.c_str(), "LLeg_Calf")) {
				//nde->m_localTransform.pos.x = nde->m_localTransform.pos.x + 20.0;
				//nde->m_localTransform.pos.y = nde->m_localTransform.pos.y + 20.0;
				//nde->m_localTransform.pos.z = nde->m_localTransform.pos.z + 100.0;

				nde->m_worldTransform.pos.x = nde->m_worldTransform.pos.x + 10.0;
				nde->m_worldTransform.pos.y = nde->m_worldTransform.pos.y + 10.0;
				nde->m_worldTransform.pos.z = nde->m_worldTransform.pos.z + 5.0;

				//uint64_t flag = 0x200000000;

				//nde->flags = nde->flags | flag;
			//}

			for (auto i = 0; i < nde->m_children.m_emptyRunStart; ++i) {
				auto nextNode = nde->m_children.m_data[i] ? nde->m_children.m_data[i]->GetAsNiNode() : nullptr;
				if (nextNode) {
					this->updateZ(nextNode);
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

	void update(uint64_t a_addr) {
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

		playerSkelly->updateZ((NiNode*)playerSkelly->getRoot());

		//playerSkelly->printNodes();
	}


	void startUp() {
		_MESSAGE("Starting up F4Body");
		isLoaded = true;
		return;
	}
}