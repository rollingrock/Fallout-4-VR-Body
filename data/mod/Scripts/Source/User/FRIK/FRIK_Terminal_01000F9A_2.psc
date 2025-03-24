;BEGIN FRAGMENT CODE - Do not edit anything between this and the end comment
Scriptname FRIK:FRIK_Terminal_01000F9A_2 Extends Terminal Hidden Const

;BEGIN FRAGMENT Fragment_Terminal_02
Function Fragment_Terminal_02(ObjectReference akTerminalRef)
;BEGIN CODE
FRIK:FRIK.OpenMainConfigurationMode()
;END CODE
EndFunction
;END FRAGMENT

;BEGIN FRAGMENT Fragment_Terminal_03
Function Fragment_Terminal_03(ObjectReference akTerminalRef)
;BEGIN CODE
FRIK:FRIK.OpenPipboyConfigurationMode()
;END CODE
EndFunction
;END FRAGMENT

;BEGIN FRAGMENT Fragment_Terminal_04
Function Fragment_Terminal_04(ObjectReference akTerminalRef)
;BEGIN CODE
FRIK_CONF_RELOAD.SetValue(FRIK:FRIK.ToggleReloadFrikIniConfig())
;END CODE
EndFunction
;END FRAGMENT

;BEGIN FRAGMENT Fragment_Terminal_05
Function Fragment_Terminal_05(ObjectReference akTerminalRef)
;BEGIN CODE
FRIK:FRIK.OpenFrikIniFile()
;END CODE
EndFunction
;END FRAGMENT

;BEGIN FRAGMENT Fragment_Terminal_07
Function Fragment_Terminal_07(ObjectReference akTerminalRef)
;BEGIN CODE
FRIK_WRM.SetValue(FRIK:FRIK.ToggleWeaponRepositionMode())
;END CODE
EndFunction
;END FRAGMENT

;BEGIN FRAGMENT Fragment_Terminal_08
Function Fragment_Terminal_08(ObjectReference akTerminalRef)
;BEGIN CODE
FRIK_WRM.SetValue(FRIK:FRIK.ToggleWeaponRepositionMode())
;END CODE
EndFunction
;END FRAGMENT

;BEGIN FRAGMENT Fragment_Terminal_09
Function Fragment_Terminal_09(ObjectReference akTerminalRef)
;BEGIN CODE
FRIK_CONF_RELOAD.SetValue(FRIK:FRIK.ToggleReloadFrikIniConfig())
;END CODE
EndFunction
;END FRAGMENT

;BEGIN FRAGMENT Fragment_Terminal_11
Function Fragment_Terminal_11(ObjectReference akTerminalRef)
;BEGIN CODE
FRIK_PBSFX.SetValue(1.0)
;END CODE
EndFunction
;END FRAGMENT

;BEGIN FRAGMENT Fragment_Terminal_12
Function Fragment_Terminal_12(ObjectReference akTerminalRef)
;BEGIN CODE
FRIK_PBSFX.SetValue(0.0)
;END CODE
EndFunction
;END FRAGMENT

;END FRAGMENT CODE - Do not edit anything between this and the begin comment

GlobalVariable Property FRIK_PBSFX Auto Const Mandatory
GlobalVariable Property FRIK_WRM Auto Const Mandatory
GlobalVariable Property FRIK_CONF_RELOAD Auto Const Mandatory
