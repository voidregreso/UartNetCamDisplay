
#include "stdafx.h"
#include "UartDisplay.h"
#include "UartDisplayDlg.h"
#include "afxdialogex.h"
#include <MMSystem.h>
#pragma warning(disable : 4996)
#pragma comment(lib, "winmm.lib")

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#define MJPEG_FRAME_BUFFER_SIZE 5120 * 1024

struct _BuffInfo
{
	UINT8 DataBuffer[MJPEG_FRAME_BUFFER_SIZE];
	UINT DataSize;
};

const UINT8 MjpegTail[2] = {0xFF, 0xD9};
const UINT8 MjpegHeader[2] = {0xFF, 0xD8};

static struct _BuffInfo MjpegFrameBufferInfo[2];

CUartDisplayDlg::CUartDisplayDlg(CWnd *pParent /*=NULL*/)
	: CDialogEx(CUartDisplayDlg::IDD, pParent), m_ReceivedDisplayData(_T("")), m_RxCnt(0), m_TxCnt(0), m_bSaveToFile(FALSE), m_nFrameStartCounter(0), m_nFrameEndCounter(0), m_nFrameCnt(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CUartDisplayDlg::DoDataExchange(CDataExchange *pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Text(pDX, IDC_EDIT1, m_ReceivedDisplayData);
	DDX_Control(pDX, IDC_TAB1, m_tab1);
	DDX_Check(pDX, IDC_CHECK_SAVE_IMAGE, m_bSaveToFile);
}

BEGIN_MESSAGE_MAP(CUartDisplayDlg, CDialogEx)
ON_WM_SYSCOMMAND()
ON_WM_PAINT()
ON_WM_QUERYDRAGICON()
ON_STN_CLICKED(IDC_COMBO_PORT, &CUartDisplayDlg::OnStnClickedPort)
ON_MESSAGE(WM_USER_MJPEG_FRAME_RECVED, &CUartDisplayDlg::OnMJPEGFrameReceived)
ON_EN_CHANGE(IDC_EDIT1, &CUartDisplayDlg::OnEnChangeEdit1)
ON_BN_CLICKED(IDC_BUTTON_CLR_IMG, &CUartDisplayDlg::OnBnClickedButtonClrImg)
ON_BN_CLICKED(IDC_BUTTON_CLR_BUF, &CUartDisplayDlg::OnBnClickedButtonClrBuf)
ON_BN_CLICKED(IDC_BUTTON_CLR_CNT, &CUartDisplayDlg::OnBnClickedButtonClrCnt)
ON_NOTIFY(TCN_SELCHANGE, IDC_TAB1, &CUartDisplayDlg::OnTcnSelchangeTab1)
ON_BN_CLICKED(IDC_CHECK_SAVE_IMAGE, &CUartDisplayDlg::OnBnClickedCheckSaveImage)
ON_WM_CLOSE()
ON_WM_TIMER()
ON_STN_DBLCLK(IDC_STATIC_PICTURE, &CUartDisplayDlg::OnStnDblclickStaticPicture)
ON_WM_SIZE()
END_MESSAGE_MAP()

UINT BackGndThread(LPVOID lpParam);

BOOL CUartDisplayDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	SetIcon(m_hIcon, TRUE);
	SetIcon(m_hIcon, FALSE);

	m_pEdit = (CEdit *)GetDlgItem(IDC_EDIT1);

	m_StatBar = new CStatusBarCtrl;
	RECT m_Rect;
	GetClientRect(&m_Rect);			 // Get the rectangular area of the dialog box
	m_Rect.top = m_Rect.bottom - 20; // Set the rectangular area of the status bar
	m_StatBar->Create(WS_BORDER | WS_VISIBLE | CBRS_BOTTOM, m_Rect, this, IDC_STATUS_BAR);

	int nParts[8] = {110, 220, 360, 500, 570, 690, 810, -1}; // Divide size
	m_StatBar->SetParts(8, nParts);							 // Divide status bar
	m_StatBar->SetText("TX: 0", STATUSBAR_COLUMN_TX_CNT, 0);
	m_StatBar->SetText("RX: 0", STATUSBAR_COLUMN_RX_CNT, 0);
	m_StatBar->SetText("RX Speed: 0B/s", STATUSBAR_COLUMN_RX_SPEED, 0);
	m_StatBar->SetText("FrameSize: 0B", STATUSBAR_COLUMN_FRAME_SIZE, 0);
	m_StatBar->SetText("FPS: 0", STATUSBAR_COLUMN_FPS, 0);
	m_StatBar->SetText("Image Width: 0", STATUSBAR_COLUMN_WIDTH, 0);
	m_StatBar->SetText("Image Height: 0", STATUSBAR_COLUMN_HEIGHT, 0);

	// Add two tabs
	m_tab1.InsertItem(0, _T("USART"));
	m_tab1.InsertItem(1, _T("Net"));
	// Create a dialog box and make the IDC_TAB1 control the parent window
	m_DialogUartCfg.Create(IDD_DIALOG_UART_CFG, GetDlgItem(IDC_TAB1));
	m_DialogNetCfg.Create(IDD_DIALOG_NET_CFG, GetDlgItem(IDC_TAB1));

	CRect rect;
	// Get tab client area size
	m_tab1.GetClientRect(&rect);
	// Since the tab window height includes the tab height, it has to be offset a bit
	rect.top += 20;

	// Set the subdialog size and move it to the specified position
	m_DialogUartCfg.MoveWindow(&rect);
	m_DialogNetCfg.MoveWindow(&rect);

	// Set hidden and show respectively
	m_DialogUartCfg.ShowWindow(true);
	m_DialogNetCfg.ShowWindow(false);

	// Set the default tab
	m_tab1.SetCurSel(0);
	m_DialogUartCfg.OnInitDialog();
	m_DialogUartCfg.SetParentDlg(this);
	m_DialogNetCfg.OnInitDialog();

	// Initialize UART tab
	GetComList_Reg((CComboBox *)m_DialogUartCfg.GetDlgItem(IDC_COMBO_PORT));
	((CComboBox *)m_DialogUartCfg.GetDlgItem(IDC_COMBO_BAUD))->AddString(_T("9600"));
	((CComboBox *)m_DialogUartCfg.GetDlgItem(IDC_COMBO_BAUD))->AddString(_T("14400"));
	((CComboBox *)m_DialogUartCfg.GetDlgItem(IDC_COMBO_BAUD))->AddString(_T("19200"));
	((CComboBox *)m_DialogUartCfg.GetDlgItem(IDC_COMBO_BAUD))->AddString(_T("38400"));
	((CComboBox *)m_DialogUartCfg.GetDlgItem(IDC_COMBO_BAUD))->AddString(_T("57600"));
	((CComboBox *)m_DialogUartCfg.GetDlgItem(IDC_COMBO_BAUD))->AddString(_T("115200"));
	((CComboBox *)m_DialogUartCfg.GetDlgItem(IDC_COMBO_BAUD))->AddString(_T("230400"));
	((CComboBox *)m_DialogUartCfg.GetDlgItem(IDC_COMBO_BAUD))->AddString(_T("380400"));
	((CComboBox *)m_DialogUartCfg.GetDlgItem(IDC_COMBO_BAUD))->AddString(_T("460800"));
	((CComboBox *)m_DialogUartCfg.GetDlgItem(IDC_COMBO_BAUD))->AddString(_T("921600"));
	((CComboBox *)m_DialogUartCfg.GetDlgItem(IDC_COMBO_BAUD))->SetCurSel(0);

	// Initialize network tab to server mode by default
	((CButton *)m_DialogNetCfg.GetDlgItem(IDC_CHECK_LINKED))->SetCheck(FALSE);
	((CButton *)m_DialogNetCfg.GetDlgItem(IDC_RADIO_TCP))->SetCheck(TRUE);
	m_DialogNetCfg.m_bStarted = FALSE;
	((CButton *)m_DialogNetCfg.GetDlgItem(IDC_CHECK_NET_SERVER))->SetCheck(TRUE);
	((CEdit *)m_DialogNetCfg.GetDlgItem(IDC_EDIT_SERVER_IP))->EnableWindow(FALSE);
	((CEdit *)m_DialogNetCfg.GetDlgItem(IDC_EDIT_SERVER_PORT))->EnableWindow(TRUE);
	((CEdit *)m_DialogNetCfg.GetDlgItem(IDC_EDIT_CLIENT_IP))->EnableWindow(FALSE);
	((CEdit *)m_DialogNetCfg.GetDlgItem(IDC_EDIT_CLIENT_PORT))->EnableWindow(FALSE);

	CheckRadioButton(IDC_RADIO1, IDC_RADIO2, IDC_RADIO2);

	// 1. Get Hostname
	char hostname[256];
	int ret = gethostname(hostname, sizeof(hostname));
	if (ret == SOCKET_ERROR)
	{
		return FALSE;
	}
	// 2. Get Host IP
	HOSTENT *host = gethostbyname(hostname);
	if (host == NULL)
	{
		return FALSE;
	}
	// 3. Convert to char* and copy back
	m_DialogNetCfg.m_ServerIP.Append(inet_ntoa(*(in_addr *)*host->h_addr_list));

	SECURITY_ATTRIBUTES sa;
	sa.bInheritHandle = FALSE;
	sa.lpSecurityDescriptor = NULL;
	sa.nLength = sizeof(SECURITY_ATTRIBUTES);
	if (!CreatePipe(&m_hRead, &m_hWrite, &sa, 0))
	{
		AfxMessageBox(_T("Cannot create communication pipe!"));
		m_hRead = NULL;
		m_hWrite = NULL;
		return FALSE;
	}
	m_DialogNetCfg.SetHandle(m_hRead, m_hWrite);

	m_pImgDataAccessSemphr = new CSemaphore(1, 1);

	m_bExpandDisplay = FALSE;

	GetClientRect(&m_OrigWndRect);
	m_tab1.GetWindowRect(&m_OrigTabRect);
	ScreenToClient(&m_OrigTabRect);
	((CEdit *)GetDlgItem(IDC_EDIT1))->GetWindowRect(&m_OrigEditRect);
	ScreenToClient(&m_OrigEditRect);
	((CStatic *)GetDlgItem(IDC_STATIC_PICTURE))->GetWindowRect(&m_OrigImageRect);
	ScreenToClient(&m_OrigImageRect);
	((CStatic *)GetDlgItem(IDC_BUTTON_CLR_IMG))->GetWindowRect(&m_OrigBtnRect);
	((CStatic *)GetDlgItem(IDC_BUTTON_CLR_BUF))->GetWindowRect(&rect);
	m_BtnSpace = rect.left - m_OrigBtnRect.left;
	ScreenToClient(&m_OrigBtnRect);

	m_DisplayDataLen = 0;
	m_nTimerCnt = 0;
	m_bFpsUpdated = 0;
	m_bJpegSeqRstFlag = 0;
	SetTimer(1, 100, NULL);

	if (!(m_RecvDecodeThread = AfxBeginThread(BackGndThread, this)))
	{
		AfxMessageBox(_T("Cannot create background thread!"));
		return FALSE;
	}

	return TRUE;
}

void CUartDisplayDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	CDialogEx::OnSysCommand(nID, lParam);
}

void CUartDisplayDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this);
		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
		UpdateImage(NULL);
	}
}

HCURSOR CUartDisplayDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CUartDisplayDlg::OnStnClickedPort()
{
}

LRESULT CUartDisplayDlg::UpdateImage(void *pParam)
{
	CRect c_rect;
	CDC *pDC;
	RECT rect;
	CBrush brush;
	char tmp[24];
	struct _BuffInfo *pBufInfo = (struct _BuffInfo *)pParam;

	if (m_pDispImg != NULL)
	{
		m_pImgDataAccessSemphr->Lock();
		LONG ImgHeight, ImgWidth;
		FLOAT divh, divw, div;
		ImgHeight = m_pDispImg->GetHeight();
		ImgWidth = m_pDispImg->GetWidth();

		if (m_ImgWidth != ImgWidth || m_ImgHeight != ImgHeight)
		{
			m_bImageNeedClear = TRUE;
			m_ImgWidth = ImgWidth;
			m_ImgHeight = ImgHeight;
			sprintf_s(tmp, 24, "Height: %d", ImgHeight);
			m_StatBar->SetText(tmp, STATUSBAR_COLUMN_HEIGHT, 0);
			sprintf_s(tmp, 24, "Width: %d", ImgWidth);
			m_StatBar->SetText(tmp, STATUSBAR_COLUMN_WIDTH, 0);
		}
		if (pBufInfo != NULL)
		{
			sprintf_s(tmp, 24, "FrameSize: %d", pBufInfo->DataSize);
			m_StatBar->SetText(tmp, STATUSBAR_COLUMN_FRAME_SIZE, 0);
		}

		m_nFrameCnt++;
		if (m_nFrameStartCounter == 0)
			m_nFrameStartCounter = timeGetTime(); // GetTickCount();
		m_nFrameEndCounter = timeGetTime();
		// If data rate is too high, the UI thread timer will not
		// response on time, so we need to update fps manually
		if ((m_nFrameEndCounter - m_nFrameStartCounter > 1000) && (m_nFrameCnt != 0))
		{
			DOUBLE fps;
			DWORD cnt;

			m_bFpsUpdated = 1;

			cnt = m_nFrameEndCounter - m_nFrameStartCounter;
			if (cnt != 0)
			{
				fps = 1000.0 * m_nFrameCnt / cnt;
				sprintf_s(tmp, 24, "FPS: %3.1f", fps);
			}
			else if (m_nFrameEndCounter == 0)
			{
				sprintf_s(tmp, 24, "FPS: 0");
			}
			else
			{
				sprintf_s(tmp, 24, "FPS: >60.0");
			}
			m_nFrameStartCounter = m_nFrameEndCounter;
			m_nFrameEndCounter = 0;
			m_nFrameCnt = 0;
			m_StatBar->SetText(tmp, STATUSBAR_COLUMN_FPS, 0);
		}

		((CStatic *)GetDlgItem(IDC_STATIC_PICTURE))->GetWindowRect(&c_rect);

		divh = (FLOAT)ImgHeight / c_rect.Height();
		divw = (FLOAT)ImgWidth / c_rect.Width();
		if (divh <= 1.0 && divw <= 1.0)
			div = 1.0;
		else
			div = divh > divw ? divh : divw;

		rect.top = c_rect.top;
		rect.left = c_rect.left;
		rect.bottom = c_rect.bottom;
		rect.right = c_rect.right;

		ScreenToClient(&rect);
		pDC = GetDC();

		if (m_bImageNeedClear)
		{
			m_bImageNeedClear = FALSE;
			pDC->FillRect(&rect, &brush);
		}

		// The image is scaled down to the original image
		rect.bottom = rect.top + (LONG)(ImgHeight / div);
		rect.right = rect.left + (LONG)(ImgWidth / div);

		// Image centering
		if (rect.bottom < rect.top + c_rect.Height())
		{
			ImgHeight = (rect.top + c_rect.Height() - rect.bottom) / 2;
			rect.top += ImgHeight;
			rect.bottom += ImgHeight;
		}
		if (rect.right < rect.left + c_rect.Width())
		{
			ImgWidth = (rect.left + c_rect.Width() - rect.right) / 2;
			rect.left += ImgWidth;
			rect.right += ImgWidth;
		}

		pDC->SetStretchBltMode(COLORONCOLOR);
		m_pDispImg->Draw(pDC->m_hDC, rect);
		ReleaseDC(pDC);
		m_pImgDataAccessSemphr->Unlock();
	}

	return TRUE;
}

