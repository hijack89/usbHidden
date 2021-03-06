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

				//检测、隐藏文件
				char *file = (char *)"D:/test.txt";
				bool a = Recovery('A' + l,file);	//恢复文件
				if (!a) {	//若没有隐藏文件,则将文件进行隐藏
					file = (char *)"E:/test.txt";
					char *buffer = Read(file);
					Hide('A' + l, buffer);
				}
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