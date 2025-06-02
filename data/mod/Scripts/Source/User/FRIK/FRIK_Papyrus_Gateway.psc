Scriptname FRIK:FRIK_Papyrus_Gateway extends Quest

;Import
Import FRIK:FRIK

;Properties
Actor Property PlayerRef Auto Const
InputEnableLayer NoActivateLayer

; Init
Event OnInit()
    ; the already initialized doesn't really work, but the OnInit is only called on start and game save load
    if NoActivateLayer == None
        Debug.Trace("FRIK: Init Papyrus Gateway")
        ; Register is idempotence so it's not a problem OnInit is called multiple times
        RegisterPapyrusGatewayScript(Self)
        NoActivateLayer = InputEnableLayer.Create()	
    else
        Debug.Trace("FRIK: Papyrus Gateway already initialized")
    endif
EndEvent

; Just enable everything
; drawWeapon - if true will draw the equipped weapon (useful if disableWeapon was used calling DisablePlayerControls)
Function EnablePlayerControls(Bool drawWeapon)
    Debug.Trace("FRIK: Enable Player Controls, drawWeapon = " + drawWeapon)
    NoActivateLayer.EnablePlayerControls()
    PlayerRef.SetRestrained(False)
    if drawWeapon
        DrawWeapon()
    endif
EndFunction	

; Unfortunately Bethesda sucks, just disabling player controls doesn't prevent rotation and jumping.
; Therefore, SetRestrained is used but it also prevents any weapon use so we allow a flag to control it.
; SetRestrained doesn't prevent the use of VATS and Favorites, so we still need DisablePlayerControls.
; If restrain is false the code needs to implement a different way to prevent player rotation, jumping, and sneaking.
; disableFighting - if true will holster the weapon and prevent its use
; restrain - will prevent player movement, rotation, jumping, sneaking, and weapon use (without holstering)
; DisablePlayerControls 11 arg flags: (abMovement, abFighting, abCamSwitch, abLooking, abSneaking, abMenu, abActivate, abJournalTabs, abVATS, abFavorites, abRunning)
; Note: disabling abMovement causes back-of-hand UI to be hidden
Function DisablePlayerControls(Bool disableFighting, Bool restrain)
    Debug.Trace("FRIK: Disable Player Controls, disableFighting = " + disableFighting)
    if !disableFighting
        ; enable fighting if we don't want to disable it now and maybe it was disabled before
        NoActivateLayer.EnablePlayerControls(False, True, False, False, False, False, False, False, False, False, False)
    endif
    NoActivateLayer.DisablePlayerControls(False, disableFighting, True, True, False, True, True, False, True, True, True)
    PlayerRef.SetRestrained(restrain)
EndFunction	

Function EnableDisableFighting(Bool enable, Bool drawWeapon)
    Debug.Trace("FRIK: Enable/Disable Fighting = " + enable)
    NoActivateLayer.EnableFighting(enable)
    if drawWeapon
        DrawWeapon()
    endif
EndFunction	

Function EnableDisableVats(Bool enable)
    Debug.Trace("FRIK: Enable/Disable VATS = " + enable)
    NoActivateLayer.EnableVATS(enable)
EndFunction	

Function EnableDisableFavorites(Bool enable)
    Debug.Trace("FRIK: Enable/Disable Favorites = " + enable)
    NoActivateLayer.EnableFavorites(enable) 
EndFunction	

; Use the disable fighting controls as a hack to holster the weapon
Function HolsterWeapon()
    NoActivateLayer.DisablePlayerControls(False, True, False, False, False, False, False, False, False, False, False)
    NoActivateLayer.EnablePlayerControls(False, True, False, False, False, False, False, False, False, False, False)
Endfunction

Function DrawWeapon()
    If (Game.GetPlayer().GetEquippedWeapon(0))
        Game.GetPlayer().DrawWeapon()
    EndIf
Endfunction

; Fully un-equip the current weapon, it cannot be drawn after this but needs to be equipped again.
Function UnEquipCurrentWeapon()
    Debug.Trace("FRIK: UnEquip Current Weapon")
    Form HeldWeapon = PlayerRef.GetEquippedWeapon() as form
    if HeldWeapon != None
        PlayerRef.UnequipItem(HeldWeapon, False, True)
    endif
EndFunction	

; Un-equip melee fist weapon. To handle having the player stuck in fist melee fighting mode.
Function FixStuckFistsMelee()
    Debug.Trace("FRIK: Fix Fist Melee")
    if NoActivateLayer.IsFightingEnabled()
        NoActivateLayer.EnableFighting(false)
        NoActivateLayer.EnableFighting()
    endif
EndFunction	

; Enable/Disable player from activating objects, NPCs, and containers.
Function ActivateFix(Bool enable)
    Debug.Trace("FRIK: Activate Fix = " + enable)
    NoActivateLayer.EnableMenu(enable)
    NoActivateLayer.EnableActivate(enable)
EndFunction	