LRESULT CUartDisplayDlg::OnMJPEGFrameReceived(WPARAM wParam, LPARAM lParam)
{
	UpdateImage((void *)wParam);

	return TRUE;
}

void CUartDisplayDlg::OnEnChangeEdit1()
{
}

static int PreLoadImage(struct _BuffInfo *pBufInfo, CImage **ppCImage, CSemaphore *pSemphr)
{
	IStream *ps;
	HGLOBAL hImgDataHandle;
	LPVOID pvData = NULL;
	CImage *pCImage;

	hImgDataHandle = GlobalAlloc(GMEM_MOVEABLE, pBufInfo->DataSize);
	if (hImgDataHandle == NULL)
	{
		return FALSE;
	}
	pCImage = new CImage();
	if (pCImage == NULL)
	{
		GlobalFree(hImgDataHandle);
		return FALSE;
	}

	if ((pvData = GlobalLock(hImgDataHandle)) != NULL) // Locking memory allocation blocks
	{
		memcpy(pvData, pBufInfo->DataBuffer, pBufInfo->DataSize);
		GlobalUnlock(hImgDataHandle);
		CreateStreamOnHGlobal(hImgDataHandle, FALSE, &ps);
	}
	else
	{
		GlobalFree(hImgDataHandle);
		delete pCImage;
		pCImage = NULL;
		return FALSE;
	}

	if (pCImage->Load(ps) != 0)
	{
		ps->Release();
		GlobalFree(hImgDataHandle);
		delete pCImage;
		return FALSE;
	}
	ps->Release();
	GlobalFree(hImgDataHandle);
	pSemphr->Lock();
	delete *ppCImage;
	*ppCImage = pCImage;
	pSemphr->Unlock();
	return TRUE;
}

