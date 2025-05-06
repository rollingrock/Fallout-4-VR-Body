#include "VR.h"
#include "matrix.h"

namespace VRHook {
	RelocPtr<uint64_t*> vrDataStruct(0x59429c0);

	void hmdMatrixToNiTransform(const NiTransform* transform, const vr::TrackedDevicePose_t* pose) {
		using func_t = decltype(&hmdMatrixToNiTransform);
		RelocAddr<func_t> func(0x1bab210);
		return func(transform, pose);
	}

	void VRSystem::getTrackerNiTransformByName(const std::string& trackerName, NiTransform* transform) {
		const auto it = _viveTrackers.find(trackerName);
		if (it != _viveTrackers.end()) {
			const vr::TrackedDevicePose_t pose = _renderPoses[it->second];
			hmdMatrixToNiTransform(transform, &pose);
			applyRoomTransform(transform);
		}
	}

	void VRSystem::getTrackerNiTransformByIndex(const vr::TrackedDeviceIndex_t idx, NiTransform* transform) const {
		if (idx < vr::k_unMaxTrackedDeviceCount) {
			const vr::TrackedDevicePose_t pose = _renderPoses[idx];
			hmdMatrixToNiTransform(transform, &pose);
			applyRoomTransform(transform);
		}
	}

	void VRSystem::getControllerNiTransformByName(const std::string& trackerName, NiTransform* transform) {
		const auto it = _controllers.find(trackerName);
		if (it != _controllers.end()) {
			const vr::TrackedDevicePose_t pose = _renderPoses[it->second];
			hmdMatrixToNiTransform(transform, &pose);
			applyRoomTransform(transform);
		}
	}

	void VRSystem::applyRoomTransform(NiTransform* transform) const {
		if (vrDataStruct && _roomNode) {
			const auto worldSpaceMat = reinterpret_cast<NiMatrix43*>(reinterpret_cast<char*>(*vrDataStruct) + 0x210);
			const auto worldSpaceVec = reinterpret_cast<NiPoint3*>(reinterpret_cast<char*>(*vrDataStruct) + 0x158);
			transform->pos -= *worldSpaceVec;
			transform->pos = *worldSpaceMat * transform->pos;
		}
	}

	void VRSystem::initializeDevices() {
		for (vr::TrackedDeviceIndex_t i = 0; i < vr::k_unMaxTrackedDeviceCount; ++i) {
			const auto dc = _vrHook->GetVRSystem()->GetTrackedDeviceClass(i);
			std::string prop = getProperty(vr::ETrackedDeviceProperty::Prop_ModelNumber_String, i);
			if (dc == vr::ETrackedDeviceClass::TrackedDeviceClass_GenericTracker) {
				_viveTrackers[prop] = i;
			} else if (dc == vr::ETrackedDeviceClass::TrackedDeviceClass_Controller) {
				if (prop.find("Right") != std::string::npos) {
					_controllers["Right"] = i;
				} else if (prop.find("Left") != std::string::npos) {
					_controllers["Left"] = i;
				}
			} else if (dc == vr::ETrackedDeviceClass::TrackedDeviceClass_HMD) {
				_controllers["HMD"] = i;
			}
		}
	}

	std::string VRSystem::getProperty(const vr::ETrackedDeviceProperty property, const vr::TrackedDeviceIndex_t idx) const {
		constexpr uint32_t bufSize = vr::k_unMaxPropertyStringSize;
		const auto pchValue = std::make_unique<char[]>(bufSize);
		vr::TrackedPropertyError pError = vr::TrackedPropertyError::TrackedProp_NotYetAvailable;

		_vrHook->GetVRSystem()->GetStringTrackedDeviceProperty(idx, property, pchValue.get(), bufSize, &pError);
		return std::string(pchValue.get());
	}

	void VRSystem::debugPrint() {
		const vr::VRCompositorError error = _vrHook->GetVRCompositor()->GetLastPoses(_renderPoses, vr::k_unMaxTrackedDeviceCount, _gamePoses, vr::k_unMaxTrackedDeviceCount);
		if (error != vr::EVRCompositorError::VRCompositorError_None) {
			_MESSAGE("Error while retrieving game poses!");
		}

		for (const auto& tracker : _viveTrackers) {
			vr::TrackedDevicePose_t renderMat = _renderPoses[tracker.second];
			const vr::TrackedDevicePose_t gameMat = _gamePoses[tracker.second];

			NiTransform renderTran;
			hmdMatrixToNiTransform(&renderTran, &renderMat);

			_MESSAGE("%d : %s --> render = %f %f %f |||| game = %f %f %f", tracker.second, tracker.first.c_str(),
				renderTran.pos.x, renderTran.pos.y, renderTran.pos.z,
				gameMat.mDeviceToAbsoluteTracking.m[0][3], gameMat.mDeviceToAbsoluteTracking.m[1][3], gameMat.mDeviceToAbsoluteTracking.m[2][3]);
		}
	}

	VRSystem* g_vrHook = nullptr;
}
