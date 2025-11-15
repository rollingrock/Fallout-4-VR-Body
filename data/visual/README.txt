How to use pre-configured .DDS and .nif files for VR UI
-------------------------------------------------------

The "Textures\ui_buttons_grid.DDS" and "Textures\ui_messages_grid.DDS" are templates for UI buttons and messages.
Use them to create a feature specific UI texture file.

Buttons:
The "ui_btn_*x*.nif" files are pre-configured to select the specific button in "Textures\ui_buttons_grid.DDS" texture.
"ui_btn_2x3.nif" configured for the second row, third column button.

Messages: 
The "ui_msg_*x*.nif" files are pre-configured to select the specific message box in "Textures\ui_messages_grid.DDS" texture.
"ui_msg_4x1.nif" configured for the forth row, first column message box.
"ui_title_1.nif" configured for the most bottom title text.

Update Source Texture:
1. Open in NifSkope
2. In Block List tree find the third element: "2 BDEffectShaderProperty"
3. In Block Details table find "Source Texture"
4. Update it to the location relevant for the mod/feature. Example: "textures\FRIK\ui_weapon_adjustment.DDS"


TIP: combining textures
If the feature doesn't require more than 10 buttons
You can take the first two rows from "ui_buttons_grid.DDS" and starting from 4th row in "ui_messages_grid.DDS"
Paste into a new texture file while keeping the original positions


Advance: changing nif texture offset
1. Open in NifSkope
2. In Block List tree right click on the second element: "1 BSTriShape"
3. Click "Texture > Edit UV"
4. Move the green frame to the desired texture


Advance: Changing element size
If changing the nif texture offset changed the proportions of the shape. i.e. the height to width ratio, you have to update the vortexes to reflect it.
Note: only the proportions are important as the shape is always scaled.
1. Open in NifSkope
2. In Block List tree right click on the second element: "1 BSTriShape"
3. In Block Details table find "Vortex Data"
4. For each of the 4 vortexes update the "Y" value to the "width/height" scale value
4.1. i.e. if the shape width in texture is 450 width and 50 height the Y values is 9 
4.1. Make sure not to change the sign of "Y"



TIP: NifSkope resources path
For NifScope tool to show the texture in the nif it needs to find the .DDS file referenced in "Source Texture"
1. In NifScope open Settings
2. Click "Resources" tab
3. Add folder to the base of the mod in your repo. Example: "MyMod\data\mod"