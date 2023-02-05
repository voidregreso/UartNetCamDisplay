// Clase de di¨¢logo de configuraci¨®n de la comunicaci¨®n de red
#include "stdafx.h"
#include "DialogNetCfg.h"
#include "afxdialogex.h"
#include "UartDisplayDlg.h"
#include <mstcpip.h>
#pragma warning(disable : 4996)


IMPLEMENT_DYNAMIC(CDialogNetCfg, CDialog)

CDialogNetCfg::CDialogNetCfg(CWnd *pParent /*=NULL*/)
	: CDialog(CDialogNetCfg::IDD, pParent), m_ServerIP(_T("")), m_ClientIP(_T("")), m_szRxBufferSize(0), m_pRxBuffer(NULL), m_pClientSocket(NULL), m_pServerSocket(NULL), m_ClientPortNumber(0), m_ServerPortNumber(0)
{
}

CDialogNetCfg::~CDialogNetCfg()
{
	if (m_szRxBufferSize)
	{
		m_szRxBufferSize = 0;
		delete m_pRxBuffer;
	}
	if (m_pClientSocket != NULL)
	{
		m_pClientSocket->Close();
		delete m_pClientSocket;
		m_pClientSocket = NULL;
	}
	if (m_pServerSocket != NULL)
	{
		m_pServerSocket->Close();
		delete m_pServerSocket;
		m_pServerSocket = NULL;
	}
}

void CDialogNetCfg::DoDataExchange(CDataExchange *pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT_CLIENT_PORT, m_ClientPortNumber);
	// DDV_MinMaxUInt(pDX, m_ClientPortNumber, 1023, 65535);
	DDX_Text(pDX, IDC_EDIT_SERVER_PORT, m_ServerPortNumber);
	// DDV_MinMaxUInt(pDX, m_ServerPortNumber, 1023, 65535);
	DDX_Text(pDX, IDC_EDIT_SERVER_IP, m_ServerIP);
	DDX_Text(pDX, IDC_EDIT_CLIENT_IP, m_ClientIP);
}

BEGIN_MESSAGE_MAP(CDialogNetCfg, CDialog)
ON_EN_CHANGE(IDC_EDIT_SERVER_PORT, &CDialogNetCfg::OnEnChangeEditPort)
ON_BN_CLICKED(IDC_BUTTON_START_NET, &CDialogNetCfg::OnBnClickedButtonStartNet)
ON_BN_CLICKED(IDC_RADIO_TCP, &CDialogNetCfg::OnBnClickedRadioTcp)
ON_BN_CLICKED(IDC_RADIO_UDP, &CDialogNetCfg::OnBnClickedRadioUdp)
ON_BN_CLICKED(IDC_CHECK_NET_SERVER, &CDialogNetCfg::OnBnClickedCheckNetServer)
ON_BN_CLICKED(IDC_CHECK_NET_CLIENT, &CDialogNetCfg::OnBnClickedCheckNetClient)
ON_EN_CHANGE(IDC_EDIT_SERVER_IP, &CDialogNetCfg::OnEnChangeEditServerIp)
END_MESSAGE_MAP()


void CDialogNetCfg::OnEnChangeEditPort()
{

	UpdateData(TRUE);
}

