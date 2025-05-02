#include "VR.h"
#include "matrix.h"

namespace VRHook {

	RelocPtr<uint64_t*> vrDataStruct(0x59429c0);

	void HmdMatrixToNiTransform(NiTransform* a_transform, vr::TrackedDevicePose_t* a_pose) {
		using func_t = decltype(&HmdMatrixToNiTransform);
		RelocAddr<func_t> func(0x1bab210);
		return func(a_transform, a_pose);
	}

	void VRSystem::getTrackerNiTransformByName(const std::string& trackerName, NiTransform* transform) {
		auto it = viveTrackers.find(trackerName);
		if (it != viveTrackers.end()) {
			vr::TrackedDevicePose_t pose = renderPoses[it->second];
			HmdMatrixToNiTransform(transform, &pose);
			applyRoomTransform(transform);
		}
	}

	void VRSystem::getTrackerNiTransformByIndex(vr::TrackedDeviceIndex_t idx, NiTransform* transform) {
		if (idx < vr::k_unMaxTrackedDeviceCount) {
			vr::TrackedDevicePose_t pose = renderPoses[idx];
			HmdMatrixToNiTransform(transform, &pose);
			applyRoomTransform(transform);
		}
	}

	void VRSystem::getControllerNiTransformByName(const std::string& trackerName, NiTransform* transform) {
		auto it = controllers.find(trackerName);
		if (it != controllers.end()) {
			vr::TrackedDevicePose_t pose = renderPoses[it->second];
			HmdMatrixToNiTransform(transform, &pose);
			applyRoomTransform(transform);
		}
	}

	void VRSystem::applyRoomTransform(NiTransform* transform) {
		if (vrDataStruct && roomNode) {
			NiMatrix43* worldSpaceMat = reinterpret_cast<NiMatrix43*>(reinterpret_cast<char*>(*vrDataStruct) + 0x210);
			NiPoint3* worldSpaceVec = reinterpret_cast<NiPoint3*>(reinterpret_cast<char*>(*vrDataStruct) + 0x158);
			transform->pos -= *worldSpaceVec;
			transform->pos = *worldSpaceMat * transform->pos;
		}
	}

	void VRSystem::initializeDevices() {
		for (vr::TrackedDeviceIndex_t i = 0; i < vr::k_unMaxTrackedDeviceCount; ++i) {
			auto dc = vrHook->GetVRSystem()->GetTrackedDeviceClass(i);
			std::string prop = getProperty(vr::ETrackedDeviceProperty::Prop_ModelNumber_String, i);
			if (dc == vr::ETrackedDeviceClass::TrackedDeviceClass_GenericTracker) {
				viveTrackers[prop] = i;
			} else if (dc == vr::ETrackedDeviceClass::TrackedDeviceClass_Controller) {
				if (prop.find("Right") != std::string::npos) {
					controllers["Right"] = i;
				} else if (prop.find("Left") != std::string::npos) {
					controllers["Left"] = i;
				}
			} else if (dc == vr::ETrackedDeviceClass::TrackedDeviceClass_HMD) {
				controllers["HMD"] = i;
			}
		}
	}

	std::string VRSystem::getProperty(vr::ETrackedDeviceProperty property, vr::TrackedDeviceIndex_t idx) const {
		const uint32_t bufSize = vr::k_unMaxPropertyStringSize;
		std::unique_ptr<char[]> pchValue = std::make_unique<char[]>(bufSize);
		vr::TrackedPropertyError pError = vr::TrackedPropertyError::TrackedProp_NotYetAvailable;

		vrHook->GetVRSystem()->GetStringTrackedDeviceProperty(idx, property, pchValue.get(), bufSize, &pError);
		return std::string(pchValue.get());
	}

	void VRSystem::debugPrint() {
		vr::VRCompositorError error = vrHook->GetVRCompositor()->GetLastPoses(renderPoses, vr::k_unMaxTrackedDeviceCount, gamePoses, vr::k_unMaxTrackedDeviceCount);
		if (error != vr::EVRCompositorError::VRCompositorError_None) {
			_MESSAGE("Error while retrieving game poses!");
		}

		for (const auto& tracker : viveTrackers) {
			vr::TrackedDevicePose_t renderMat = renderPoses[tracker.second];
			vr::TrackedDevicePose_t gameMat = gamePoses[tracker.second];

			NiTransform renderTran;
			HmdMatrixToNiTransform(&renderTran, &renderMat);

			_MESSAGE("%d : %s --> render = %f %f %f |||| game = %f %f %f", tracker.second, tracker.first.c_str(),
				renderTran.pos.x, renderTran.pos.y, renderTran.pos.z,
				gameMat.mDeviceToAbsoluteTracking.m[0][3], gameMat.mDeviceToAbsoluteTracking.m[1][3], gameMat.mDeviceToAbsoluteTracking.m[2][3]);
		}
	}

	VRSystem* g_vrHook = nullptr;

}
