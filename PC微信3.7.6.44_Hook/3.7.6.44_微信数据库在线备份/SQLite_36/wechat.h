#include <Windows.h>
#include <string>
#include <list>
#pragma once

//����һ���ṹ�����洢 ���ݿ���-->���ݿ���
struct DB_HANDLE_NAME
{
	int handle;
	char name[MAX_PATH];
};

//���ڴ��д洢һ�������ݿ���-->���ݿ�����������
extern std::list<DB_HANDLE_NAME> wx_db_list;
extern const std::string wxVersoin;
extern DWORD hookAddress;
extern DWORD jumpBackAddress;

BOOL CheckVersion();
VOID WechatReboot();
VOID WechatHook();
VOID WechatUnhook();
VOID WechatHookJump();
VOID WechatSQLiteHandle(int dbAddress, int dbHandle);
