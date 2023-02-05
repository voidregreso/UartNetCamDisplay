#pragma once

#include "Resource.h"

typedef enum
{
	CONNECTION_MODE_TCP = 0,
	CONNECTION_MODE_UDP,
} CONNECTION_MODE;

typedef enum
{
	WORKING_MODE_SERVER = 0,
	WORKING_MODE_CLIENT,
} WORKING_MODE;

#define WM_NET_RXDATA (WM_USER + 7)

class CMySocket : public CAsyncSocket
{
public:
	CMySocket();
	virtual ~CMySocket();
	void SetParentClass(void *pDlg);
	void *GetParentClass(void);

protected:
	void *m_pDlg;

protected:
	virtual void OnAccept(int nErrorCode);
	virtual void OnReceive(int nErrorCode);
	virtual void OnSend(int nErrorCode);
	virtual void OnOutOfBandData(int nErrorCode);
	virtual void OnConnect(int nErrorCode);
	virtual void OnClose(int nErrorCode);
};


class CDialogNetCfg : public CDialog
{
	DECLARE_DYNAMIC(CDialogNetCfg)

public:
	CDialogNetCfg(CWnd *pParent = NULL);
	virtual ~CDialogNetCfg();

	enum
	{
		IDD = IDD_DIALOG_NET_CFG
	};

protected:
	virtual void DoDataExchange(CDataExchange *pDX); 

	DECLARE_MESSAGE_MAP()
public:
	CString m_ServerIP;
	CString m_ClientIP;
	UINT m_ClientPortNumber;
	UINT m_ServerPortNumber;
	BOOL m_bStarted;
	CMySocket *m_pServerSocket;
	CMySocket *m_pClientSocket;
	CONNECTION_MODE m_ConnectionMode;
	WORKING_MODE m_WorkingMode;
	afx_msg void OnEnChangeEditPort();
	afx_msg void OnBnClickedButtonStartNet();
	afx_msg void OnBnClickedRadioTcp();
	afx_msg void OnBnClickedRadioUdp();
	void OnClientDisconnect(int nErrorCode);
	void OnClientAccept(int nErrorCode);
	void OnDataReceive();
	void OnDataSend();
	void OnServerConnected();

	DWORD m_szRxBufferSize;
	char *m_pRxBuffer;
	HANDLE m_hInput;
	HANDLE m_hOutput;
	UINT m_TimerCnt; // Recuento de tiempo de espera (utilizado para determinar si se env¨ªa un paquete heartbeat)
	void SetHandle(HANDLE hIn, HANDLE hOut);
	afx_msg void OnBnClickedCheckNetServer();
	afx_msg void OnBnClickedCheckNetClient();
	afx_msg void OnEnChangeEditServerIp();
};
