#ifndef __TCP_SERVER_H__
#define __TCP_SERVER_H__

#define WM_NET_RXDATA (WM_USER + 7) // data were received and placed in the input buffer.
#define WM_NET_TXDATA
#define WM_NET_CLIENT_CONNECTED (WM_USER + 20)	  // The event character was received and placed in the input buffer.
#define WM_NET_CLIENT_DISCONNECTED (WM_USER + 21) // The last character in the output buffer was sent.

class CDialogNetCfg;

class MyCSocket : public CAsyncSocket
{
public:
	MyCSocket(HANDLE *hEventArray, UINT Size);
	virtual ~MyCSocket();

private:
	// CChartRoom_UDP_ServerDlg* m_pdlg;
	HANDLE m_hSocketEvent[6];

protected:
	virtual void OnAccept(int nErrorCode);
	virtual void OnReceive(int nErrorCode);
	virtual void OnSend(int nErrorCode);
	virtual void OnOutOfBandData(int nErrorCode);
	virtual void OnConnect(int nErrorCode);
	virtual void OnClose(int nErrorCode);
};

typedef enum
{
	SOCKET_EVENT_ONACCEPT = 0,
	SOCKET_EVENT_ONRECEIVED,
	SOCKET_EVENT_ONSEND,
	SOCKET_EVENT_ONOUTOFBANDDATA,
	SOCKET_EVENT_ONCONNECT,
	SOCKET_EVENT_ONCLOSE,
	TCP_SERVER_EVENT_TX_EXIT,
	TCP_SERVER_EVENT_RX_EXIT,
} MY_SOCKET_EVENT;

class CTcpServer
{
public:
	// contruction and destruction
	CTcpServer();
	virtual ~CTcpServer();

	// port initialisation
	BOOL StartTcpServer(CDialogNetCfg *pOwner, HANDLE hRead, HANDLE hWrite, UINT portnr, CString *IP);

	void StopTcpServer();

	DWORD GetWriteBufferSize();
	DWORD GetCommEvents();

	void WriteToPort(char *string);

protected:
	// protected memberfunctions
	void ProcessErrorMessage(char *ErrorText);
	static UINT TcpServerThread(LPVOID pParam);

	// thread
	CWinThread *m_TxThread;
	CWinThread *m_RxThread;
	BOOL m_bTxThreadActive;
	BOOL m_bRxThreadActive;

	static UINT TxThread(LPVOID pParam);
	static UINT RxThread(LPVOID pParam);

	// synchronisation objects
	CRITICAL_SECTION m_csCommunicationSync;

	MyCSocket *m_hSocket;
	CAsyncSocket *m_hClientSocket;
	HANDLE m_hWriteEvent;

	HANDLE m_hInput;
	HANDLE m_hOutput;

	// Event array.
	// One element is used for each event. There are two event handles for each port.
	// A Write event and a receive character event which is located in the overlapped structure (m_ov.hEvent).
	// There is a general shutdown when the port is closed.
	HANDLE m_hEventArray[8];

	// HANDLE				m_hRxThreadExitEvent;
	// HANDLE				m_hTxThreadExitEvent;

	// owner window
	CDialogNetCfg *m_pOwner;

	// misc
	UINT m_nPortNr;
	CString m_nIpAddr;
	char *m_szWriteBuffer;
	DWORD m_nWriteBufferSize;
	char *m_szReceiveBuffer;
	DWORD m_nReceiveBufferSize;
	DWORD m_dwCommEvents;
};

#endif