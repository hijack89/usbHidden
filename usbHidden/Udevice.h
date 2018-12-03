#pragma once
#include <windows.h>
#include <iostream>
#include <cstring>
#include <math.h>

struct uinfo {
	char name[6] = { '\0' };
	BYTE cluSec;	//每个簇扇区数
	WORD dbrSec;	//保留区扇区数

	BYTE fatNum;	//fat的数量
	DWORD fatSec;	//单个fat扇区数

	WORD fsinfoSecNum;	//fsinfo扇区号

	DWORD emptyClu;
	DWORD nxtClu;

};

char *Read(char *filePath)
{
	HANDLE pFile;
	char *buffer;
	DWORD dwBytes;
	DWORD fileSize;

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
	fileSize = GetFileSize(pFile, NULL);

	buffer = (char *)malloc(fileSize);
	RtlZeroMemory(buffer, fileSize);
	dwBytes = 0;

	ReadFile(pFile, buffer, fileSize, &dwBytes, NULL);
	if (dwBytes != fileSize) {
		printf("read file fail.\n");
		return FALSE;
	}
	CloseHandle(pFile);

	return buffer;
}

bool Hide(char deviceName, char *buffer) {
	printf("hide file.\n");

	DWORD dwBytes;
	struct uinfo *deInfo = (struct uinfo *)malloc(sizeof(struct uinfo));
	RtlZeroMemory(deInfo, 512);
	strcpy(deInfo->name, "\\\\.\\");
	strcat(deInfo->name, &deviceName);
	deInfo->name[5] = 58;

	DWORD offset;

	HANDLE pFile = CreateFileA(deInfo->name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (pFile == INVALID_HANDLE_VALUE)
	{
		printf("create ufile error!\n");
		CloseHandle(pFile);
		return FALSE;
	}
	RtlZeroMemory(&dwBytes, sizeof(dwBytes));
	BOOL bLOCK = DeviceIoControl(
		pFile,
		FSCTL_DISMOUNT_VOLUME,
		NULL,
		0,
		NULL,
		0,
		&dwBytes,
		NULL
	);
	if (!bLOCK) {
		printf("lock disk error.\n");
		CloseHandle(pFile);
		return FALSE;
	}

	char *bdrHead = (char *)malloc(0x1fe);
	RtlZeroMemory(bdrHead, 512);
	//读取bdr的头部
	offset = 512;
	RtlZeroMemory(&dwBytes, sizeof(dwBytes));
	ReadFile(pFile, bdrHead, offset, &dwBytes, NULL);	//buf必须是扇区整数倍
	if (dwBytes != offset) {
		printf("read dbrHead fail.\n");
		CloseHandle(pFile);
		return FALSE;
	}
	//读取dbr区关键信息
	deInfo->cluSec = *((BYTE *)(bdrHead + 0x0d));
	deInfo->dbrSec = *((WORD *)(bdrHead + 0x0e));
	deInfo->fatNum = *((BYTE *)(bdrHead + 0x10));
	deInfo->fatSec = *((DWORD *)(bdrHead + 0x24));
	deInfo->fsinfoSecNum = *((WORD *)(bdrHead + 0x30));

	//读取fsinfo扇区
	char *fsinfoSec = (char *)malloc(512);
	offset = deInfo->fsinfoSecNum * 512;
	SetFilePointer(pFile, offset, 0, FILE_BEGIN);
	offset = 512;
	RtlZeroMemory(&dwBytes, sizeof(dwBytes));
	ReadFile(pFile, fsinfoSec, offset, &dwBytes, NULL);
	if (dwBytes != offset) {
		printf("read fsinfo error!\n");
		CloseHandle(pFile);
		return FALSE;
	}
	//读取空闲簇数量和下一空闲簇
	deInfo->emptyClu = *((DWORD *)(fsinfoSec + 0x1e8));
	deInfo->nxtClu = *((DWORD *)(fsinfoSec + 0x1ec));

	//写入文件需要的簇数
	DWORD needClu = (DWORD)ceil((double)strlen(buffer) / (double)(deInfo->cluSec * 512));
	//判断剩余空间是否足够写文件
	if (deInfo->emptyClu < needClu) {
		printf("emptyClu is not enough.\n");
		CloseHandle(pFile);
		return FALSE;
	}

	//修改空闲簇数
	*((DWORD *)(fsinfoSec + 0x1e8)) = deInfo->emptyClu - needClu;

	//修改下一可用簇
	*((DWORD *)(fsinfoSec + 0x1ec)) = deInfo->nxtClu + needClu;

	//将原始下一可用簇号和文件长度备份进保留字段,便于提取文件
	*((DWORD *)(fsinfoSec + 0x04)) = deInfo->nxtClu;
	*((DWORD *)(fsinfoSec + 0x08)) = needClu;
	*((DWORD *)(fsinfoSec + 0x0c)) = (DWORD)strlen(buffer);

	//重写fsinfo扇区
	SetFilePointer(pFile, offset, 0, FILE_BEGIN);
	RtlZeroMemory(&dwBytes, sizeof(dwBytes));
	WriteFile(pFile, fsinfoSec, offset, &dwBytes, 0);
	if (dwBytes != offset)
	{
		printf("write fsinfo error!\n");
		CloseHandle(pFile);
		return FALSE;
	}

	//修改所有fat表
	char *fat = (char *)malloc(deInfo->fatSec * 512);
	for (int i = 0; i < deInfo->fatNum; i++) {
		offset = deInfo->dbrSec * 512 + deInfo->fatSec * 512 * i;
		SetFilePointer(pFile, offset, 0, FILE_BEGIN);	//定位到fat表中下一空闲簇状态字符所在的扇区
		offset = deInfo->fatSec * 512;
		RtlZeroMemory(&dwBytes, sizeof(dwBytes));
		ReadFile(pFile, fat, offset, &dwBytes, NULL);

		DWORD state = 0xfffffff7;
		for (int j = 0; j < needClu; j++) {
			*(DWORD *)(fat + deInfo->nxtClu * 4 + j * 4) = state;
		}

		offset = deInfo->dbrSec * 512 + deInfo->fatSec * 512 * i;
		SetFilePointer(pFile, offset, 0, FILE_BEGIN);	//定位到fat表中下一空闲簇状态字符所在的扇区
		RtlZeroMemory(&dwBytes, sizeof(dwBytes));
		offset = deInfo->fatSec * 512;
		WriteFile(pFile, fat, offset, &dwBytes, 0);
		if (dwBytes != offset)
		{
			printf("write state error!\n");
			CloseHandle(pFile);
			return FALSE;
		}
	}

	//修改数据
	char *data = (char *)malloc(512 * deInfo->cluSec*needClu);
	offset = deInfo->dbrSec * 512 + deInfo->fatNum*deInfo->fatSec * 512 + (deInfo->nxtClu - 2)*deInfo->cluSec * 512;
	SetFilePointer(pFile, offset, 0, FILE_BEGIN);
	RtlZeroMemory(&dwBytes, sizeof(dwBytes));
	ReadFile(pFile, data, 512 * deInfo->cluSec*needClu, &dwBytes, 0);
	if (dwBytes != 512 * deInfo->cluSec*needClu)
	{
		printf("read data error!\n");
		CloseHandle(pFile);
		return FALSE;
	}

	strcpy(data, buffer);
	SetFilePointer(pFile, offset, 0, FILE_BEGIN);
	RtlZeroMemory(&dwBytes, sizeof(dwBytes));
	WriteFile(pFile, data, 512 * deInfo->cluSec*needClu, &dwBytes, 0);
	if (dwBytes != 512 * deInfo->cluSec*needClu)
	{
		printf("write data error!\n");
		CloseHandle(pFile);
		return FALSE;
	}

	CloseHandle(pFile);
	printf("hide file success.\n");
	return TRUE;
}

bool Recovery(char deviceName,char *filename) {
	printf("recover file.\n");

	DWORD dwBytes;
	struct uinfo *deInfo = (struct uinfo *)malloc(sizeof(struct uinfo));
	RtlZeroMemory(deInfo, 512);
	strcpy(deInfo->name, "\\\\.\\");
	strcat(deInfo->name, &deviceName);
	deInfo->name[5] = 58;

	DWORD offset;

	HANDLE pFile = CreateFileA(deInfo->name, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, NULL, NULL);
	if (pFile == INVALID_HANDLE_VALUE)
	{
		printf("create ufile error!\n");
		CloseHandle(pFile);
		return FALSE;
	}

	BOOL bLOCK = DeviceIoControl(
		pFile,
		FSCTL_DISMOUNT_VOLUME,
		NULL,
		0,
		NULL,
		0,
		&dwBytes,
		NULL
	);
	if (!bLOCK) {
		printf("lock disk error.\n");
		CloseHandle(pFile);
		return FALSE;
	}

	char *bdrHead = (char *)malloc(0x1fe);
	RtlZeroMemory(bdrHead, 512);
	//读取bdr的头部
	offset = 512;
	RtlZeroMemory(&dwBytes, sizeof(dwBytes));
	ReadFile(pFile, bdrHead, offset, &dwBytes, NULL);	//buf必须是扇区整数倍
	if (dwBytes != offset) {
		printf("read dbrHead fail.\n");
		CloseHandle(pFile);
		return FALSE;
	}
	//读取dbr区关键信息
	deInfo->cluSec = *((BYTE *)(bdrHead + 0x0d));
	deInfo->dbrSec = *((WORD *)(bdrHead + 0x0e));
	deInfo->fatNum = *((BYTE *)(bdrHead + 0x10));
	deInfo->fatSec = *((DWORD *)(bdrHead + 0x24));
	deInfo->fsinfoSecNum = *((WORD *)(bdrHead + 0x30));

	//读取fsinfo扇区
	char *fsinfoSec = (char *)malloc(512);
	offset = deInfo->fsinfoSecNum * 512;
	SetFilePointer(pFile, offset, 0, FILE_BEGIN);
	offset = 512;
	RtlZeroMemory(&dwBytes, sizeof(dwBytes));
	ReadFile(pFile, fsinfoSec, offset, &dwBytes, NULL);
	if (dwBytes != offset) {
		printf("read fsinfo error!\n");
		CloseHandle(pFile);
		return FALSE;
	}

	DWORD nxtClu = *((DWORD *)(fsinfoSec + 0x04));
	DWORD needClu = *((DWORD *)(fsinfoSec + 0x08));
	DWORD fileSize = *((DWORD *)(fsinfoSec + 0x0c));
	if (!needClu || !fileSize) {
		printf("no file hidden\n");
		CloseHandle(pFile);
		return FALSE;
	}

	char *data = (char *)malloc(512 * deInfo->cluSec*needClu);
	offset = deInfo->dbrSec * 512 + deInfo->fatNum*deInfo->fatSec * 512 + (nxtClu - 2)*deInfo->cluSec * 512;
	SetFilePointer(pFile, offset, 0, FILE_BEGIN);
	RtlZeroMemory(&dwBytes, sizeof(dwBytes));
	ReadFile(pFile, data, 512 * deInfo->cluSec*needClu, &dwBytes, 0);
	if (dwBytes != 512 * deInfo->cluSec*needClu)
	{
		printf("read data error!\n");
		CloseHandle(pFile);
		return FALSE;
	}
	CloseHandle(pFile);

	
	HANDLE file = CreateFileA(filename, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, CREATE_ALWAYS, NULL, NULL);
	if (pFile == INVALID_HANDLE_VALUE)
	{
		printf("create file error!\n");
		CloseHandle(file);
		return FALSE;
	}

	RtlZeroMemory(&dwBytes, sizeof(dwBytes));
	WriteFile(file, data, fileSize, &dwBytes, 0);
	if(dwBytes != fileSize)
	{
		printf("write file error!\n");
		CloseHandle(file);
		return FALSE;
	}
	CloseHandle(file);
	printf("recover file success.\n");
	return TRUE;
}
