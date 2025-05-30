ScriptName FRIK:FRIK extends ScriptObject native hidden

; Used to call Papyrus functions from C++ code
Function RegisterPapyrusGatewayScript(ScriptObject akObject) global native

; Used by FRIK Settings holotape
Function OpenMainConfigurationMode() global native
Function OpenPipboyConfigurationMode() global native
int Function ToggleWeaponRepositionMode() global native
Function OpenFrikIniFile() global native
int Function GetWeaponRepositionMode() global native

; External API used by VirtualHolsters
Function toggleSelfieMode() global native
Function setSelfieMode(bool selfieMode) global native
Function moveForward() global native
Function moveBackward() global native
bool Function isLeftHandedMode() global native
Function setDynamicCameraHeight(float dynamicCameraHeight) global native
Function setFingerPositionScalar(bool isLeft, float thumb, float index, float middle, float ring, float pinky) global native
Function restoreFingerPoseControl(bool isLeft) global native

; Bone Spheres API
Function RegisterForBoneSphereEvents(ScriptObject akObject) global native
Function UnRegisterForBoneSphereEvents(ScriptObject akObject) global native
int Function RegisterBoneSphere(float radius, string bone) global native
int Function RegisterBoneSphereOffset(float radius, string bone, float[] position) global native
Function DestroyBoneSphere(int handle) global native
Function toggleDebugBoneSpheres(bool turnOn) global native
Function toggleDebugBoneSpheresAtBone(int bone, bool turnOn) global native