void CDialogNetCfg::OnBnClickedButtonStartNet()
{
	int res = 0;

	if (m_bStarted)
	{
		if (m_pClientSocket != NULL)
		{
			m_pClientSocket->Close();
			delete m_pClientSocket;
			m_pClientSocket = NULL;
			m_TimerCnt = 0;
		}
		if (m_pServerSocket != NULL)
		{
			m_pServerSocket->Close();
			delete m_pServerSocket;
			m_pServerSocket = NULL;
		}
		if (m_szRxBufferSize)
		{
			delete m_pRxBuffer;
			m_szRxBufferSize = 0;
		}
		m_bStarted = FALSE;

		((CButton *)GetDlgItem(IDC_RADIO_TCP))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_RADIO_UDP))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_CHECK_NET_SERVER))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_CHECK_NET_CLIENT))->EnableWindow(TRUE);
		SetDlgItemText(IDC_BUTTON_START_NET, _T("Start"));
		((CButton *)GetDlgItem(IDC_CHECK_LINKED))->SetCheck(FALSE);
		SetDlgItemText(IDC_CHECK_LINKED, _T("Disconnected"));

		if (m_WorkingMode == WORKING_MODE_SERVER)
		{
			m_ClientIP.Empty();
			((CEdit *)GetDlgItem(IDC_EDIT_SERVER_IP))->EnableWindow(FALSE);
			((CEdit *)GetDlgItem(IDC_EDIT_SERVER_PORT))->EnableWindow(TRUE);
		}
		else
		{
			((CEdit *)GetDlgItem(IDC_EDIT_SERVER_IP))->EnableWindow(TRUE);
			((CEdit *)GetDlgItem(IDC_EDIT_SERVER_PORT))->EnableWindow(TRUE);
		}
	}
	else
	{

		m_bStarted = TRUE;

		((CButton *)GetDlgItem(IDC_RADIO_TCP))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_RADIO_UDP))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_CHECK_NET_SERVER))->EnableWindow(FALSE);
		((CButton *)GetDlgItem(IDC_CHECK_NET_CLIENT))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_EDIT_SERVER_IP))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_EDIT_SERVER_PORT))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_EDIT_CLIENT_IP))->EnableWindow(FALSE);
		((CEdit *)GetDlgItem(IDC_EDIT_CLIENT_PORT))->EnableWindow(FALSE);

		if (m_WorkingMode == WORKING_MODE_SERVER)
		{
			if (m_ServerPortNumber < 1024 || m_ServerPortNumber > 65535)
				AfxMessageBox(_T("The port range should be between 1024 and 65535!"));
			else
			{

				if (m_pServerSocket != NULL)
				{
					m_pServerSocket->Close();
					delete m_pServerSocket;
				}

				m_pServerSocket = new CMySocket();
				m_pServerSocket->SetParentClass(this);
				if (!m_pServerSocket->Create(m_ServerPortNumber, SOCK_STREAM))
				{
					res = GetLastError();
					TRACE("Server socket create failed with err:%d", res);
					// sprintf_s(szError, 256, "Create Socket Faild: %d", GetLastError());
					AfxMessageBox(_T("Create Socket Failed!"));
					goto server_error_exit;
				}
				BOOL bOptVal = TRUE;
				int bOptLen = sizeof(BOOL);
				m_TimerCnt = 0;
				// La configuraci¨®n de las opciones de Socket es un paso necesario para resolver el error 10048
				m_pServerSocket->SetSockOpt(SO_REUSEADDR, (void *)&bOptVal, bOptLen, SOL_SOCKET);
				// Listen
				if (!m_pServerSocket->Listen(1))
				{
					AfxMessageBox(_T("Socket Listen Failed"));
					m_pServerSocket->Close();
					goto server_error_exit;
				}
				SetDlgItemText(IDC_BUTTON_START_NET, _T("Stop"));
				goto ok_exit;

			server_error_exit:
			{
				delete m_pServerSocket;
				m_pServerSocket = NULL;
				goto start_err_exit;
			}
			}
		}
		else
		{
			if (m_ServerIP.GetLength() == 0 || m_ClientIP.GetLength() == 0)
			{
				AfxMessageBox(_T("Please input server IP address!"));
				((CEdit *)GetDlgItem(IDC_EDIT_SERVER_IP))->EnableWindow(TRUE);
				goto start_err_exit;
			}
			if (m_ServerPortNumber < 1024 || m_ServerPortNumber > 65535)
			{
				AfxMessageBox(_T("The port range should be between 1024 and 65535!"));
				((CEdit *)GetDlgItem(IDC_EDIT_SERVER_IP))->EnableWindow(TRUE);
				goto start_err_exit;
			}
			else
			{

				if (m_pClientSocket != NULL)
				{
					m_pClientSocket->Close();
					delete m_pClientSocket;
				}

				m_pClientSocket = new CMySocket();
				m_pClientSocket->SetParentClass(this);
				if (!m_pClientSocket->Create(0, SOCK_STREAM))
				{
					AfxMessageBox(_T("Create Socket Failed!"));
					goto client_error_exit;
				}

				m_TimerCnt = 0;

				// listen
				if (m_pClientSocket->Connect(m_ServerIP, m_ServerPortNumber) == 0)
				{
					res = GetLastError();
					if (res == WSAEWOULDBLOCK)
					{
						SetDlgItemText(IDC_BUTTON_START_NET, _T("Connecting"));
						goto ok_exit;
					}
					AfxMessageBox(_T("Socket Connected Failed"));
					m_pClientSocket->Close();
					goto client_error_exit;
				}

				goto ok_exit;

			client_error_exit:
			{
				delete m_pClientSocket;
				m_pClientSocket = NULL;
				goto start_err_exit;
			}
			}
		}

	start_err_exit:
		m_bStarted = FALSE;
		SetDlgItemText(IDC_BUTTON_START_NET, _T("Start"));
		((CEdit *)GetDlgItem(IDC_EDIT_SERVER_PORT))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_RADIO_TCP))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_RADIO_UDP))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_CHECK_NET_SERVER))->EnableWindow(TRUE);
		((CButton *)GetDlgItem(IDC_CHECK_NET_CLIENT))->EnableWindow(TRUE);
	}
