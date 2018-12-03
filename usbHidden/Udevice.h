#pragma once
#include <windows.h>
#include <iostream>
#include <cstring>
#include <math.h>

struct uinfo {
	char name[6] = { '\0' };
	BYTE cluSec;	//ÿ����������
	WORD dbrSec;	//������������

	BYTE fatNum;	//fat������
	DWORD fatSec;	//����fat������

	WORD fsinfoSecNum;	//fsinfo������

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
		OPEN_EXISTING,        //���Ѵ��ڵ��ļ� 
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
	//��ȡbdr��ͷ��
	offset = 512;
	RtlZeroMemory(&dwBytes, sizeof(dwBytes));
	ReadFile(pFile, bdrHead, offset, &dwBytes, NULL);	//buf����������������
	if (dwBytes != offset) {
		printf("read dbrHead fail.\n");
		CloseHandle(pFile);
		return FALSE;
	}
	//��ȡdbr���ؼ���Ϣ
	deInfo->cluSec = *((BYTE *)(bdrHead + 0x0d));
	deInfo->dbrSec = *((WORD *)(bdrHead + 0x0e));
	deInfo->fatNum = *((BYTE *)(bdrHead + 0x10));
	deInfo->fatSec = *((DWORD *)(bdrHead + 0x24));
	deInfo->fsinfoSecNum = *((WORD *)(bdrHead + 0x30));

	//��ȡfsinfo����
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
	//��ȡ���д���������һ���д�
	deInfo->emptyClu = *((DWORD *)(fsinfoSec + 0x1e8));
	deInfo->nxtClu = *((DWORD *)(fsinfoSec + 0x1ec));

	//д���ļ���Ҫ�Ĵ���
	DWORD needClu = (DWORD)ceil((double)strlen(buffer) / (double)(deInfo->cluSec * 512));
	//�ж�ʣ��ռ��Ƿ��㹻д�ļ�
	if (deInfo->emptyClu < needClu) {
		printf("emptyClu is not enough.\n");
		CloseHandle(pFile);
		return FALSE;
	}

	//�޸Ŀ��д���
	*((DWORD *)(fsinfoSec + 0x1e8)) = deInfo->emptyClu - needClu;

	//�޸���һ���ô�
	*((DWORD *)(fsinfoSec + 0x1ec)) = deInfo->nxtClu + needClu;

	//��ԭʼ��һ���ôغź��ļ����ȱ��ݽ������ֶ�,������ȡ�ļ�
	*((DWORD *)(fsinfoSec + 0x04)) = deInfo->nxtClu;
	*((DWORD *)(fsinfoSec + 0x08)) = needClu;
	*((DWORD *)(fsinfoSec + 0x0c)) = (DWORD)strlen(buffer);

	//��дfsinfo����
	SetFilePointer(pFile, offset, 0, FILE_BEGIN);
	RtlZeroMemory(&dwBytes, sizeof(dwBytes));
	WriteFile(pFile, fsinfoSec, offset, &dwBytes, 0);
	if (dwBytes != offset)
	{
		printf("write fsinfo error!\n");
		CloseHandle(pFile);
		return FALSE;
	}

	//�޸�����fat��
	char *fat = (char *)malloc(deInfo->fatSec * 512);
	for (int i = 0; i < deInfo->fatNum; i++) {
		offset = deInfo->dbrSec * 512 + deInfo->fatSec * 512 * i;
		SetFilePointer(pFile, offset, 0, FILE_BEGIN);	//��λ��fat������һ���д�״̬�ַ����ڵ�����
		offset = deInfo->fatSec * 512;
		RtlZeroMemory(&dwBytes, sizeof(dwBytes));
		ReadFile(pFile, fat, offset, &dwBytes, NULL);

		DWORD state = 0xfffffff7;
		for (int j = 0; j < needClu; j++) {
			*(DWORD *)(fat + deInfo->nxtClu * 4 + j * 4) = state;
		}

		offset = deInfo->dbrSec * 512 + deInfo->fatSec * 512 * i;
		SetFilePointer(pFile, offset, 0, FILE_BEGIN);	//��λ��fat������һ���д�״̬�ַ����ڵ�����
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

	//�޸�����
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
	//��ȡbdr��ͷ��
	offset = 512;
	RtlZeroMemory(&dwBytes, sizeof(dwBytes));
	ReadFile(pFile, bdrHead, offset, &dwBytes, NULL);	//buf����������������
	if (dwBytes != offset) {
		printf("read dbrHead fail.\n");
		CloseHandle(pFile);
		return FALSE;
	}
	//��ȡdbr���ؼ���Ϣ
	deInfo->cluSec = *((BYTE *)(bdrHead + 0x0d));
	deInfo->dbrSec = *((WORD *)(bdrHead + 0x0e));
	deInfo->fatNum = *((BYTE *)(bdrHead + 0x10));
	deInfo->fatSec = *((DWORD *)(bdrHead + 0x24));
	deInfo->fsinfoSecNum = *((WORD *)(bdrHead + 0x30));

	//��ȡfsinfo����
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
