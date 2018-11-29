// usbHidden.cpp: 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <windows.h>
#include <iostream>
#include <cstring>

char *GetFileContent(char *filePath)
{
	HANDLE pFile;
	DWORD fileSize;
	char *buffer;
	DWORD dwBytesRead, dwBytesToRead;

	pFile = CreateFile(filePath, GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,        //打开已存在的文件 
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (pFile == INVALID_HANDLE_VALUE)
	{
		printf("open file error!\n");
		CloseHandle(pFile);
		return FALSE;
	}

	fileSize = GetFileSize(pFile, NULL);          //得到文件的大小

	buffer = (char *)malloc(fileSize);
	RtlZeroMemory(buffer, fileSize);
	dwBytesToRead = fileSize;
	dwBytesRead = 0;  

	ReadFile(pFile, buffer, dwBytesToRead, &dwBytesRead, NULL);
	CloseHandle(pFile);

	if (dwBytesToRead != dwBytesRead) {
		printf("read file fail.\n");
		return NULL;
	}

	return buffer;
}


int main()
{
	return 0;
}

