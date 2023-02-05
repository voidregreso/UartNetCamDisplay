
// stdafx.h : el archivo de inclusi¨®n para el archivo de inclusi¨®n est¨¢ndar del sistema.
// o de uso frecuente pero cambio poco frecuente
// archivos de inclusi¨®n espec¨ªficos del proyecto

#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN            // Excluir el material poco utilizado del encabezado de Windows
#endif

#include "targetver.h"

#define _ATL_CSTRING_EXPLICIT_CONSTRUCTORS      // Algunos constructores CString ser¨¢n expl¨ªcitos

#define _AFX_ALL_WARNINGS

#include <afxwin.h>         // Componentes b¨¢sicos y est¨¢ndar de MFC
#include <afxext.h>         // Extensiones MFC


#include <afxdisp.h>        // Clase de automatizaci¨®n MFC



#ifndef _AFX_NO_OLE_SUPPORT
#include <afxdtctl.h>           // Compatibilidad de MFC con los controles p¨²blicos de Internet Explorer 4
#endif
#ifndef _AFX_NO_AFXCMN_SUPPORT
#include <afxcmn.h>             // Compatibilidad de MFC con los controles p¨²blicos de Windows
#endif // _AFX_NO_AFXCMN_SUPPORT

#include <afxcontrolbars.h>     // Soporte MFC para cinta de opciones y barras de control


#include <afxsock.h>            // Extensiones de Socket de MFC


#pragma comment(linker,"/manifestdependency:\"type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")