ok_exit:
	UpdateData(FALSE);
}

void CDialogNetCfg::SetHandle(HANDLE hIn, HANDLE hOut)
{
	m_hInput = hIn;
	m_hOutput = hOut;
}

void CDialogNetCfg::OnBnClickedRadioTcp()
{
	m_ConnectionMode = CONNECTION_MODE_TCP;
}

void CDialogNetCfg::OnBnClickedRadioUdp()
{
	AfxMessageBox(_T("Does not support UDP Mode!"));
	m_ConnectionMode = CONNECTION_MODE_TCP;
	((CButton *)GetDlgItem(IDC_RADIO_UDP))->SetCheck(FALSE);
	((CButton *)GetDlgItem(IDC_RADIO_TCP))->SetCheck(TRUE);
}

struct _tcp_keepalive
{
	u_long onoff;
	u_long keepalivetime;
	u_long keepaliveinterval;
};

void CDialogNetCfg::OnClientAccept(int nErrorCode)
{
	if (m_pClientSocket != NULL)
	{
		return;
	}
	else
	{
		CMySocket *pSocket = new CMySocket;
		if (m_pServerSocket->Accept(*pSocket))
		{
			pSocket->SetParentClass(m_pServerSocket->GetParentClass());
			m_szRxBufferSize = 4096;
			m_pRxBuffer = new char[m_szRxBufferSize];
			UINT m_ClientPortNumber = m_ServerPortNumber;
			// pSocket->AsyncSelect(FD_READ);

			struct _tcp_keepalive alive;
			DWORD dwBytesRet;
			alive.onoff = TRUE;
			alive.keepalivetime = 5000;
			alive.keepaliveinterval = 3000;
			if (WSAIoctl(*pSocket, SIO_KEEPALIVE_VALS, &alive, sizeof(alive),
						 NULL, 0, &dwBytesRet, NULL, NULL) == SOCKET_ERROR)
			{
				printf("WSAIotcl(SIO_KEEPALIVE_VALS) failed; %d\n",
					   WSAGetLastError());
			}

			m_pClientSocket = pSocket;
			m_TimerCnt = 0;
			pSocket->GetPeerName(m_ClientIP, m_ClientPortNumber);
			((CButton *)GetDlgItem(IDC_CHECK_LINKED))->SetCheck(TRUE);
			SetDlgItemText(IDC_CHECK_LINKED, _T("Connected"));
			UpdateData(FALSE);
		}
		else
		{
			m_szRxBufferSize = 0;
			delete m_pRxBuffer;
			m_pClientSocket = NULL;
			delete pSocket;
		}
	}
}

void CDialogNetCfg::OnClientDisconnect(int nErrorCode)
{
	if (m_pClientSocket != NULL)
	{
		m_pClientSocket->Close();
		delete m_pClientSocket;
		m_pClientSocket = NULL;
		m_TimerCnt = 0;
		if (m_szRxBufferSize)
		{
			m_szRxBufferSize = 0;
			delete m_pRxBuffer;
		}
		((CButton *)GetDlgItem(IDC_CHECK_LINKED))->SetCheck(FALSE);
		SetDlgItemText(IDC_CHECK_LINKED, _T("Disconnected"));
	}
	if (m_WorkingMode == WORKING_MODE_SERVER)
	{
		m_ClientIP.Empty();
	}
	else
	{
		OnBnClickedButtonStartNet();
	}
	UpdateData(FALSE);
}

void CDialogNetCfg::OnDataReceive()
{
	int nReceived = m_pClientSocket->Receive(m_pRxBuffer, m_szRxBufferSize);
	DWORD nWrite;

	m_TimerCnt = 0;
	if (nReceived > 0 && m_hOutput)
	{
		WriteFile(m_hOutput, m_pRxBuffer, nReceived, &nWrite, NULL);
		if ((DWORD)nReceived > nWrite)
		{
			AfxMessageBox(_T("Pipe data flow out!"));
		}
	}
}

void CDialogNetCfg::OnDataSend()
{
	m_TimerCnt = 0;
}

void CDialogNetCfg::OnServerConnected()
{
	m_szRxBufferSize = 4096;
	m_pRxBuffer = new char[m_szRxBufferSize];
	m_ClientPortNumber = m_ServerPortNumber;
	((CButton *)GetDlgItem(IDC_CHECK_LINKED))->SetCheck(TRUE);
	SetDlgItemText(IDC_BUTTON_START_NET, _T("Stop"));
	SetDlgItemText(IDC_CHECK_LINKED, _T("Connected"));
	UpdateData(FALSE);
}

