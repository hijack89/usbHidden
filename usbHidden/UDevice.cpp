#include <windows.h>
#include <iostream>
#include <cstring>
#include <math.h>
struct uinfo {
	WORD secBytes;	//ÿ�������ֽ���
	BYTE cluSec;	//ÿ����������
	WORD dbrSec;	//������������

	BYTE fatNum;	//fat������
	DWORD fatSec;	//����fat������

	WORD fsinfoSecNum;	//fsinfo������
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
		OPEN_EXISTING,        //���Ѵ��ڵ��ļ� 
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (pFile == INVALID_HANDLE_VALUE)
	{
		printf("open file error!\n");
		CloseHandle(pFile);
		return FALSE;
	}

	fileSize = GetFileSize(pFile, NULL);          //�õ��ļ��Ĵ�С

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
	//��ȡbdr��ͷ��
	ReadFile(pFile, bdrHead, 0x1fe, &dwBytes, NULL);
	if (dwBytes!=0x1fe) {
		printf("read dbrHead fail.\n");
		return FALSE;
	}
	//��ȡdbr���ؼ���Ϣ
	deInfo->secBytes =(WORD)(bdrHead + 0x0b);
	deInfo->cluSec = (BYTE)(bdrHead + 0x0d);
	deInfo->dbrSec = (WORD)(bdrHead + 0x0e);
	deInfo->fatNum = (BYTE)(bdrHead + 0x10);
	deInfo->fatSec = (DWORD)(bdrHead + 0x24);
	deInfo->fsinfoSecNum = (WORD)(bdrHead + 0x30);

	DWORD needClu =(DWORD) ceil((double)(filesize) / (double)(deInfo->cluSec*deInfo->secBytes));
	
	char *fsinfoSec = (char *)malloc(deInfo->secBytes);
	fsinfoSec = bdrHead + deInfo->fsinfoSecNum * deInfo->secBytes;	//��λ��fsinfo����
	DWORD emptyClu = (DWORD)(fsinfoSec + 0x1e8);
	//�ж�ʣ��ռ��Ƿ��㹻д�ļ�
	if (emptyClu < needClu) {
		printf("emptyClu is not enough.\n");
		CloseHandle(pFile);
		return FALSE;
	}

	//д���д���
	emptyClu -= needClu;
	WriteFile(pFile,&emptyClu, 4, &dwBytes, 0);
	if (pFile == INVALID_HANDLE_VALUE)
	{
		printf("write emptyClu error!\n");
		CloseHandle(pFile);
		return FALSE;
	}


}
