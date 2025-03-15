Scriptname FRIK:FRIK_Holotape extends ReferenceAlias

GlobalVariable Property FRIK_PBSFX Auto Const Mandatory
GlobalVariable Property FRIK_WRM Auto Const Mandatory
GlobalVariable Property FRIK_CONF_RELOAD Auto Const Mandatory
Holotape Property PFRIK_Holotape Auto Const Mandatory
ObjectReference Property PlayerRef Auto Const Mandatory
Sound Property UIPipBoyOKPress Auto Const
Sound Property UIPipBoyFavoriteOn Auto Const Mandatory

Event OnInit()
    PlayerRef.AddItem(PFRIK_Holotape, 1, true)
    RegisterForMenuOpenCloseEvent("PipboyMenu")
EndEvent

Event OnPlayerLoadGame()
    if (PlayerRef.GetItemCount(PFRIK_Holotape) == 0)
        PlayerRef.AddItem(PFRIK_Holotape, 1, true)
    Endif
    RegisterForMenuOpenCloseEvent("PipboyMenu")
EndEvent

Event OnMenuOpenCloseEvent(string asMenuName, bool abOpening)
    if (asMenuName== "PipboyMenu")
        if (FRIK_PBSFX.GetValue()==0.0)
            int instanceID = UIPipBoyOKPress.play(PlayerRef)
            int instanceID2 = UIPipBoyFavoriteOn.play(PlayerRef)
        endif
        ; Hack to make sure values are fresh before settings holotape is loaded
        FRIK_WRM.SetValue(FRIK:FRIK.GetWeaponRepositionMode())
        FRIK_CONF_RELOAD.SetValue(FRIK:FRIK.GetFrikIniAutoReloading())
    endif
endEvent
