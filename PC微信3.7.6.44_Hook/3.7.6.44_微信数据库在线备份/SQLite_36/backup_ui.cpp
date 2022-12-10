#include "pch.h"
#include "backup_ui.h"
#include "pch.h"
#include "offsets.h"
#include "wechat.h"
#include <Windows.h>
#include "resource.h"
#include <CommCtrl.h>
#include "utility.h"
#include "backup.h"

string columns;
VOID ShowUI(HMODULE hModule)
{
	//��ȡWeChatWin.dll�Ļ�ַ
	while (baseAddress == 0)
	{
		baseAddress = (DWORD)GetModuleHandle(TEXT("WeChatWin.dll"));
		Sleep(100);
	}

	if (CheckVersion() == FALSE)
	{
		string message;
		message.append("΢�Ű汾��ƥ�䣬��ʹ��");
		message.append(wxVersoin);
		message.append("�汾��");

		MessageBoxA(NULL, message.c_str(), "����", MB_OK);
		WechatUnhook();
		return;
	}

	//�ڵ�¼΢��ǰע��
	int* loginStatus = (int*)(baseAddress + offset_wechat_login);
	if (*loginStatus != 0)
	{
		if (IDOK == MessageBox(NULL, TEXT("΢���Ѿ���¼���ù����޷�ʹ�ã�\n�Ƿ���������΢�ţ�"), TEXT("����"), MB_OKCANCEL | MB_ICONERROR))
		{
			WechatReboot();
		}
		WechatUnhook();
		return;
	}

	//��������
	DialogBox(hModule, MAKEINTRESOURCE(IDD_MAIN), NULL, &DialogProcess);
}

INT_PTR CALLBACK DialogProcess(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	hWinDlg = hwndDlg;
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		//DLL���غ󣬾�����Inline HOOK
		WechatHook();

		HWND hProgress = GetDlgItem(hWinDlg, IDC_PROGRESS);
		//���ý�������Χ
		SendMessageA(hProgress, PBM_SETRANGE, NULL, MAKELPARAM(0, 100));
		//���ý���������
		SendMessageA(hProgress, PBM_SETPOS, 0, 0);
		UpdateWindow(hProgress);

		HWND hStatic = GetDlgItem(hWinDlg, IDC_STATIC_PROGRESS);
		//�����ı�����
		SendMessageA(hStatic, WM_SETTEXT, NULL, (LPARAM)("00.00%"));
		UpdateWindow(hStatic);

		SetSQLText("PRAGMA database_list");
		break;
	}
	case WM_CLOSE:
	{
		//�ָ�HOOK���Ĵ���...ʡ��unhook
		//�ͷ����ڴ��д����ĺ���(lpAddressBackupDB)...ʡ��  
		//���ڴ���ж�ر�dll
		WechatUnhook();
		EndDialog(hwndDlg, 0);
		break;
	}
	case WM_COMMAND:
	{
		//ִ��ѡ��
		if (LOWORD(wParam) == IDC_COMBO_DB)
		{
			if (HIWORD(wParam) == CBN_SELCHANGE)
			{
				//��ȡ���ݿ���
				int index = SendMessageA((HWND)lParam, CB_GETCURSEL, NULL, 0);
				char buf[MAX_PATH] = { 0 };
				SendMessageA((HWND)lParam, CB_GETLBTEXT, index, (LPARAM)buf);

				//����ı���
				HWND edit = GetDlgItem(hWinDlg, IDC_EDIT_LOG);
				SendMessageA(edit, WM_SETTEXT, NULL, NULL);

				//�����־
				string text = "���߱��ݵ����ݿ⣺\r\n";
				text.append(buf);
				text.append("\r\n");
				AddLogUI(text);

				//��ȡ��ѯ�����ݿ���
				string dbName(buf);
				char hexString[12] = { 0 };
				for (auto& db : wx_db_list)
				{
					string dbNameInList(db.name);
					if (dbNameInList == dbName)
					{
						sprintf_s(hexString, "0x%08X", db.handle);
						text = "���ݿ���:\r\n";
						text.append(hexString);
						text.append("\r\n");
						AddLogUI(text);

						//���þ���ı���
						SetHandleText(hexString);
						break;
					}
				}
			}
			break;
		}

		//ˢ��ComboBox
		if (wParam == IDC_BUTTON_REFRESH)
		{
			RefreshComboBox();
			break;
		}

		//ִ�б���
		if (wParam == IDC_BUTTON_SQLRUN)
		{
			BackupButtonClick();
			break;
		}

		//����΢��
		if (wParam == IDC_BUTTON_WX_REBOOT)
		{
			WechatReboot();
			break;
		}

		//���ݿ��ѯ
		if (wParam == IDC_BUTTON_SQL)
		{
			QueryButtonClick();
			break;
		}

		break;
	}
	default:
		break;
	}
	return FALSE;
}

