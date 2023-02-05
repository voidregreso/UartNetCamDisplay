// Clase de di¨¢logo de configuraci¨®n de la comunicaci¨®n del puerto serie

#include "stdafx.h"
#include "UartDisplay.h"
#include "DialogUartCfg.h"
#include "afxdialogex.h"
#include "SerialPort.h"
#include "UartDisplayDlg.h"



IMPLEMENT_DYNAMIC(CDialogUartCfg, CDialog)

CDialogUartCfg::CDialogUartCfg(CWnd* pParent /*=NULL*/)
	: CDialog(CDialogUartCfg::IDD, pParent)
	, m_bComOpenFlag(FALSE)
	, m_pSerialPort(NULL)
	, m_pParentDlg(NULL)
{

}

CDialogUartCfg::~CDialogUartCfg()
{
	if (m_pSerialPort)
		delete m_pSerialPort;
}

void CDialogUartCfg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

void CDialogUartCfg::SetParentDlg(void* pDlg)
{
	m_pParentDlg = pDlg;
}

BEGIN_MESSAGE_MAP(CDialogUartCfg, CDialog)
	ON_BN_CLICKED(IDC_BUTTON_ON_OFF, &CDialogUartCfg::OnBnClickedButtonOnOff)
	ON_CBN_DROPDOWN(IDC_COMBO_PORT, &CDialogUartCfg::OnDropdownComboPort)
END_MESSAGE_MAP()


void CDialogUartCfg::OnBnClickedButtonOnOff()
{

	UINT portnr, baud, Index;
	CString selText;
	CUartDisplayDlg *pDlg = (CUartDisplayDlg *)m_pParentDlg;

	if (!m_bComOpenFlag)
	{
		m_pSerialPort = new CSerialPort();
		Index = ((CComboBox *)GetDlgItem(IDC_COMBO_PORT))->GetCurSel();
		((CComboBox *)GetDlgItem(IDC_COMBO_PORT))->GetLBText(Index, selText);
		portnr = _ttoi(selText.Mid(3));
		Index = ((CComboBox *)GetDlgItem(IDC_COMBO_BAUD))->GetCurSel();
		((CComboBox *)GetDlgItem(IDC_COMBO_BAUD))->GetLBText(Index, selText);
		baud = _ttoi(selText);
		if (m_pSerialPort->InitPort(pDlg, pDlg->m_hWrite, pDlg->m_hRead, portnr, baud) 
			&& m_pSerialPort->StartMonitoring())
		{
			m_bComOpenFlag = TRUE;
			((CComboBox *)GetDlgItem(IDC_COMBO_PORT))->EnableWindow(FALSE);
			((CComboBox *)GetDlgItem(IDC_COMBO_BAUD))->EnableWindow(FALSE);
			SetDlgItemText(IDC_BUTTON_ON_OFF, _T("Close USART"));
		}
		else
		{
			AfxMessageBox(_T("Failed to open serial port!"));
			delete m_pSerialPort;
			m_pSerialPort = NULL;
		}
	}
	else
	{
		//pDlg->m_pSerialPort->StopMonitoring();
		delete m_pSerialPort;
		m_bComOpenFlag = FALSE;
		m_pSerialPort = NULL;
		((CComboBox *)GetDlgItem(IDC_COMBO_PORT))->EnableWindow(TRUE);
		((CComboBox *)GetDlgItem(IDC_COMBO_BAUD))->EnableWindow(TRUE);
		SetDlgItemText(IDC_BUTTON_ON_OFF, _T("Open USART"));
	}
}


int GetComList_Reg(CComboBox *CCombox)
{
	HKEY hkey;
	int result;
	int i = 0;

	CString strComName; // Nombre del puerto serie
	CString strDrName; // Nombre detallado del puerto serie

	result = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
		_T("Hardware\\DeviceMap\\SerialComm"),
		NULL,
		KEY_READ,
		&hkey);

	if (ERROR_SUCCESS == result)
	{
		TCHAR portName[0x100], commName[0x100];
		DWORD dwLong, dwSize;

		CCombox->ResetContent();
		do
		{
			dwSize = sizeof(portName) / sizeof(TCHAR);
			dwLong = dwSize;
			result = RegEnumValue(hkey, i, portName, &dwLong, NULL, NULL, (LPBYTE)commName, &dwSize);
			if (ERROR_NO_MORE_ITEMS == result)
			{
				// Enumeraci¨®n de PSs
				break; 
			}

			CCombox->AddString(commName);

			i++;
		} while (1);

		RegCloseKey(hkey);
	}

	CCombox->SetCurSel(0);
	return i;
}

void CDialogUartCfg::OnDropdownComboPort()
{
	GetComList_Reg((CComboBox *)GetDlgItem(IDC_COMBO_PORT));
}
