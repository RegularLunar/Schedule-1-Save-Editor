#ifndef DRAWING_H
#define DRAWING_H

#include "pch.h"

class Drawing
{
private:
	static LPCSTR lpWindowName;
	static ImVec2 vMainWindowSize;
	static ImVec2 vSecondaryWindowSize;
	static ImGuiWindowFlags WindowFlags;
	static bool bDraw;

public:
	static void Active();
	static bool isActive();
	static void Draw();

};

#endif
