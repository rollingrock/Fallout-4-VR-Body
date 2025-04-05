ScriptName FRIK:FRIK extends ScriptObject native hidden

Function Calibrate() global native
Function OpenMainConfigurationMode() global native
Function OpenPipboyConfigurationMode() global native
int Function ToggleWeaponRepositionMode() global native
Function OpenFrikIniFile() global native
int Function ToggleReloadFrikIniConfig() global native
int Function GetWeaponRepositionMode() global native
Function toggleSelfieMode() global native
Function setSelfieMode(bool selfieMode) global native
Function moveForward() global native
Function moveBackward() global native
bool Function isLeftHandedMode() global native
Function setDynamicCameraHeight(float dynamicCameraHeight) global native
int Function GetFrikIniAutoReloading() global native
int Function RegisterBoneSphere(float radius, string bone) global native
int Function RegisterBoneSphereOffset(float radius, string bone, float[] position) global native
Function DestroyBoneSphere(int handle) global native
Function RegisterForBoneSphereEvents(ScriptObject akObject) global native
Function UnRegisterForBoneSphereEvents(ScriptObject akObject) global native
Function toggleDebugBoneSpheres(bool turnOn) global native
Function toggleDebugBoneSpheresAtBone(int bone, bool turnOn) global native
Function setFingerPositionScalar(bool isLeft, float thumb, float index, float middle, float ring, float pinky) global native
Function restoreFingerPoseControl(bool isLeft) global native