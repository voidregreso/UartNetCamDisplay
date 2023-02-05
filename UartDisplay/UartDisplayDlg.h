
#pragma once

#include "afxcmn.h"
#include "DialogNetCfg.h"
#include "DialogUartCfg.h"
#include "ExpandViewDlg.h"

#define WM_USER_MJPEG_FRAME_RECVED ((WM_USER) + 10)
#define WM_USER_DATA_RECVED ((WM_USER) + 11)

#define STATUSBAR_COLUMN_TX_CNT 0
#define STATUSBAR_COLUMN_RX_CNT 1
#define STATUSBAR_COLUMN_RX_SPEED 2
#define STATUSBAR_COLUMN_FRAME_SIZE 3
#define STATUSBAR_COLUMN_FPS 4
#define STATUSBAR_COLUMN_WIDTH 5
#define STATUSBAR_COLUMN_HEIGHT 6
#define EDIT_CONTENT_UPDATE 100

class CUartDisplayDlg : public CDialogEx
{
public:
	CUartDisplayDlg(CWnd *pParent = NULL);

	enum
	{
		IDD = IDD_UARTDISPLAY_DIALOG
	};

protected:
	virtual void DoDataExchange(CDataExchange *pDX);

protected:
	HICON m_hIcon;

	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	CStatusBarCtrl *m_StatBar;
	afx_msg void OnStnClickedPort();
	afx_msg LRESULT OnMJPEGFrameReceived(WPARAM wParam, LPARAM lParam);
	CWinThread *m_RecvDecodeThread;

	CString m_ReceivedDisplayData;
	int m_DisplayDataLen;
	CString m_ToBeDisplayedData;
	CEdit *m_pEdit;
	CImage *m_pDispImg;

	CSemaphore *m_pImgDataAccessSemphr;
	LRESULT UpdateImage(void *pParam);
	afx_msg void OnEnChangeEdit1();
	DWORD m_RxCnt;
	DWORD m_TxCnt;
	DWORD m_LastRxCnt;
	DWORD m_LastRxSpeedUpdateTime;
	DWORD m_nFrameStartCounter;
	DWORD m_nFrameEndCounter;

	HANDLE m_hRead;
	HANDLE m_hWrite;

	CFileException m_FileErr;
	CString m_ImgFilePath;
	UINT m_FileNameCnt;
	CTime m_TimeNow;
	UINT m_nTimerCnt;

	UINT m_nFrameCnt;
	BOOL m_bFpsUpdated;
	UINT m_ImgWidth;
	UINT m_ImgHeight;

	BOOL m_bExpandDisplay;
	BOOL m_bImageNeedClear;
	BOOL m_bJpegSeqRstFlag;

	CRect m_OrigWndRect;
	CRect m_OrigTabRect;
	CRect m_OrigEditRect;
	CRect m_OrigImageRect;
	CRect m_OrigBtnRect;
	UINT m_BtnSpace;

	afx_msg void OnBnClickedButtonClrImg();

	afx_msg void OnBnClickedButtonClrBuf();

	afx_msg void OnBnClickedButtonClrCnt();
	CTabCtrl m_tab1;
	CDialogNetCfg m_DialogNetCfg;
	CDialogUartCfg m_DialogUartCfg;
	afx_msg void OnTcnSelchangeTab1(NMHDR *pNMHDR, LRESULT *pResult);

	BOOL m_bSaveToFile;
	afx_msg void OnClose();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedCheckSaveImage();
	afx_msg void OnStnDblclickStaticPicture();
	afx_msg void OnSize(UINT nType, int cx, int cy);
};
