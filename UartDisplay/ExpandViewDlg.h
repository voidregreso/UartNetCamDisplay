#pragma once

// ExpandViewDlg 对话框

class ExpandViewDlg : public CDialogEx
{
	DECLARE_DYNAMIC(ExpandViewDlg)

public:
	ExpandViewDlg(CWnd *pParent = NULL); // 标准构造函数
	virtual ~ExpandViewDlg();

	// 对话框数据
	enum
	{
		IDD = IDD_DIALOG_EXPAND_DISPLAY
	};

protected:
	virtual void DoDataExchange(CDataExchange *pDX); // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL PreTranslateMessage(MSG *pMsg);
	//	afx_msg void OnMove(int x, int y);
	//	afx_msg void OnPaint();
	void SetParentDlg(void *pDlg);
	void *m_pParentDlg;
	//	afx_msg void OnMove(int x, int y);
	afx_msg void OnPaint();
};
