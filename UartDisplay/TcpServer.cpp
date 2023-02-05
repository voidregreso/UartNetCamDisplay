#include "stdafx.h"
#include "TcpServer.h"
#include "DialogNetCfg.h"

MyCSocket::MyCSocket(HANDLE *hEventArray, UINT Size)
{
	UINT i;
	CAsyncSocket::CAsyncSocket();
	for (i = 0; (i < 6); i++)
		m_hSocketEvent[i] = hEventArray[i];
}

MyCSocket::~MyCSocket()
{
	UINT i;
	CAsyncSocket::~CAsyncSocket();
	for (i = 0; (i < 6); i++)
		m_hSocketEvent[i] = NULL;
}

void MyCSocket::OnAccept(int nErrorCode)
{
	CAsyncSocket::OnAccept(nErrorCode);
	SetEvent(m_hSocketEvent[SOCKET_EVENT_ONACCEPT]);
}

void MyCSocket::OnReceive(int nErrorCode)
{
	CAsyncSocket::OnReceive(nErrorCode);
	SetEvent(m_hSocketEvent[SOCKET_EVENT_ONRECEIVED]);
}

void MyCSocket::OnSend(int nErrorCode)
{
	CAsyncSocket::OnSend(nErrorCode);
	SetEvent(m_hSocketEvent[SOCKET_EVENT_ONSEND]);
}

void MyCSocket::OnOutOfBandData(int nErrorCode)
{
	CAsyncSocket::OnOutOfBandData(nErrorCode);
	SetEvent(m_hSocketEvent[SOCKET_EVENT_ONOUTOFBANDDATA]);
}

void MyCSocket::OnConnect(int nErrorCode)
{
	CAsyncSocket::OnConnect(nErrorCode);
	SetEvent(m_hSocketEvent[SOCKET_EVENT_ONCONNECT]);
}

void MyCSocket::OnClose(int nErrorCode)
{
	CAsyncSocket::OnClose(nErrorCode);
	SetEvent(m_hSocketEvent[SOCKET_EVENT_ONCLOSE]);
}

CTcpServer::CTcpServer()
{
	m_TxThread = NULL;
	m_RxThread = NULL;
	m_hInput = NULL;
	m_hOutput = NULL;
	m_bTxThreadActive = FALSE;
	m_bRxThreadActive = FALSE;
	// m_hRxThreadExitEvent = NULL;
	// m_hTxThreadExitEvent = NULL;
}

//
// Delete dynamic memory
//
CTcpServer::~CTcpServer()
{

	while (m_bTxThreadActive)
	{
		SetEvent(m_hEventArray[TCP_SERVER_EVENT_TX_EXIT]);
		Sleep(100);
	}

	while (m_bRxThreadActive)
	{
		SetEvent(m_hEventArray[TCP_SERVER_EVENT_RX_EXIT]);
		Sleep(100);
	}

	for (int i = 0; i < 8; i++)
	{
		if (m_hEventArray[i] != NULL)
		{
			CloseHandle(m_hEventArray[i]);
			m_hEventArray[i] = NULL;
		}
	}

	// if (m_hRxThreadExitEvent)
	//	CloseHandle(m_hRxThreadExitEvent);
	// if (m_hTxThreadExitEvent)
	//	CloseHandle(m_hTxThreadExitEvent);

	if (m_szWriteBuffer != NULL)
		delete[] m_szWriteBuffer;
	m_szWriteBuffer = NULL;
	if (m_szReceiveBuffer != NULL)
		delete[] m_szReceiveBuffer;
	m_szReceiveBuffer = NULL;

	m_hInput = NULL;
	m_hOutput = NULL;
	m_hSocket->Close();
	delete m_hSocket;
	m_hSocket = NULL;
}

BOOL CTcpServer::StartTcpServer(CDialogNetCfg *pOwner, HANDLE hInput, HANDLE hOutput, UINT portnr, CString *IP)
{
	char szError[256] = {0};
	while (m_bTxThreadActive)
	{
		SetEvent(m_hEventArray[TCP_SERVER_EVENT_TX_EXIT]);
		Sleep(100);
	}

	while (m_bRxThreadActive)
	{
		SetEvent(m_hEventArray[TCP_SERVER_EVENT_RX_EXIT]);
		Sleep(100);
	}

	m_hInput = hInput;
	m_hOutput = hOutput;

	m_pOwner = pOwner;

	if (m_szWriteBuffer != NULL)
		delete[] m_szWriteBuffer;
	if (m_szReceiveBuffer != NULL)
		delete[] m_szReceiveBuffer;
	m_szWriteBuffer = new char[1024];
	m_szReceiveBuffer = new char[2048];

	for (int i = 0; i < 8; i++)
	{
		if (m_hEventArray[i] != NULL)
			ResetEvent(m_hEventArray[i]);
		else
			m_hEventArray[i] = CreateEvent(NULL, TRUE, FALSE, NULL);
	}

	// if (m_hRxThreadExitEvent != NULL)
	//	ResetEvent(m_hRxThreadExitEvent);
	// else
	//	m_hRxThreadExitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	// if (m_hTxThreadExitEvent != NULL)
	//	ResetEvent(m_hTxThreadExitEvent);
	// else
	//	m_hTxThreadExitEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	m_hSocket = new MyCSocket(m_hEventArray, 4);
	if (!m_hSocket->Create(portnr, SOCK_STREAM))
	{

		sprintf_s(szError, 256, "Create Socket Faild: %d", GetLastError());
		AfxMessageBox(szError);
		return FALSE;
	}
	BOOL bOptVal = TRUE;
	int bOptLen = sizeof(BOOL);

	//设置Socket的选项, 解决10048错误必须的步骤
	m_hSocket->SetSockOpt(SO_REUSEADDR, (void *)&bOptVal, bOptLen, SOL_SOCKET);
	//监听
	if (!m_hSocket->Listen(1))
	{
		sprintf_s(szError, 256, "Listen Faild: %d", GetLastError());
		AfxMessageBox(szError);
		return FALSE;
	}
	if (!(m_TxThread = AfxBeginThread(TxThread, (LPVOID *)this)))
		return FALSE;
	// if (!(m_RxThread = AfxBeginThread(RxThread, (LPVOID *)this)))
	// return FALSE;
	return TRUE;
}

