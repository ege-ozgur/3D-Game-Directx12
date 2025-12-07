#pragma once

#define WINDOW_GET_X_LPARAM(lp) ((int)(short)LOWORD(lp))
#define WINDOW_GET_Y_LPARAM(lp) ((int)(short)HIWORD(lp))
#define NOMINMAX
#include <Windows.h>
#include <string>
using namespace std;

class Window {
public:
	HWND hwnd;
	HINSTANCE hinstance;
	string name;
	float height;
	float width;
	bool keys[256];
	int mousex;
	int mousey;
	bool mouseButtons[3];
	void initialize(unsigned int window_height, unsigned int window_width, string window_name);
	Window()
	{
		memset(keys, 0, sizeof(keys));
		memset(mouseButtons, 0, sizeof(mouseButtons));
		mousex = 0;
		mousey = 0;
	}
	void updateMouse(int x, int y)
	{
		mousex = x;
		mousey = y;
	}
	void processMessages();
};


