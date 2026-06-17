## Reverse Engineering
Tools:
- [jpexs-decompiler](https://github.com/jindrapetrik/jpexs-decompiler)
- [B.A.E. - Bethesda Archive Extractor](https://www.nexusmods.com/fallout4/mods/78)
 
Steps:
1. Extract the `Data\Fallout4 - Interface.ba2` file using BAE.
2. Open the relevant `.swf` file in `Interface`.
  - `PipboyMenu.swf`
  - `Pipboy_InvPage.swf`
  - etc.
3. Look in the `scripts` folder for classes like `PipboyMenu`, `PipboyPage`, `Pipboy_InvPage`, `StatusTab`, etc.

## General Navigation
```
// changes sub tabs
root->Invoke("root.Menu_mc.gotoNextTab", nullptr, nullptr, 0); 
root->Invoke("root.Menu_mc.gotoPrevTab", nullptr, nullptr, 0);

// changes main tabs
root->Invoke("root.Menu_mc.gotoNextPage", nullptr, nullptr, 0); 
root->Invoke("root.Menu_mc.gotoPrevPage", nullptr, nullptr, 0);
```

### General Information
```
// Returns Current Page Number (0 = STAT, 1 = INV, 2 = DATA, 3 = MAP, 4 = RADIO)
if (root->GetVariable(&PBCurrentPage, "root.Menu_mc.DataObj._CurrentPage")) {   
   X = PBCurrentPage.GetUInt();
}
```

## List Controls
Most of the Pip-Boy UI uses a generic list with the following API, which can be used to navigate and select items.
Most of the work needed to control the Pip-Boy is finding the list path in Scaleform.

### Moving Selected Item Up/Down
```
root->Invoke("root.Menu_mc.CurrentPage.List_mc.moveSelectionUp", nullptr, &event, 1);
root->Invoke("root.Menu_mc.CurrentPage.List_mc.moveSelectionDown", nullptr, &event, 1);
```

### Selecting/Pressing an Item in a List
This is like selecting an item in the inventory, activating a quest, or selecting an item in a context menu message box.
```
GFxValue event;
GFxValue args[3];
args[0].SetString("BSScrollingList::itemPress");
args[1].SetBool(true);
args[2].SetBool(true);
root->CreateObject(&event, "flash.events.Event", args, 3);
root->Invoke("root.Menu_mc.CurrentPage.List_mc.dispatchEvent", nullptr, &event, 1);
```

## Specific Pages/Tabs

### Inventory Tab Controls
```
root->GetVariable("root.Menu_mc.CurrentPage.List_mc", nullptr, nullptr, 0);
```

### Inventory and Radio Tab Controls
```
root->GetVariable("root.Menu_mc.CurrentPage.List_mc", nullptr, nullptr, 0);
```

### Data Tab Controls
```
// Stats page list
root->GetVariable("root.Menu_mc.CurrentPage.StatsTab_mc.CategoryList_mc", nullptr, nullptr, 0);

// Quest page list
root->GetVariable("root.Menu_mc.CurrentPage.QuestsTab_mc.QuestsList_mc", nullptr, nullptr, 0);

// Workshop page list
root->GetVariable("root.Menu_mc.CurrentPage.WorkshopsTab_mc.List_mc", nullptr, nullptr, 0);
```

### STATS TAB CONTROLS
```
root->GetVariable("root.Menu_mc.CurrentPage.PerksTab_mc.List_mc", nullptr, nullptr, 0);
```

```
// Press on Settlements list to navigate to map
GFxValue workshopArgs[2];
workshopArgs[0].SetString("XButton");
workshopArgs[1].SetBool(false);
root->Invoke("root.Menu_mc.CurrentPage.WorkshopsTab_mc.ProcessUserEvent", nullptr, workshopArgs, 2);
```

### Map Tab Controls
```
// Move Map
GFxValue akArgs[2];
akArgs[0]   <- X Value
akArgs[1]   <- Y Value
root->Invoke("root.Menu_mc.CurrentPage.WorldMapHolder_mc.PanMap", nullptr, akArgs, 2)
```

## Context Menu Message Box
The box that pops up with selectable options, usually when pressing the offhand thumbstick.
The context menu also has a generic list inside, but unlike the other lists, it is dynamically created and does not have a static path. This requires finding the list by walking the UI hierarchy from a static path to `MessageHolder`.

### Detect Context Menu Message Box Visible
```
// Is a message box visible
root->GetVariable(&var, "root.Menu_mc.CurrentPage.MessageHolder_mc.visible") && var.IsBool() && var.GetBool()
root->GetVariable(&var, "root.Menu_mc.CurrentPage.QuestsTab_mc.MessageHolder_mc.visible") && var.IsBool() && var.GetBool()
```
