
#pragma once

#ifndef __AFXWIN_H__
#error "Incluya "stdafx.h" antes de incluir este archivo para generar el archivo PCH"
#endif

#include "resource.h" // S¨ªmbolo principal

// CUartDisplayApp:
// Para una implementaci¨®n de esto, v¨¦ase UartDisplay.cpp
//

class CUartDisplayApp : public CWinApp
{
public:
	CUartDisplayApp();

	// Reescritura
public:
	virtual BOOL InitInstance();

	// Implementaci¨®n

	DECLARE_MESSAGE_MAP()
};

extern CUartDisplayApp theApp;