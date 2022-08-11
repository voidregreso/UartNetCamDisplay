#pragma once

#include "SerialPort.h"

int GetComList_Reg(CComboBox *CCombox);

// CDialogUartCfg 对话框

class CDialogUartCfg : public CDialog
{
	DECLARE_DYNAMIC(CDialogUartCfg)

public:
	CDialogUartCfg(CWnd *pParent = NULL); // 标准构造函数
	virtual ~CDialogUartCfg();

	// 对话框数据
	enum
	{
		IDD = IDD_DIALOG_UART_CFG
	};

protected:
	virtual void DoDataExchange(CDataExchange *pDX); // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonOnOff();
	//	afx_msg void OnCbnSelchangeComboPort();
	afx_msg void OnDropdownComboPort();
	bool m_bComOpenFlag;
	CSerialPort *m_pSerialPort;
	void *m_pParentDlg;
	void SetParentDlg(void *pdlg);
};