void CTcpServer::StopTcpServer()
{
	delete this;
}

UINT CTcpServer::RxThread(LPVOID pParam)
{
	DWORD Event = 0;
	int nRead;
	DWORD nWrite;
	CTcpServer *pObj = (CTcpServer *)pParam;
	while (1)
	{
		Event = WaitForMultipleObjects(8, pObj->m_hEventArray, FALSE, INFINITE);
		switch (Event)
		{
		case SOCKET_EVENT_ONRECEIVED:
			if (pObj->m_hInput != NULL)
			{
				nRead = pObj->m_hClientSocket->Receive(pObj->m_szReceiveBuffer, 1024);
				if (nRead == SOCKET_ERROR)
					break;
				WriteFile(pObj->m_hInput, pObj->m_szReceiveBuffer, nRead, &nWrite, NULL);
				if ((DWORD)nRead > nWrite)
				{
					AfxMessageBox(_T("管道数据溢出！"));
				}
				else
					::SendMessage((pObj->m_pOwner)->m_hWnd, WM_NET_RXDATA, (WPARAM)nWrite, (LPARAM)NULL);
			}
			break;
		case SOCKET_EVENT_ONCLOSE:

			break;
		case TCP_SERVER_EVENT_RX_EXIT:
			pObj->m_bRxThreadActive = FALSE;

			// Kill this thread.  break is not needed, but makes me feel better.
			AfxEndThread(100);
			break;
		default:
			break;
		}
		// ResetEvent(pObj->m_hEventArray[Event]);
	}
}

UINT CTcpServer::TxThread(LPVOID pParam)
{
	DWORD Event = 0;
	CTcpServer *pObj = (CTcpServer *)pParam;
	while (1)
	{
		Event = WaitForMultipleObjects(8, pObj->m_hEventArray, FALSE, INFINITE);
		switch (Event)
		{
		case SOCKET_EVENT_ONCLOSE:
			pObj->m_hClientSocket->Close();
			pObj->m_hClientSocket = NULL;
			pObj->m_pOwner->m_sClientIP.Empty();
			PostMessage(pObj->m_pOwner->m_hWnd, WM_NET_CLIENT_CONNECTED, NULL, NULL);
			break;

		case SOCKET_EVENT_ONACCEPT:
			if (pObj->m_hClientSocket != NULL)
			{
				// If there has already a client connected to the server,
				// ignore new connection request.
				break;
			}
			else
			{
				CAsyncSocket *pSocket = new CAsyncSocket;
				if (pObj->m_hSocket->Accept(*pSocket))
				{
					UINT ClientPort = pObj->m_pOwner->m_PortNumber;
					// pSocket->AsyncSelect(FD_READ);
					pObj->m_hClientSocket = pSocket;
					pSocket->GetPeerName(pObj->m_pOwner->m_sClientIP, ClientPort);
					PostMessage(pObj->m_pOwner->m_hWnd, WM_NET_CLIENT_CONNECTED, NULL, NULL);
				}
				else
				{
					pObj->m_hClientSocket = NULL;
					delete pSocket;
				}
			}

			break;
		case SOCKET_EVENT_ONSEND:
			break;
		case SOCKET_EVENT_ONOUTOFBANDDATA:
			break;
		case SOCKET_EVENT_ONCONNECT:
			break;

		case TCP_SERVER_EVENT_TX_EXIT:
			pObj->m_bRxThreadActive = FALSE;
			if (pObj->m_hClientSocket != NULL)
			{
				pObj->m_hClientSocket->Close();
				pObj->m_hClientSocket = NULL;
			}

			// Kill this thread.  break is not needed, but makes me feel better.
			AfxEndThread(100);
			break;
		default:
			break;
		}
		ResetEvent(pObj->m_hEventArray[Event]);
	}
}