/// <summary>
/// Ϊ���ݿ�Handle�ı�����������
/// </summary>
/// <param name="text"></param>
VOID SetHandleText(std::string text)
{
	HWND edit = GetDlgItem(hWinDlg, IDC_EDIT_SQL_HANDLE);
	SendMessageA(edit, WM_SETTEXT, NULL, (LPARAM)(text.c_str()));
}

VOID SetSQLText(std::string text)
{
	HWND edit = GetDlgItem(hWinDlg, IDC_EDIT_SQL);
	SendMessageA(edit, WM_SETTEXT, NULL, (LPARAM)(text.c_str()));
}

VOID AddLogUI(std::string text)
{
	HWND edit = GetDlgItem(hWinDlg, IDC_EDIT_LOG);

	//��ȡ��ǰ�ı�����ַ�����
	int count = SendMessageA(edit, WM_GETTEXTLENGTH, NULL, NULL);

	//��ȡ��ǰ�ı��������
	char* oldChars = new char[count + 1]{ 0 };
	SendMessageA(edit, WM_GETTEXT, (WPARAM)(count + 1), (LPARAM)oldChars);
	string oldText(oldChars);
	delete[] oldChars;

	//����ַ�
	oldText.append(text);
	SendMessageA(edit, WM_SETTEXT, NULL, (LPARAM)(oldText.c_str()));
}

VOID RefreshComboBox()
{
	//��ֹ ˢ�°�ť
	HWND hRefresh = GetDlgItem(hWinDlg, IDC_BUTTON_REFRESH);
	EnableWindow(hRefresh, FALSE);

	//�ѡ����ݿ���-->���ݿ������������浽CombBox��
	HWND combo1 = GetDlgItem(hWinDlg, IDC_COMBO_DB);

	//ɾ��ȫ������
	while (SendMessage(combo1, CB_DELETESTRING, 0, 0) > 0) {}

	//�������
	for (auto& db : wx_db_list)
	{
		SendMessageA(combo1, CB_ADDSTRING, NULL, (LPARAM)(db.name));
	}
}
VOID QueryButtonClick()
{
	//��ȡ���ݿ���
	HWND edit_handle = GetDlgItem(hWinDlg, IDC_EDIT_SQL_HANDLE);
	char buf[20] = { 0 };
	GetWindowTextA(edit_handle, buf, GetWindowTextLength(edit_handle) + 1);
	if (std::string(buf) == "")
	{
		AddLogUI("������д���ݿ�����\r\n");
		return;
	}
	else
	{
		string message = "���ݿ�����\t";
		message.append(buf);
		message.append("\r\n");
		AddLogUI(message);
	}

	//��ȡSQL���
	HWND edit_sql = GetDlgItem(hWinDlg, IDC_EDIT_SQL);
	DWORD iLength = GetWindowTextLength(edit_sql);
	char* zBuffer;
	if (iLength != 0)
	{
		zBuffer = (char*)malloc(iLength * 2);
		GetWindowTextA(edit_sql, zBuffer, GetWindowTextLength(edit_sql) + 1);

		string message = "SQL��䣺\t";
		message.append(zBuffer);
		message.append("\r\n");
		AddLogUI(message);
	}
	else
	{
		AddLogUI("������дҪ��ѯ��SQL��䣡\r\n");
		return;
	}

	// ִ��SQL��ѯ
	columns = "";
	char* errMsg = NULL;
	RunSQLByHandle(buf, zBuffer, QueryCallback, NULL, &errMsg);
}

