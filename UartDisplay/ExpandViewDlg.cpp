// 

#include "stdafx.h"
#include "UartDisplay.h"
#include "ExpandViewDlg.h"
#include "afxdialogex.h"
#include "UartDisplayDlg.h"


IMPLEMENT_DYNAMIC(ExpandViewDlg, CDialogEx)

ExpandViewDlg::ExpandViewDlg(CWnd *pParent /*=NULL*/)
	: CDialogEx(ExpandViewDlg::IDD, pParent)
{
}

ExpandViewDlg::~ExpandViewDlg()
{
}

void ExpandViewDlg::DoDataExchange(CDataExchange *pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(ExpandViewDlg, CDialogEx)

ON_WM_PAINT()
END_MESSAGE_MAP()

BOOL ExpandViewDlg::PreTranslateMessage(MSG *pMsg)
{
	if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_ESCAPE)
	{
		m_pParentDlg = 0;
		return FALSE;
	}
	return CDialogEx::PreTranslateMessage(pMsg);
}

void ExpandViewDlg::SetParentDlg(void *pDlg)
{
	m_pParentDlg = pDlg;
}

void ExpandViewDlg::OnPaint()
{
	// Actualizar la UI del cuadro de di¨¢logo padre
	CPaintDC dc(this);
	if (m_pParentDlg)
		((CUartDisplayDlg *)m_pParentDlg)->UpdateImage(NULL);
}
