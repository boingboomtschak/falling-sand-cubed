#pragma once

#ifndef FS_GUI_HDR
#define FS_GUI_HDR

namespace FSGui {

const char* AboutText =
"FSC is a game based upon the falling sand/powder style games of the late 2000s. \
As with most falling sand games, it involves a particle simulation where each \
particle is of a specific type of element, and the interactions between these elements \
creates some interesting emergent behavior. This iteration of the falling sand game \
involves some major features including 3-dimensional voxels, fast simulation computation \
with OpenGL compute shaders, and a heavy focus on user interaction and customizability.\n\n\
This game was created as a final project for the Graphics class at Seattle University in the fall \
of 2021 by Devon McKee and Michael Pablo.";

const char* ElementsAirText = 
"Air, while technically being an element, denotes an empty space in the grid. Placing \
air in an area will remove any elements in the brush area.";

const char* ElementsStoneText = 
"Stone is the first immovable solid in the simulation. Place it and drop some other \
movable elements nearby to watch them filter around it.";

const char* ElementsWaterText = 
"Water is a liquid with a higher dispersal rate than usual. It can displace gases, \
fill areas, and mix with other liquids, like oil.";

const char* ElementsSandText = 
"Sand is the first movable solid. It can displace liquids, gases, and piles up in \
pyramid-like shapes. Use it to create hills and liquid channels.";

const char* ElementsOilText = 
"Oil is a liquid with a slower dispersal rate, and does not displace other liquid \
elements, such as water.";

const char* ElementsSaltText = 
"Salt is a movable solid, similar to sand, but provides an interesting change of \
color with which to create new hills and liquid channels.";

const char* ElementsSteamText = 
"Steam is a gas which constantly attemps to move upwards, and moves sideways when \
encountering any other element it cannot displace.";

};

#endif // FS_GUI_HDR