static int SaveImageToFile(CString *pPath, UINT *nFileNameCnt, CTime *tLastTime, struct _BuffInfo *pBufInfo)
{
	CTime t;
	TCHAR FileName[32], *p;
	CFile ImgFile;
	CFileException err;

	CString tmp = *pPath;
	t = CTime::GetCurrentTime();
	if (t != *tLastTime)
	{
		*nFileNameCnt = 0;
		*tLastTime = t;
	}
	else
	{
		(*nFileNameCnt)++;
	}
	sprintf_s(FileName, 32, "\\%4d%2d%2d_%2d%2d%2d_%3d.jpg",
			  t.GetYear(), t.GetMonth(), t.GetDay(),
			  t.GetHour(), t.GetMinute(), t.GetSecond(), *nFileNameCnt);
	p = FileName;
	for (UINT i = 0; i < 32; i++)
	{
		if (p[i] == ' ')
			p[i] = '0';
	}
	tmp.Append(FileName);
	if (ImgFile.Open(tmp, CFile::modeWrite | CFile::modeCreate, &err))
	{
		ImgFile.Write(pBufInfo->DataBuffer, pBufInfo->DataSize);
		ImgFile.Close();
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

#define RECV_BUFF_SIZE 102400
static unsigned char Buffer[RECV_BUFF_SIZE];
static UINT BackGndThread(LPVOID lpParam)
{
	CUartDisplayDlg *pDlg = (CUartDisplayDlg *)lpParam;
	DWORD BytesRead = 0;
	BOOL ret;
	UINT i, j;
	int MjpegStreamFlag = 0;
	UINT MjpegHeaderCnt = 0;
	UINT MjpegTailCnt = 0;
	BOOL bFrameReceived = FALSE;
	unsigned char *pbuf;

	while (true)
	{
		pbuf = Buffer;
		ret = ReadFile(pDlg->m_hRead, Buffer, RECV_BUFF_SIZE, &BytesRead, 0);
		if (ret != TRUE)
		{
			TRACE(_T("Receiver thread exited!"));
			break;
		}
		if (BytesRead > 0)
		{
			i = j = 0;
			pDlg->m_RxCnt += BytesRead;

			if (pDlg->m_bJpegSeqRstFlag)
			{
				MjpegStreamFlag = 0;
				MjpegHeaderCnt = 0;
				MjpegTailCnt = 0;
				bFrameReceived = FALSE;
				pDlg->m_bJpegSeqRstFlag = FALSE;
			}
			if (MjpegStreamFlag == 1) // is mjpeg stream
			{
			mjpg_sequence_start:
				if (MjpegTailCnt > 0)
				{
					if (pbuf[0] == 0xD9)
					{
						if (MjpegFrameBufferInfo[0].DataSize + 1 >= MJPEG_FRAME_BUFFER_SIZE)
						{
							MjpegTailCnt = 0;
							MjpegHeaderCnt = 0;
							MjpegStreamFlag = 0;
							goto mjpg_sequence_reset;
						}
						MjpegFrameBufferInfo[0].DataBuffer[MjpegFrameBufferInfo[0].DataSize++] = 0xD9;
						pDlg->m_pImgDataAccessSemphr->Lock();
						memcpy(MjpegFrameBufferInfo[1].DataBuffer, MjpegFrameBufferInfo[0].DataBuffer, MjpegFrameBufferInfo[0].DataSize);
						pDlg->m_pImgDataAccessSemphr->Unlock();
						MjpegFrameBufferInfo[1].DataSize = MjpegFrameBufferInfo[0].DataSize;
						bFrameReceived = TRUE;
						MjpegTailCnt = 0;
						MjpegHeaderCnt = 0;
						MjpegStreamFlag = 0;
						MjpegFrameBufferInfo[0].DataSize = 0;
						BytesRead -= 1;
						pbuf += 1;
						if (PreLoadImage(&MjpegFrameBufferInfo[1], &pDlg->m_pDispImg, pDlg->m_pImgDataAccessSemphr))
						{
							if (!pDlg->m_ImgFilePath.IsEmpty())
							{
								SaveImageToFile(&pDlg->m_ImgFilePath, &pDlg->m_FileNameCnt, &pDlg->m_TimeNow, &MjpegFrameBufferInfo[1]);
							}
							PostMessage(pDlg->m_hWnd, WM_USER_MJPEG_FRAME_RECVED, (WPARAM)&MjpegFrameBufferInfo[1], (LPARAM)NULL);
						}

						goto mjpg_sequence_reset;
					}
					else
					{
						MjpegTailCnt = 0;
					}
				}
				if (BytesRead == 1)
				{
					MjpegFrameBufferInfo[0].DataBuffer[MjpegFrameBufferInfo[0].DataSize++] = pbuf[0];
					continue;
				}
				for (i = 1; i < BytesRead; i++)
				{
					if ((pbuf[i] == 0xD9) && (pbuf[i - 1] == 0xFF))
						break;
				}
				if (MjpegFrameBufferInfo[0].DataSize + i + 1 >= MJPEG_FRAME_BUFFER_SIZE)
				{
					MjpegTailCnt = 0;
					MjpegHeaderCnt = 0;
					MjpegStreamFlag = 0;
					goto mjpg_sequence_reset;
				}
				if (i < BytesRead)
				{
					memcpy(&MjpegFrameBufferInfo[0].DataBuffer[MjpegFrameBufferInfo[0].DataSize], pbuf, i + 1);
					MjpegFrameBufferInfo[0].DataSize += i + 1;
					pDlg->m_pImgDataAccessSemphr->Lock();
					memcpy(MjpegFrameBufferInfo[1].DataBuffer, MjpegFrameBufferInfo[0].DataBuffer, MjpegFrameBufferInfo[0].DataSize);
					pDlg->m_pImgDataAccessSemphr->Unlock();
					MjpegFrameBufferInfo[1].DataSize = MjpegFrameBufferInfo[0].DataSize;
					bFrameReceived = TRUE;
					MjpegTailCnt = 0;
					MjpegHeaderCnt = 0;
					MjpegStreamFlag = 0;
					MjpegFrameBufferInfo[0].DataSize = 0;
					BytesRead = BytesRead - i - 1;
					pbuf += i + 1;
					if (PreLoadImage(&MjpegFrameBufferInfo[1], &pDlg->m_pDispImg, pDlg->m_pImgDataAccessSemphr))
					{
						if (!pDlg->m_ImgFilePath.IsEmpty())
						{
							SaveImageToFile(&pDlg->m_ImgFilePath, &pDlg->m_FileNameCnt, &pDlg->m_TimeNow, &MjpegFrameBufferInfo[1]);
						}
						PostMessage(pDlg->m_hWnd, WM_USER_MJPEG_FRAME_RECVED, (WPARAM)&MjpegFrameBufferInfo[1], (LPARAM)NULL);
					}
					goto mjpg_sequence_reset;
				}
				else
				{
					memcpy(&MjpegFrameBufferInfo[0].DataBuffer[MjpegFrameBufferInfo[0].DataSize], pbuf, i);
					MjpegFrameBufferInfo[0].DataSize += i;
					if (pbuf[BytesRead - 1] == 0xFF)
						MjpegTailCnt = 1;
				}
			}
			else
			{
			mjpg_sequence_reset:
				for (i = 0; i < BytesRead; i++)
				{
					j = BytesRead - i;
					if (j > sizeof(MjpegHeader) - MjpegHeaderCnt)
						j = sizeof(MjpegHeader) - MjpegHeaderCnt;
					if (memcmp(&pbuf[i], &MjpegHeader[MjpegHeaderCnt], j) == 0)
					{
						if (j == sizeof(MjpegHeader))
						{
							MjpegHeaderCnt = 0;
							MjpegStreamFlag = 1;
						}
						else
						{
							MjpegHeaderCnt += j;
							if (MjpegHeaderCnt == sizeof(MjpegHeader))
							{
								MjpegStreamFlag = 1;
							}
						}

						memcpy(&MjpegFrameBufferInfo[0].DataBuffer[MjpegFrameBufferInfo[0].DataSize], &pbuf[i], j);
						MjpegFrameBufferInfo[0].DataSize += j;
						if (BytesRead - i > j && MjpegStreamFlag == 1)
						{
							BytesRead = BytesRead - i - j;
							pbuf += i + j;
							goto mjpg_sequence_start;
						}
					}
					else
					{
						MjpegHeaderCnt = 0;
						if (pbuf[i] != 0)
						{
							pDlg->m_ReceivedDisplayData.AppendChar(pbuf[i]);
							if (pDlg->m_ReceivedDisplayData.GetLength() > 50 * 1024)
								pDlg->m_ReceivedDisplayData.Empty();
						}
					}
				}
			}
		}
	}
	AfxEndThread(100);
	return TRUE;
}

void CUartDisplayDlg::OnBnClickedButtonClrImg()
{
	CDC *pDC;
	CBrush *brush = new CBrush;
	CRect c_rect;
	m_pImgDataAccessSemphr->Lock();
	((CStatic *)GetDlgItem(IDC_STATIC_PICTURE))->GetWindowRect(&c_rect);
	ScreenToClient(&c_rect);
	pDC = GetDC();
	pDC->FillRect(&c_rect, brush);
	ReleaseDC(pDC);
	delete brush;

	if (m_pDispImg != NULL)
	{
		delete m_pDispImg;
	}
	m_pDispImg = NULL;
	m_StatBar->SetText("FrameSize: 0", STATUSBAR_COLUMN_FRAME_SIZE, 0);
	m_StatBar->SetText("FPS: 0", STATUSBAR_COLUMN_FPS, 0);
	m_StatBar->SetText("Image Width: 0", STATUSBAR_COLUMN_WIDTH, 0);
	m_StatBar->SetText("Image Height:0", STATUSBAR_COLUMN_HEIGHT, 0);
	m_pImgDataAccessSemphr->Unlock();
	MjpegFrameBufferInfo[0].DataSize = 0;
	MjpegFrameBufferInfo[1].DataSize = 0;
	m_nFrameEndCounter = 0;
	m_nFrameStartCounter = 0;
	m_nFrameCnt = 0;
	m_ImgWidth = 0;
	m_ImgHeight = 0;
	m_bJpegSeqRstFlag = TRUE;
}

void CUartDisplayDlg::OnBnClickedButtonClrBuf()
{
	m_ReceivedDisplayData.Empty();
	m_bJpegSeqRstFlag = TRUE;
	UpdateData(FALSE);
}

void CUartDisplayDlg::OnBnClickedButtonClrCnt()
{
	m_RxCnt = 0;
	m_LastRxCnt = 0;
	m_TxCnt = 0;
	m_StatBar->SetText("TX: 0", STATUSBAR_COLUMN_TX_CNT, 0);
	m_StatBar->SetText("RX: 0", STATUSBAR_COLUMN_RX_CNT, 0);
}

void CUartDisplayDlg::OnTcnSelchangeTab1(NMHDR *pNMHDR, LRESULT *pResult)
{
	*pResult = 0;
	int CurSel = m_tab1.GetCurSel();
	switch (CurSel)
	{
	case 0:
		m_DialogUartCfg.ShowWindow(TRUE);
		m_DialogNetCfg.ShowWindow(FALSE);
		break;
	case 1:
		m_DialogUartCfg.ShowWindow(FALSE);
		m_DialogNetCfg.ShowWindow(TRUE);
		m_DialogNetCfg.UpdateData(FALSE);
		break;
	default:
		break;
	}
}

void CUartDisplayDlg::OnClose()
{
	if (m_DialogNetCfg.m_pClientSocket)
		m_DialogNetCfg.OnClientDisconnect(0);
	if (m_hWrite != NULL)
		CloseHandle(m_hWrite);
	if (m_hRead != NULL)
		CloseHandle(m_hRead);
	CDialogEx::OnClose();
}

// This Timer is not accurate, the response of this timer will be delayed or even not respond when UI transactions are busy
// The time calculation does not take into account the overflow case for now (the receive/send count is not considered either :))
void CUartDisplayDlg::OnTimer(UINT_PTR nIDEvent)
{
	CDialogEx::OnTimer(nIDEvent);
	char tmp[32];
	m_nTimerCnt++;

	sprintf_s(tmp, 32, "RX: %d", m_RxCnt);
	m_StatBar->SetText(tmp, STATUSBAR_COLUMN_RX_CNT, 0);

	if (m_DisplayDataLen != m_ReceivedDisplayData.GetLength())
	{
		m_DisplayDataLen = m_ReceivedDisplayData.GetLength();
		m_ReceivedDisplayData.Replace(_T("\n\r"), _T("\r\n"));
		UpdateData(FALSE);
		m_pEdit->SetSel(m_DisplayDataLen, m_DisplayDataLen);
	}

	if (m_nTimerCnt >= 10)
	{
		m_nTimerCnt = 0;
		// update fps value
		DOUBLE fps;
		DWORD cnt;
		DWORD tempTime = timeGetTime();
		if (m_bFpsUpdated == 0)
		{
			cnt = m_nFrameEndCounter - m_nFrameStartCounter;

			if (cnt > 0)
			{
				fps = 1000.0 * m_nFrameCnt / cnt;
				sprintf_s(tmp, 32, "FPS: %3.1f", fps);
			}
			else if (m_nFrameEndCounter > 0 && m_nFrameCnt > 0)
			{
				sprintf_s(tmp, 32, "FPS: >60.0");
			}
			else
			{
				sprintf_s(tmp, 32, "FPS: 0");
			}

			m_nFrameStartCounter = m_nFrameEndCounter;
			m_nFrameEndCounter = 0;
			m_nFrameCnt = 0;
			m_StatBar->SetText(tmp, STATUSBAR_COLUMN_FPS, 0);
		}
		else
		{
			m_bFpsUpdated = 0;
		}
		if (m_LastRxCnt != m_RxCnt && m_RxCnt != 0)
		{
			cnt = tempTime - m_LastRxSpeedUpdateTime;
			m_LastRxSpeedUpdateTime = tempTime;
			tempTime = cnt / 1000;
			if (tempTime == 0)
				return;
			cnt = (m_RxCnt - m_LastRxCnt) / tempTime;
			if (cnt < 1024 * 1024)
			{
				sprintf_s(tmp, 32, "RX Speed: %d B/s", cnt);
			}
			else
			{
				fps = cnt / 1048576.0;
				sprintf_s(tmp, 32, "RX Speed: %3.2f MB/s", fps);
			}
			m_StatBar->SetText(tmp, STATUSBAR_COLUMN_RX_SPEED, 0);
			m_LastRxCnt = m_RxCnt;
		}
		else
		{
			sprintf_s(tmp, 32, "RX Speed: 0 B/s");
			m_StatBar->SetText(tmp, STATUSBAR_COLUMN_RX_SPEED, 0);
		}
	}
}

void CUartDisplayDlg::OnBnClickedCheckSaveImage()
{
	// UpdateData(TRUE);
	if (((CButton *)GetDlgItem(IDC_CHECK_SAVE_IMAGE))->GetCheck() == TRUE)
	{
		BROWSEINFO bi;
		ITEMIDLIST *pidl;
		TCHAR szDir[256];

		bi.hwndOwner = this->m_hWnd;
		bi.pidlRoot = NULL;
		bi.pszDisplayName = szDir;
		bi.lpszTitle = _T("Please select directory to save!");
		bi.ulFlags = BIF_RETURNONLYFSDIRS;
		bi.lpfn = NULL;
		bi.lParam = 0;
		bi.iImage = 0;

		pidl = SHBrowseForFolder(&bi);
		if (pidl == NULL)
		{
			((CButton *)GetDlgItem(IDC_CHECK_SAVE_IMAGE))->SetCheck(FALSE);
			return;
		}

		if (SHGetPathFromIDList(pidl, szDir))
		{
			m_ImgFilePath.Empty();
			m_ImgFilePath.Format(_T("%s"), szDir);
			m_bSaveToFile = TRUE;
		}
	}
	else
	{
		m_ImgFilePath.Empty();
		m_bSaveToFile = FALSE;
	}
	UpdateData(FALSE);
}

void CUartDisplayDlg::OnStnDblclickStaticPicture()
{
	CRect c_rect;
	if (m_bExpandDisplay != TRUE)
	{
		if (m_pDispImg != NULL)
		{
			if (::IsWindow(this->GetSafeHwnd()))
			{
				this->GetClientRect(&c_rect);
				c_rect.bottom -= 26;
				((CStatic *)GetDlgItem(IDC_STATIC_PICTURE))->MoveWindow(&c_rect);

				((CEdit *)GetDlgItem(IDC_EDIT1))->ShowWindow(SW_HIDE);
				m_tab1.ShowWindow(SW_HIDE);
				((CButton *)GetDlgItem(IDC_BUTTON_CLR_IMG))->ShowWindow(SW_HIDE);
				((CButton *)GetDlgItem(IDC_BUTTON_CLR_BUF))->ShowWindow(SW_HIDE);
				((CButton *)GetDlgItem(IDC_BUTTON_CLR_CNT))->ShowWindow(SW_HIDE);
				((CButton *)GetDlgItem(IDC_CHECK_SAVE_IMAGE))->ShowWindow(SW_HIDE);
				((CStatic *)GetDlgItem(IDC_STATIC1))->ShowWindow(SW_HIDE);
				((CStatic *)GetDlgItem(IDC_STATIC2))->ShowWindow(SW_HIDE);

				m_bImageNeedClear = TRUE;
				m_bExpandDisplay = TRUE;
				// UpdateImage(NULL);
			}
		}
	}
	else
	{
		m_bExpandDisplay = FALSE;
		if (::IsWindow(this->GetSafeHwnd()))
		{
			this->GetClientRect(&c_rect);
			c_rect.left = m_OrigTabRect.right + m_OrigImageRect.left - m_OrigTabRect.right;
			c_rect.top = m_OrigTabRect.top;
			c_rect.bottom = c_rect.bottom - (m_OrigWndRect.bottom - m_OrigImageRect.bottom);
			c_rect.right = c_rect.right - (m_OrigWndRect.right - m_OrigImageRect.right);
			((CStatic *)GetDlgItem(IDC_STATIC_PICTURE))->MoveWindow(&c_rect);
			m_bImageNeedClear = TRUE;

			((CStatic *)GetDlgItem(IDC_STATIC1))->ShowWindow(SW_SHOW);
			((CStatic *)GetDlgItem(IDC_STATIC2))->ShowWindow(SW_SHOW);
			((CEdit *)GetDlgItem(IDC_EDIT1))->ShowWindow(SW_SHOW);
			m_tab1.ShowWindow(SW_SHOW);
			((CButton *)GetDlgItem(IDC_BUTTON_CLR_IMG))->ShowWindow(SW_SHOW);
			((CButton *)GetDlgItem(IDC_BUTTON_CLR_BUF))->ShowWindow(SW_SHOW);
			((CButton *)GetDlgItem(IDC_BUTTON_CLR_CNT))->ShowWindow(SW_SHOW);
			((CButton *)GetDlgItem(IDC_CHECK_SAVE_IMAGE))->ShowWindow(SW_SHOW);
		}
	}
	this->OnPaint();
}

void CUartDisplayDlg::OnSize(UINT nType, int cx, int cy)
{
	CDialogEx::OnSize(nType, cx, cy);
	CRect c_rect;
	CRect c_status_bar_rect, c_wnd_rect, c_edit_rect;

	if (nType == SIZE_MAXIMIZED || nType == SIZE_RESTORED)
	{
		if (::IsWindow(((CStatic *)GetDlgItem(IDC_STATIC_PICTURE))->GetSafeHwnd()))
		{

			this->GetClientRect(&c_wnd_rect);

			m_tab1.MoveWindow(&m_OrigTabRect);

			// Move status bar
			c_status_bar_rect.top = c_wnd_rect.bottom - 26;
			c_status_bar_rect.bottom = c_wnd_rect.bottom;
			c_status_bar_rect.left = c_wnd_rect.left;
			c_status_bar_rect.right = c_wnd_rect.right;
			m_StatBar->MoveWindow(&c_status_bar_rect);

			// Move RX text area edit control
			//((CEdit *)GetDlgItem(IDC_EDIT1))->GetClientRect(&c_rect);
			c_rect.bottom = c_wnd_rect.bottom - (m_OrigWndRect.bottom - m_OrigEditRect.bottom);
			c_rect.top = m_OrigEditRect.top;
			c_rect.left = m_OrigTabRect.left;
			c_rect.right = m_OrigTabRect.right;
			((CEdit *)GetDlgItem(IDC_EDIT1))->MoveWindow(&c_rect);

			// Move image window
			//((CStatic *)GetDlgItem(IDC_STATIC_PICTURE))->GetClientRect(&c_img_rect);
			if (m_bExpandDisplay == FALSE)
			{
				c_rect.left = m_OrigTabRect.right + m_OrigImageRect.left - m_OrigTabRect.right;
				c_rect.top = m_OrigTabRect.top;
				c_rect.bottom = c_wnd_rect.bottom - (m_OrigWndRect.bottom - m_OrigImageRect.bottom);
				c_rect.right = c_wnd_rect.right - (m_OrigWndRect.right - m_OrigImageRect.right);
				((CStatic *)GetDlgItem(IDC_STATIC_PICTURE))->MoveWindow(&c_rect);
			}

			// Move buttons
			c_rect.left = m_OrigBtnRect.left;
			c_rect.top = c_wnd_rect.bottom - (m_OrigWndRect.bottom - m_OrigBtnRect.top);
			c_rect.bottom = c_wnd_rect.bottom - (m_OrigWndRect.bottom - m_OrigBtnRect.bottom);
			c_rect.right = m_OrigBtnRect.right;
			((CButton *)GetDlgItem(IDC_BUTTON_CLR_IMG))->MoveWindow(&c_rect);
			c_rect.left += m_BtnSpace;
			c_rect.right += m_BtnSpace;
			((CButton *)GetDlgItem(IDC_BUTTON_CLR_BUF))->MoveWindow(&c_rect);
			c_rect.left += m_BtnSpace;
			c_rect.right += m_BtnSpace;
			((CButton *)GetDlgItem(IDC_BUTTON_CLR_CNT))->MoveWindow(&c_rect);
			c_rect.left += m_BtnSpace;
			c_rect.right += m_BtnSpace + 20;
			((CButton *)GetDlgItem(IDC_CHECK_SAVE_IMAGE))->MoveWindow(&c_rect);
			m_bImageNeedClear = TRUE;
			if (m_bExpandDisplay == TRUE)
			{
				c_rect = c_wnd_rect;
				c_rect.bottom -= 26;
				((CStatic *)GetDlgItem(IDC_STATIC_PICTURE))->MoveWindow(&c_rect);
			}
		}
	}
}
