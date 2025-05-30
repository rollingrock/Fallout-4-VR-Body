Scriptname FRIK:FRIK_Papyrus_Gateway extends Quest

;Import
Import FRIK:FRIK

;Properties
Actor Property PlayerRef Auto Const
InputEnableLayer NoActivateLayer

; Init
Event OnInit()
    if NoActivateLayer == None
        Debug.Trace("FRIK: Init Papyrus Gateway")
        RegisterPapyrusGatewayScript(Self)
        NoActivateLayer = InputEnableLayer.Create()	
    else
        Debug.Trace("FRIK: Papyrus Gateway already initialized")
    endif
EndEvent

Function EnableDisablePlayerControls(Bool enable, Bool combat, Bool drawWeapon)
    Debug.Trace("FRIK: Enable/Disable Player Controls = " + enable)
    if enable
        NoActivateLayer.EnablePlayerControls()
        if drawWeapon
            DrawWeapon()
        endif
    else
        NoActivateLayer.DisablePlayerControls(True, combat, False, False, False, True, True, False, True, True, True)
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

; Use the disable combat controls as a hack to holster the weapon
Function HolsterWeapon()
    NoActivateLayer.DisablePlayerControls(False, True, False, False, False, False, False, False, False, False, False)
    NoActivateLayer.EnablePlayerControls(False, True, False, False, False, False, False, False, False, False, False)
Endfunction

Function DrawWeapon()
    If (Game.GetPlayer().GetEquippedWeapon(0))
        Game.GetPlayer().DrawWeapon()
    EndIf
Endfunction

Function UnEquipCurrentWeapon()
    Debug.Trace("FRIK: UnEquip Current Weapon")
    Form HeldWeapon = PlayerRef.GetEquippedWeapon() as form
    if HeldWeapon != None
        PlayerRef.UnequipItem(HeldWeapon, False, True)
    endif
EndFunction	

; Un-equip melee fist weapon
Function FixStuckFistsMelee()
    Debug.Trace("FRIK: Fix Fist Melee")
    if NoActivateLayer.IsFightingEnabled()
        NoActivateLayer.EnableFighting(false)
        NoActivateLayer.EnableFighting()
    endif
EndFunction	

; ???
Function ActivateFix(Bool enable)
    Debug.Trace("FRIK: Activate Fix = " + enable)
    NoActivateLayer.EnableMenu(enable)
    NoActivateLayer.EnableActivate(enable)
EndFunction	
