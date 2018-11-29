#include <windows.h>
#include <iostream>
#include <cstring>
#include <math.h>
struct uinfo {
	WORD secBytes;	//每个扇区字节数
	BYTE cluSec;	//每个簇扇区数
	WORD dbrSec;	//保留区扇区数

	BYTE fatNum;	//fat的数量
	DWORD fatSec;	//单个fat扇区数

	WORD fsinfoSecNum;	//fsinfo扇区号
};

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

	return buffer;
}

bool ChangeFSINFO(char deviceName,DWORD filesize) {
	struct uinfo *deInfo;
	DWORD dwBytes;

	char *device = (char *)malloc(strlen("\\\\.\\C:"));
	sprintf(device, "%s%s%s", "\\\\.\\", deviceName, ":");
	
	HANDLE pFile = CreateFileA(device, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (pFile == INVALID_HANDLE_VALUE)
	{
		printf("create file error!\n");
		CloseHandle(pFile);
		return FALSE;
	}

	char *bdrHead=(char *)malloc(0x1fe);
	//读取bdr的头部
	ReadFile(pFile, bdrHead, 0x1fe, &dwBytes, NULL);
	if (dwBytes!=0x1fe) {
		printf("read dbrHead fail.\n");
		return FALSE;
	}
	//读取dbr区关键信息
	deInfo->secBytes =(WORD)(bdrHead + 0x0b);
	deInfo->cluSec = (BYTE)(bdrHead + 0x0d);
	deInfo->dbrSec = (WORD)(bdrHead + 0x0e);
	deInfo->fatNum = (BYTE)(bdrHead + 0x10);
	deInfo->fatSec = (DWORD)(bdrHead + 0x24);
	deInfo->fsinfoSecNum = (WORD)(bdrHead + 0x30);

	DWORD needClu =(DWORD) ceil((double)(filesize) / (double)(deInfo->cluSec*deInfo->secBytes));
	
	char *fsinfoSec = (char *)malloc(deInfo->secBytes);
	fsinfoSec = bdrHead + deInfo->fsinfoSecNum * deInfo->secBytes;	//定位到fsinfo扇区
	DWORD emptyClu = (DWORD)(fsinfoSec + 0x1e8);
	//判断剩余空间是否足够写文件
	if (emptyClu < needClu) {
		printf("emptyClu is not enough.\n");
		CloseHandle(pFile);
		return FALSE;
	}

	//写空闲簇数
	emptyClu -= needClu;
	WriteFile(pFile,&emptyClu, 4, &dwBytes, 0);
	if (pFile == INVALID_HANDLE_VALUE)
	{
		printf("write emptyClu error!\n");
		CloseHandle(pFile);
		return FALSE;
	}


}
