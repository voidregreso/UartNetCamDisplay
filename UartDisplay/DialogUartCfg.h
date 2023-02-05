#pragma once

#include "SerialPort.h"

int GetComList_Reg(CComboBox *CCombox);

class CDialogUartCfg : public CDialog
{
	DECLARE_DYNAMIC(CDialogUartCfg)

public:
	CDialogUartCfg(CWnd *pParent = NULL); 
	virtual ~CDialogUartCfg();

	enum
	{
		IDD = IDD_DIALOG_UART_CFG
	};

protected:
	virtual void DoDataExchange(CDataExchange *pDX);

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonOnOff();
	afx_msg void OnDropdownComboPort();
	bool m_bComOpenFlag;
	CSerialPort *m_pSerialPort;
	void *m_pParentDlg;
	void SetParentDlg(void *pdlg);
};
