Scriptname FRIK:FRIK_Holotape extends ReferenceAlias

GlobalVariable Property FRIK_PBSFX Auto Const Mandatory
GlobalVariable Property FRIK_WRM Auto Const Mandatory
Holotape Property PFRIK_Holotape Auto Const Mandatory
ObjectReference Property PlayerRef Auto Const Mandatory
Sound Property UIPipBoyOKPress Auto Const
Sound Property UIPipBoyFavoriteOn Auto Const Mandatory

Event OnInit()
    Debug.Trace("FRIK: Init Holotape on OnInit")
    PlayerRef.AddItem(PFRIK_Holotape, 1, true)
    RegisterForMenuOpenCloseEvent("PipboyMenu")
EndEvent

Event OnPlayerLoadGame()
    Debug.Trace("FRIK: Init holotape handling on OnPlayerLoadGame")
    if (PlayerRef.GetItemCount(PFRIK_Holotape) == 0)
        Debug.Trace("FRIK: Add Holotape item to player")
        PlayerRef.AddItem(PFRIK_Holotape, 1, true)
    Endif
    RegisterForMenuOpenCloseEvent("PipboyMenu")
EndEvent

Event OnMenuOpenCloseEvent(string asMenuName, bool abOpening)
    if (asMenuName== "PipboyMenu")
        Debug.Trace("FRIK: Pipboy menu opened/closed")
        if (FRIK_PBSFX.GetValue()==0.0)
            int instanceID = UIPipBoyOKPress.play(PlayerRef)
            int instanceID2 = UIPipBoyFavoriteOn.play(PlayerRef)
        endif
        if (abOpening)
            ; Hack to make sure values are fresh before settings holotape is loaded
            FRIK_WRM.SetValue(FRIK:FRIK.GetWeaponRepositionMode())
        endif
    endif
endEvent
