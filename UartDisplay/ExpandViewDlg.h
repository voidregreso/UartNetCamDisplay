#pragma once


class ExpandViewDlg : public CDialogEx
{
	DECLARE_DYNAMIC(ExpandViewDlg)

public:
	ExpandViewDlg(CWnd *pParent = NULL);
	virtual ~ExpandViewDlg();

	enum
	{
		IDD = IDD_DIALOG_EXPAND_DISPLAY
	};

protected:
	virtual void DoDataExchange(CDataExchange *pDX);

	DECLARE_MESSAGE_MAP()
public:
	virtual BOOL PreTranslateMessage(MSG *pMsg);
	void SetParentDlg(void *pDlg);
	void *m_pParentDlg;
	afx_msg void OnPaint();
};
