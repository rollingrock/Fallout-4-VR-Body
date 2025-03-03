ScriptName FRIK:FRIK extends ScriptObject native hidden

Function saveStates() global native
Function Calibrate() global native
Function makeTaller() global native
Function makeShorter() global native
Function moveCameraUp() global native
Function moveCameraDown() global native
Function moveUp() global native
Function moveDown() global native
Function moveForward() global native
Function moveBackward() global native
Function increaseScale() global native
Function decreaseScale() global native
Function togglePAHUD() global native
Function togglePipboyVis() global native
Function toggleSelfieMode() global native
Function toggleArmsOnlyMode() global native
Function toggleStaticGripping() global native
Function handUiXUp() global native
Function handUiXDown() global native
Function handUiYUp() global native
Function handUiYDown() global native
Function handUiZUp() global native
Function handUiZDown() global native
Function toggleHeadVis() global native
Function toggleRepositionMasterMode() global native
Function toggleDampenHands() global native
Function increaseDampenRotation() global native
Function decreaseDampenRotation() global native
Function increaseDampenTranslation() global native
Function decreaseDampenTranslation() global native
bool Function isLeftHandedMode() global native
int Function RegisterBoneSphere(float radius, string bone) global native
int Function RegisterBoneSphereOffset(float radius, string bone, float[] position) global native
Function DestroyBoneSphere(int handle) global native
Function RegisterForBoneSphereEvents(ScriptObject akObject) global native
Function UnRegisterForBoneSphereEvents(ScriptObject akObject) global native
Function toggleDebugBoneSpheres(bool turnOn) global native
Function toggleDebugBoneSpheresAtBone(int bone, bool turnOn) global native
Function setFingerPositionScalar(bool isLeft, float thumb, float index, float middle, float ring, float pinky) global native
Function restoreFingerPoseControl(bool isLeft) global native
Function dumpGeometryArray() global native