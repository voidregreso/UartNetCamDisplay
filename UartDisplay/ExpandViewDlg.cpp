// ExpandViewDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "UartDisplay.h"
#include "ExpandViewDlg.h"
#include "afxdialogex.h"
#include "UartDisplayDlg.h"

// ExpandViewDlg 对话框

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
//	ON_WM_MOVE()
//	ON_WM_PAINT()
//	ON_WM_MOVE()
ON_WM_PAINT()
END_MESSAGE_MAP()

// ExpandViewDlg 消息处理程序

BOOL ExpandViewDlg::PreTranslateMessage(MSG *pMsg)
{
	// TODO:  在此添加专用代码和/或调用基类
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
	CPaintDC dc(this); // device context for painting
	// TODO:  在此处添加消息处理程序代码
	if (m_pParentDlg)
		((CUartDisplayDlg *)m_pParentDlg)->UpdateImage(NULL);
	// 不为绘图消息调用 CDialogEx::OnPaint()
}