CMySocket::CMySocket()
{
	CAsyncSocket::CAsyncSocket();
	m_pDlg = NULL;
}

CMySocket::~CMySocket()
{
	CAsyncSocket::~CAsyncSocket();
	m_pDlg = NULL;
}

void CMySocket::OnAccept(int nErrorCode)
{
	if (m_pDlg != NULL)
		((CDialogNetCfg *)m_pDlg)->OnClientAccept(nErrorCode);
	CAsyncSocket::OnAccept(nErrorCode);
}

void CMySocket::OnReceive(int nErrorCode)
{
	if (m_pDlg != NULL && nErrorCode == 0)
		((CDialogNetCfg *)m_pDlg)->OnDataReceive();
	CAsyncSocket::OnReceive(nErrorCode);
}

void CMySocket::OnSend(int nErrorCode)
{
	if (m_pDlg != NULL && nErrorCode == 0)
		((CDialogNetCfg *)m_pDlg)->OnDataSend();
	CAsyncSocket::OnSend(nErrorCode);
}

void CMySocket::OnOutOfBandData(int nErrorCode)
{
	CAsyncSocket::OnOutOfBandData(nErrorCode);
}

void CMySocket::OnConnect(int nErrorCode)
{
	if (m_pDlg != NULL)
	{
		if (nErrorCode == 0)
			((CDialogNetCfg *)m_pDlg)->OnServerConnected();
		else
		{
			if (nErrorCode == WSAETIMEDOUT)
				AfxMessageBox(_T("Connection timeout!"));
			((CDialogNetCfg *)m_pDlg)->OnClientDisconnect(nErrorCode);
		}
	}
	CAsyncSocket::OnConnect(nErrorCode);
}

void CMySocket::OnClose(int nErrorCode)
{
	if (m_pDlg != NULL)
		((CDialogNetCfg *)m_pDlg)->OnClientDisconnect(nErrorCode);
	CAsyncSocket::OnClose(nErrorCode);
}

void CMySocket::SetParentClass(void *pDlg)
{
	m_pDlg = pDlg;
}

void *CMySocket::GetParentClass(void)
{
	return m_pDlg;
}

int GetLocalMachineIP(CString *pIP)
{
	TCHAR hostname[256];
	int ret = gethostname(hostname, sizeof(hostname));
	if (ret == SOCKET_ERROR)
	{
		return FALSE;
	}

	HOSTENT *host = gethostbyname(hostname);
	if (host == NULL)
	{
		return FALSE;
	}
	pIP->Append(inet_ntoa(*(in_addr *)*host->h_addr_list));
	return TRUE;
}

void CDialogNetCfg::OnBnClickedCheckNetServer()
{
	m_WorkingMode = WORKING_MODE_SERVER;
	m_ClientIP.Empty();
	m_ServerIP.Empty();
	m_ClientPortNumber = 0;
	GetLocalMachineIP(&m_ServerIP);

	((CEdit *)GetDlgItem(IDC_EDIT_SERVER_IP))->EnableWindow(FALSE);
	((CEdit *)GetDlgItem(IDC_EDIT_SERVER_PORT))->EnableWindow(TRUE);

	((CEdit *)GetDlgItem(IDC_EDIT_CLIENT_IP))->EnableWindow(FALSE);
	((CEdit *)GetDlgItem(IDC_EDIT_CLIENT_PORT))->EnableWindow(FALSE);

	((CButton *)GetDlgItem(IDC_CHECK_NET_CLIENT))->SetCheck(FALSE);
	UpdateData(FALSE);
}

void CDialogNetCfg::OnBnClickedCheckNetClient()
{
	m_WorkingMode = WORKING_MODE_CLIENT;
	m_ServerIP.Empty();
	m_ClientIP.Empty();
	m_ServerPortNumber = 0;
	GetLocalMachineIP(&m_ClientIP);

	((CEdit *)GetDlgItem(IDC_EDIT_SERVER_IP))->EnableWindow(TRUE);
	((CEdit *)GetDlgItem(IDC_EDIT_SERVER_PORT))->EnableWindow(TRUE);

	((CEdit *)GetDlgItem(IDC_EDIT_CLIENT_IP))->EnableWindow(FALSE);
	((CEdit *)GetDlgItem(IDC_EDIT_CLIENT_PORT))->EnableWindow(FALSE);

	((CButton *)GetDlgItem(IDC_CHECK_NET_SERVER))->SetCheck(FALSE);
	UpdateData(FALSE);
}

void CDialogNetCfg::OnEnChangeEditServerIp()
{
	UpdateData(TRUE);
}