VOID BackupButtonClick()
{
	//����ı�������
	HWND edit = GetDlgItem(hWinDlg, IDC_EDIT_LOG);
	SendMessageA(edit, WM_SETTEXT, NULL, NULL);

	//��ȡ���ݿ���
	HWND combo1 = GetDlgItem(hWinDlg, IDC_COMBO_DB);
	int index = SendMessageA(combo1, CB_GETCURSEL, NULL, 0);
	char buf[MAX_PATH] = { 0 };
	SendMessageA(combo1, CB_GETLBTEXT, index, (LPARAM)buf);

	if (std::string(buf) == "")
	{
		AddLogUI("����ѡ��Ҫ���߱��ݵ����ݿ⣡\r\n");
		return;
	}

	//�����־
	std::string text = "���ݿ⣺\r\n";
	text.append(buf);
	text.append("\r\n");
	AddLogUI(text);

	//��ȡ��ѯ�����ݿ���
	std::string dbName(buf);
	char hexString[12] = { 0 };
	for (auto& db : wx_db_list)
	{
		std::string dbNameInList(db.name);
		if (dbNameInList == dbName)
		{
			selectedDbHander = db.handle;
			break;
		}
	}

	if (selectedDbHander == 0)
	{
		AddLogUI("����ѡ��Ҫ���߱��ݵ����ݿ⣡\r\n");
		return;
	}

	//��ȡĬ���ļ���
	TCHAR driver[_MAX_DRIVE] = { 0 };
	TCHAR dir[_MAX_DIR] = { 0 };
	TCHAR fname[_MAX_FNAME] = { 0 };
	TCHAR ext[_MAX_EXT] = { 0 };
	TCHAR szFile[MAX_PATH] = { 0 };
	_wsplitpath_s(StringToWString(dbName).c_str(), driver, _MAX_DRIVE, dir, _MAX_DIR, fname, _MAX_FNAME, ext, _MAX_EXT);
	wstring fileName(fname);
	fileName.append(ext);
	wmemcpy(szFile, fileName.c_str(), fileName.length());

	//����ǰĿ¼
	TCHAR szCurDir[MAX_PATH];
	GetCurrentDirectory(sizeof(szCurDir), szCurDir);

	// �����Ի���ṹ��   
	OPENFILENAME ofn;
	ZeroMemory(&ofn, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = GetForegroundWindow();
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = L"SQL(*.db)\0*.db\0All(*.*)\0*.*\0\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = szCurDir;
	ofn.lpstrDefExt = L"db";
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT | OFN_CREATEPROMPT | OFN_EXTENSIONDIFFERENT;

	// ��ʾ��ѡ���ļ��Ի���
	if (GetSaveFileName(&ofn))
	{
		wstring dumpFile(szFile);
		backupFileName2 = WStringToString(dumpFile);
		string text = "���ݵ����ݿ�:\r\n";
		text.append(backupFileName2);
		text.append("\r\n");
		AddLogUI(text);

		//���̵߳ķ�ʽ����
		//�������治�Ῠ��
		HANDLE hANDLE = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)BackupSQLiteDB, NULL, NULL, 0);
		if (hANDLE != 0)
		{
			CloseHandle(hANDLE);
		}
	}
}

VOID SetRefreshButtonEnable()
{
	HWND hRefresh = GetDlgItem(hWinDlg, IDC_BUTTON_REFRESH);
	EnableWindow(hRefresh, TRUE);
}

VOID UpdateProgressBar(int pos, int all)
{
	HWND hProgress = GetDlgItem(hWinDlg, IDC_PROGRESS);
	//���ý�������Χ
	//SendMessageA(hProgress, PBM_SETRANGE, NULL, MAKELPARAM(0, 100));
	//���ý���������
	SendMessageA(hProgress, PBM_SETPOS, (all - pos) * 100 / all, 0);
	UpdateWindow(hProgress);

	//�����ı�����
	HWND hStatic = GetDlgItem(hWinDlg, IDC_STATIC_PROGRESS);
	char dataPercent[7] = { 0 };
	sprintf_s(dataPercent, 7, "%.2f", (100.0f * (all - pos)) / all);
	string posStr(dataPercent);
	posStr.append("%");
	SendMessageA(hStatic, WM_SETTEXT, NULL, (LPARAM)(posStr.c_str()));
	UpdateWindow(hStatic);
}

int __cdecl QueryCallback(void* para, int nColumn, char** colValue, char** colName)
{
	if (columns == "")
	{
		for (int i = 0; i < nColumn; i++)
		{
			//ָ������������д��
			//printf("%s :%s\n", *(colName + i), colValue[i]);
			columns.append(colName[i]);
			if (i < nColumn - 1)
			{
				columns.append(",");
			}
		}
		columns.append("\r\n");
		//MessageBoxA(NULL, columns.c_str(),"",MB_OK);
		AddLogUI(columns);
	}

	string data;
	for (int i = 0; i < nColumn; i++)
	{
		//ָ������������д��
		//printf("%s :%s\n", *(colName + i), colValue[i]);
		string temp;
		auto aa = colValue[i];
		if (aa == NULL)
		{
			temp = "";
		}
		else
		{
			temp = string(aa);
		}
		data.append(temp);
		if (i < nColumn - 1)
		{
			data.append(",");
		}
	}
	data.append("\r\n");
	//MessageBoxA(NULL, data.c_str(), "", MB_OK);
	AddLogUI(data);
	return 0;
}
