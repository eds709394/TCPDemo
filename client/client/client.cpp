// client.cpp: 定义控制台应用程序的入口点。
//

#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include "../../XYSocket.h"

#include <tchar.h>
//---------------------------------------------------------------------------
VOID WriteLog(const TCHAR *filename, const TCHAR *string)
{
	HANDLE hfile;
	DWORD numberofbytes;
	CHAR multibytes[1024];
	UINT k;

	hfile = CreateFile(filename, GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hfile != INVALID_HANDLE_VALUE)
	{
		SetFilePointer(hfile, 0, NULL, FILE_END);

		k = WideCharToMultiByte(CP_ACP, 0, string, _tcslen(string), multibytes, 1024, NULL, NULL);
		multibytes[k] = _T('\0');

		WriteFile(hfile, multibytes, k, &numberofbytes, NULL);
		WriteFile(hfile, "\r\n", 2, &numberofbytes, NULL);

		CloseHandle(hfile);
	}
}

BYTE *ReadBuffer(const TCHAR *filename, int *length)
{
	HANDLE hfile;
	DWORD numberofbytes;
	BYTE *buffer = NULL;

	hfile = CreateFile(filename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hfile != INVALID_HANDLE_VALUE)
	{
		numberofbytes = GetFileSize(hfile, NULL);

		buffer = (BYTE *)MALLOC(numberofbytes);
		if (buffer != NULL)
		{
			ReadFile(hfile, buffer, numberofbytes, &numberofbytes, NULL);

			*length = numberofbytes;
		}

		CloseHandle(hfile);
	}
	return(buffer);
}
//---------------------------------------------------------------------------
typedef struct tagCLIENT_CONTEXT
{
	HANDLE hevent;

	DWORD tickcount;
}CLIENT_CONTEXT, *PCLIENT_CONTEXT;
//---------------------------------------------------------------------------
int CALLBACK SocketProcedure(LPVOID parameter, LPVOID **pointer, LPVOID context, SOCKET s, BYTE type, BYTE number, SOCKADDR *psa, int *salength, const char *buffer, int length)
{
	PXYSOCKET ps = (PXYSOCKET)parameter;
	PXYSOCKET_CONTEXT psc = (PXYSOCKET_CONTEXT)context;
	int result = 0;
	PSOCKADDR_IN psai;
	PCLIENT_CONTEXT pcc = (PCLIENT_CONTEXT)psc->context;

	switch (number)
	{
	case XYSOCKET_CLOSE:
		switch (type)
		{
		case XYSOCKET_TYPE_TCP:
			break;
		case XYSOCKET_TYPE_TCP0:
			OutputDebugString(_T("Client Close\r\n"));
			break;
		case XYSOCKET_TYPE_TCP1:
			break;
		default:
			break;
		}
		break;
	case XYSOCKET_CONNECT:
		switch (type)
		{
		case XYSOCKET_TYPE_TCP0:
			switch (length)
			{
			case 0:
				// 成功
				if (pointer == NULL)
				{
					if (GetTickCount() - pcc->tickcount > 30000)
					{
						result = XYSOCKET_ERROR_TIMEOUT;

						OutputDebugString(_T("Client Time out\r\n"));

						SetEvent(pcc->hevent);
					}
				}
				else
				{
					OutputDebugString(_T("Client Ok\r\n"));

					SetEvent(pcc->hevent);
				}
				break;
			case XYSOCKET_ERROR_FAILED:
			case XYSOCKET_ERROR_REFUSED:
			case XYSOCKET_ERROR_OVERFLOW:
			default:
				if (TRUE)
				{
					TCHAR debugtext[256];
					wsprintf(debugtext, _T("Client Failed %d\r\n"), length);
					OutputDebugString(debugtext);
				}

				SetEvent(pcc->hevent);
				break;
			}
			break;
		case XYSOCKET_TYPE_TCP1:
			switch (length)
			{
			case XYSOCKET_ERROR_ACCEPT:
				break;
			case XYSOCKET_ERROR_ACCEPTED:
				break;
			case XYSOCKET_ERROR_OVERFLOW:
				break;
			default:
				break;
			}
			break;
		default:
			break;
		}
		break;
	case XYSOCKET_RECV:
		switch (type)
		{
		case XYSOCKET_TYPE_TCP0:
			break;
		case XYSOCKET_TYPE_TCP1:
			break;
		default:
			break;
		}
		break;
	case XYSOCKET_SEND:
		break;
	case XYSOCKET_TIMEOUT:
		switch (type)
		{
		case XYSOCKET_TYPE_TCP:
			break;
		case XYSOCKET_TYPE_TCP0:
			//{
			//	static int sss = 0;
			//	sss++;
			//	if (sss == 15)
			//	{
			//		result = XYSOCKET_ERROR_FAILED;
			//	}
			//}
			OutputDebugString(_T("Client time out\r\n"));
			break;
		case XYSOCKET_TYPE_TCP1:
			break;
		default:
			break;
		}
		break;
	default:
		break;
	}

	return(result);
}
//---------------------------------------------------------------------------
int _tmain(int argc, _TCHAR* argv[])
{
	XYSOCKET ps[1];
	SOCKET s;
	SOCKADDR_IN sai;
	CLIENT_CONTEXT pcc[1];
	WSADATA wsad;
	int i;
	char cp[20];
	TCHAR string[20];

	if (argc > 1)
	{
		i = 0;
		while (argv[1][i] != _T('\0') && i + 1 < sizeof(cp))
		{
			cp[i] = argv[1][i];
			i++;
		}
		cp[i] = '\0';
	}
	else
	{
		_tcscpy(string, _T("127.0.0.1"));
		//_tcscpy(string, _T("192.168.126.155"));

		_tprintf(_T("%s\r\n"), string);

		i = 0;
		while (string[i] != _T('\0') && i + 1 < sizeof(cp))
		{
			cp[i] = string[i];
			i++;
		}
		cp[i] = '\0';
	}

	WSAStartup(MAKEWORD(2, 2), &wsad);

	XYSocketsStartup(ps, NULL, NULL, SocketProcedure);

	XYSocketLaunchThread(ps, XYSOCKET_THREAD_CONNECT, 64);
	XYSocketLaunchThread(ps, XYSOCKET_THREAD_CLIENT, 64);

	pcc->hevent = CreateEvent(NULL, TRUE, FALSE, NULL);
	pcc->tickcount = GetTickCount();

	sai.sin_family = AF_INET;
	sai.sin_port = htons(1024);
	sai.sin_addr.s_addr = inet_addr(cp);
	//sai.sin_addr.s_addr = inet_addr("192.168.132.168");

	int sendbuffersize;
	sendbuffersize = 1024;
	s = XYTCPConnect(ps, (LPVOID)pcc, (const SOCKADDR *)&sai, sizeof(sai), sendbuffersize);
	if (s != INVALID_SOCKET)
	{
		WaitForSingleObject(pcc->hevent, INFINITE);

		int sendlength;
		BYTE *sendbuffer = ReadBuffer(_T("C:\\Tools\\6012fd737c28a25af0370c2bd02c4ed3.bmp"), &sendlength);
		int i;
		int l0, l1;
		if (sendbuffer != NULL)
		{
			l1 = 18000;
			for (i = 0; i < sendlength; i += l1)
			{
				l0 = sendlength - i;
				l0 = l0 < l1 ? l0 : l1;
				XYTCPSend(s, (const char *)sendbuffer + i, l0, 5);
			}
		}
	}

	OutputDebugString(_T("Client quit\r\n"));

	MessageBox(NULL, _T("client"), _T("quit"), MB_OK);

	CloseHandle(pcc->hevent);

	XYSocketsCleanup(ps);

	WSACleanup();

	return 0;
}


//---------------------------------------------------------------------------