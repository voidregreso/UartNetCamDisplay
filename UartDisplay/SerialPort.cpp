// SERIAL PORT COMMUNCATION UTILITY CLASS

#include "stdafx.h"
#include "SerialPort.h"
#include <assert.h>

//
// Constructor
//
CSerialPort::CSerialPort()
{
	m_hComm = NULL;
	m_Thread = NULL;
	// initialize overlapped structure members to zero
	m_ov.Offset = 0;
	m_ov.OffsetHigh = 0;

	// create events
	m_ov.hEvent = NULL;
	m_hWriteEvent = NULL;
	m_hShutdownEvent = NULL;

	m_hInput = NULL;
	m_hOutput = NULL;

	m_szWriteBuffer = NULL;

	m_bThreadAlive = FALSE;
}

//
// Delete dynamic memory
//
CSerialPort::~CSerialPort()
{
	MSG message;
	while (m_bThreadAlive)
	{
		if (::PeekMessage(&message, m_pOwner->m_hWnd, 0, 0, PM_REMOVE))
		{
			::TranslateMessage(&message);
			::DispatchMessage(&message);
		}
		SetEvent(m_hShutdownEvent);
		Sleep(50);
	}

	if (m_hComm != NULL)
	{
		CloseHandle(m_hComm);
		m_hComm = NULL;
	}
	if (m_hShutdownEvent != NULL)
		CloseHandle(m_hShutdownEvent);
	if (m_ov.hEvent != NULL)
		CloseHandle(m_ov.hEvent);
	if (m_hWriteEvent != NULL)
		CloseHandle(m_hWriteEvent);
	TRACE("Thread ended\n");
	if (m_szWriteBuffer != NULL)
		delete[] m_szWriteBuffer;
	if (m_szReceiveBuffer != NULL)
		delete[] m_szReceiveBuffer;
}

//
// Initialize the port. This can be port 1 to 4.
//
BOOL CSerialPort::InitPort(CWnd *pPortOwner,   // the owner (CWnd) of the port (receives message)
						   HANDLE hInput,	   // FIFO pipe handle for data input(serial receive data)
						   HANDLE hOutput,	   // FIFO pipe handle for data ouput(serial send data)
						   UINT portnr,		   // portnumber (1,2,3...255)
						   UINT baud,		   // baudrate
						   char parity,		   // parity
						   UINT databits,	   // databits
						   UINT stopbits,	   // stopbits
						   DWORD dwCommEvents, // EV_RXCHAR, EV_CTS etc
						   UINT rxInterval)	   // unit: ms
{
	assert(pPortOwner != NULL);

	// if the thread is alive: Kill
	while (m_bThreadAlive)
	{
		SetEvent(m_hShutdownEvent);
		Sleep(100);
	}

	// create events
	if (m_ov.hEvent != NULL)
		ResetEvent(m_ov.hEvent);
	else
		m_ov.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (m_hWriteEvent != NULL)
		ResetEvent(m_hWriteEvent);
	else
		m_hWriteEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	if (m_hShutdownEvent != NULL)
		ResetEvent(m_hShutdownEvent);
	else
		m_hShutdownEvent = CreateEvent(NULL, TRUE, FALSE, NULL);

	// initialize the event objects
	m_hEventArray[0] = m_hShutdownEvent; // highest priority
	m_hEventArray[1] = m_ov.hEvent;
	m_hEventArray[2] = m_hWriteEvent;

	// initialize critical section
	InitializeCriticalSection(&m_csCommunicationSync);

	// set buffersize for writing and save the owner
	m_pOwner = pPortOwner;

	if (m_szWriteBuffer != NULL)
		delete[] m_szWriteBuffer;
	m_szWriteBuffer = new unsigned char[1024];

	if (m_szReceiveBuffer != NULL)
		delete[] m_szReceiveBuffer;
	m_szReceiveBuffer = new unsigned char[2048];

	m_hInput = hInput;
	m_hOutput = hOutput;

	m_nPortNr = portnr;

	m_nWriteBufferSize = 1024;
	m_nReceiveBufferSize = 2048;
	m_dwCommEvents = dwCommEvents;

	m_nRxInterval = rxInterval;

	BOOL bResult = FALSE;
	char *szPort = new char[50];
	char *szBaud = new char[50];

	// now it critical!
	EnterCriticalSection(&m_csCommunicationSync);

	// if the port is already opened: close it
	if (m_hComm != NULL)
	{
		CloseHandle(m_hComm);
		m_hComm = NULL;
	}

	// prepare port strings
	sprintf_s(szPort, 50, "COM%d", portnr);
	sprintf_s(szBaud, 50, "baud=%d parity=%c data=%d stop=%d", baud, parity, databits, stopbits);

	// get a handle to the port
	m_hComm = CreateFile(szPort,					   // communication port string (COMX)
						 GENERIC_READ | GENERIC_WRITE, // read/write types
						 0,							   // comm devices must be opened with exclusive access
						 NULL,						   // no security attributes
						 OPEN_EXISTING,				   // comm devices must use OPEN_EXISTING
						 FILE_FLAG_OVERLAPPED,		   // Async I/O
						 0);						   // template must be 0 for comm devices

	if (m_hComm == INVALID_HANDLE_VALUE)
	{
		// port not found
		delete[] szPort;
		delete[] szBaud;

		return FALSE;
	}

	// set the timeout values
	m_CommTimeouts.ReadIntervalTimeout = 1000;
	m_CommTimeouts.ReadTotalTimeoutMultiplier = 1000;
	m_CommTimeouts.ReadTotalTimeoutConstant = 1000;
	m_CommTimeouts.WriteTotalTimeoutMultiplier = 1000;
	m_CommTimeouts.WriteTotalTimeoutConstant = 1000;

	// configure
	if (SetCommTimeouts(m_hComm, &m_CommTimeouts))
	{
		if (SetCommMask(m_hComm, dwCommEvents))
		{
			if (GetCommState(m_hComm, &m_dcb))
			{
				// m_dcb.fRtsControl = RTS_CONTROL_ENABLE;		// set RTS bit high!
				m_dcb.fRtsControl = RTS_CONTROL_DISABLE;
				m_dcb.fDtrControl = DTR_CONTROL_DISABLE;
				if (BuildCommDCB(szBaud, &m_dcb))
				{
					if (SetCommState(m_hComm, &m_dcb))
						bResult = TRUE; // normal operation... continue
					else
						ProcessErrorMessage("SetCommState()");
				}
				else
					ProcessErrorMessage("BuildCommDCB()");
			}
			else
				ProcessErrorMessage("GetCommState()");
		}
		else
			ProcessErrorMessage("SetCommMask()");
	}
	else
		ProcessErrorMessage("SetCommTimeouts()");

	delete[] szPort;
	delete[] szBaud;

	// flush the port
	PurgeComm(m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

	// release critical section
	LeaveCriticalSection(&m_csCommunicationSync);

	TRACE("Initialisation for communicationport %d completed.\nUse Startmonitor to communicate.\n", portnr);

	return bResult;
}

//
//  The CommThread Function.
//
UINT CSerialPort::CommThread(LPVOID pParam)
{
	// Cast the void pointer passed to the thread back to
	// a pointer of CSerialPort class
	CSerialPort *port = (CSerialPort *)pParam;

	// Set the status variable in the dialog class to
	// TRUE to indicate the thread is running.
	port->m_bThreadAlive = TRUE;

	// Misc. variables
	DWORD BytesTransfered = 0;
	DWORD Event = 0;
	DWORD CommEvent = 0;
	DWORD dwError = 0;
	COMSTAT comstat;
	BOOL bResult = TRUE;

	// Clear comm buffers at startup
	if (port->m_hComm) // check if the port is opened
		PurgeComm(port->m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

	// begin forever loop.  This loop will run as long as the thread is alive.
	for (;;)
	{
		bResult = WaitCommEvent(port->m_hComm, &Event, &port->m_ov);

		if (!bResult)
		{
			// If WaitCommEvent() returns FALSE, process the last error to determin
			// the reason..
			switch (dwError = GetLastError())
			{
			case ERROR_IO_PENDING:
			{
				// This is a normal return value if there are no bytes
				// to read at the port.
				// Do nothing and continue
				break;
			}
			case 87:
			{
				// Under Windows NT, this value is returned for some reason.
				// I have not investigated why, but it is also a valid reply
				// Also do nothing and continue.
				break;
			}
			default:
			{
				// All other error codes indicate a serious error has
				// occured.  Process this error.
				port->ProcessErrorMessage("WaitCommEvent()");
				break;
			}
			}
		}
		else
		{
			bResult = ClearCommError(port->m_hComm, &dwError, &comstat);

			if (comstat.cbInQue == 0)
				continue;
		} // end if bResult

		// Main wait function.  This function will normally block the thread
		// until one of nine events occur that require action.
		Event = WaitForMultipleObjects(3, port->m_hEventArray, FALSE, INFINITE);
		switch (Event)
		{
		case 0:
			port->m_bThreadAlive = FALSE;

			// Kill this thread.  break is not needed, but makes me feel better.
			AfxEndThread(100);
			break;
		case 1: // read event
			GetCommMask(port->m_hComm, &CommEvent);
			if (CommEvent & EV_CTS)
				::SendMessage(port->m_pOwner->m_hWnd, WM_COMM_CTS_DETECTED, (WPARAM)0, (LPARAM)port->m_nPortNr);
			if (CommEvent & EV_RXFLAG)
				::SendMessage(port->m_pOwner->m_hWnd, WM_COMM_RXFLAG_DETECTED, (WPARAM)0, (LPARAM)port->m_nPortNr);
			if (CommEvent & EV_BREAK)
				::SendMessage(port->m_pOwner->m_hWnd, WM_COMM_BREAK_DETECTED, (WPARAM)0, (LPARAM)port->m_nPortNr);
			if (CommEvent & EV_ERR)
				::SendMessage(port->m_pOwner->m_hWnd, WM_COMM_ERR_DETECTED, (WPARAM)0, (LPARAM)port->m_nPortNr);
			if (CommEvent & EV_RING)
				::SendMessage(port->m_pOwner->m_hWnd, WM_COMM_RING_DETECTED, (WPARAM)0, (LPARAM)port->m_nPortNr);

			if (CommEvent & EV_RXCHAR)
			{
				// Receive character event from port.
				ReceiveChar(port);
			}
			break;
		case 2: // write event
		{
			// Write character event from port
			WriteChar(port);
			break;
		}

		} // end switch
		ResetEvent(port->m_hEventArray[Event]);
	} // close forever loop

	return 0;
}

//
// start comm watching
//
BOOL CSerialPort::StartMonitoring()
{
	if (!(m_Thread = AfxBeginThread(CommThread, this)))
		return FALSE;
	TRACE("Thread started\n");
	return TRUE;
}

//
// Restart the comm thread
//
BOOL CSerialPort::RestartMonitoring()
{
	TRACE("Thread resumed\n");
	m_Thread->ResumeThread();
	return TRUE;
}

//
// Suspend the comm thread
//
BOOL CSerialPort::SuspendMonitoring()
{
	TRACE("Thread suspended\n");
	m_Thread->SuspendThread();
	return TRUE;
}

//
// If there is a error, give the right message
//
void CSerialPort::ProcessErrorMessage(char *ErrorText)
{
	char *Temp = new char[200];

	LPVOID lpMsgBuf;

	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		GetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), // Default language
		(LPTSTR)&lpMsgBuf,
		0,
		NULL);

	sprintf_s(Temp, 200, "WARNING:  %s Failed with the following error: \n%s\nPort: %d\n", (char *)ErrorText, lpMsgBuf, m_nPortNr);
	MessageBox(NULL, Temp, "Application Error", MB_ICONSTOP);

	LocalFree(lpMsgBuf);
	delete[] Temp;
}

//
// Write a character.
//
void CSerialPort::WriteChar(CSerialPort *port)
{
	BOOL bWrite = TRUE;
	BOOL bResult = TRUE;

	DWORD BytesSent = 0;

	ResetEvent(port->m_hWriteEvent);

	// Gain ownership of the critical section
	EnterCriticalSection(&port->m_csCommunicationSync);

	if (bWrite)
	{
		// Initailize variables
		port->m_ov.Offset = 0;
		port->m_ov.OffsetHigh = 0;

		// Clear buffer
		PurgeComm(port->m_hComm, PURGE_RXCLEAR | PURGE_TXCLEAR | PURGE_RXABORT | PURGE_TXABORT);

		bResult = WriteFile(port->m_hComm,						   // Handle to COMM Port
							port->m_szWriteBuffer,				   // Pointer to message buffer in calling finction
							strlen((char *)port->m_szWriteBuffer), // Length of message to send
							&BytesSent,							   // Where to store the number of bytes sent
							&port->m_ov);						   // Overlapped structure

		// deal with any error codes
		if (!bResult)
		{
			DWORD dwError = GetLastError();
			switch (dwError)
			{
			case ERROR_IO_PENDING:
			{
				// continue to GetOverlappedResults()
				BytesSent = 0;
				bWrite = FALSE;
				break;
			}
			default:
			{
				// all other error codes
				port->ProcessErrorMessage("WriteFile()");
			}
			}
		}
		else
		{
			LeaveCriticalSection(&port->m_csCommunicationSync);
		}
	} // end if(bWrite)

	if (!bWrite)
	{
		bWrite = TRUE;

		bResult = GetOverlappedResult(port->m_hComm, // Handle to COMM port
									  &port->m_ov,	 // Overlapped structure
									  &BytesSent,	 // Stores number of bytes sent
									  TRUE);		 // Wait flag

		LeaveCriticalSection(&port->m_csCommunicationSync);

		// deal with the error code
		if (!bResult)
		{
			port->ProcessErrorMessage("GetOverlappedResults() in WriteFile()");
		}
	} // end if (!bWrite)

	// Verify that the data size send equals what we tried to send
	if (BytesSent != strlen((char *)port->m_szWriteBuffer))
	{
		TRACE("WARNING: WriteFile() error.. Bytes Sent: %d; Message Length: %d\n", BytesSent, strlen((char *)port->m_szWriteBuffer));
	}
}

//
// Character received. Inform the owner
//
void CSerialPort::ReceiveChar(CSerialPort *port)
{
	BOOL bRead = TRUE;
	BOOL bResult = TRUE;
	DWORD dwError = 0;
	DWORD WantRead = 0;
	DWORD BytesRead = 0;
	DWORD PipeWritten;
	COMSTAT comstat;
	unsigned char *pbuf = NULL;

	// Gain ownership of the comm port critical section.
	// This process guarantees no other part of this program
	// is using the port object.

	EnterCriticalSection(&port->m_csCommunicationSync);

	// ClearCommError() will update the COMSTAT structure and
	// clear any other errors.

	bResult = ClearCommError(port->m_hComm, &dwError, &comstat);

	LeaveCriticalSection(&port->m_csCommunicationSync);

	// start forever loop.  I use this type of loop because I
	// do not know at runtime how many loops this will have to
	// run. My solution is to start a forever loop and to
	// break out of it when I have processed all of the
	// data available.  Be careful with this approach and
	// be sure your loop will exit.
	// My reasons for this are not as clear in this sample
	// as it is in my production code, but I have found this
	// solutiion to be the most efficient way to do this.

	if (comstat.cbInQue == 0)
	{
		// break out when all bytes have been read
		return;
	}

	EnterCriticalSection(&port->m_csCommunicationSync);

	if (bRead)
	{
		if (comstat.cbInQue > port->m_nReceiveBufferSize)
		{
			pbuf = new unsigned char[comstat.cbInQue];
			if (pbuf == NULL)
			{
				WantRead = port->m_nReceiveBufferSize;
				pbuf = port->m_szReceiveBuffer;
			}
			else
			{
				WantRead = comstat.cbInQue;
			}
		}
		else
		{
			WantRead = comstat.cbInQue;
			pbuf = port->m_szReceiveBuffer;
		}
		// PipeWritten = comstat.cbInQue > port->m_nReceiveBufferSize ? port->m_nReceiveBufferSize : comstat.cbInQue;

		bResult = ReadFile(port->m_hComm, // Handle to COMM port
						   pbuf,		  // RX Buffer Pointer
						   WantRead,	  // Read bytes
						   &BytesRead,	  // Stores number of bytes read
						   &port->m_ov);  // pointer to the m_ov structure
		// deal with the error code
		if (!bResult)
		{
			switch (dwError = GetLastError())
			{
			case ERROR_IO_PENDING:
			{
				// asynchronous i/o is still in progress
				// Proceed on to GetOverlappedResults();
				bRead = FALSE;
				break;
			}
			default:
			{
				// Another error has occured.  Process this error.
				port->ProcessErrorMessage("ReadFile()");
				break;
			}
			}
		}
		else
		{
			if (WantRead != BytesRead)
			{
				TRACE("UART %d data droped!!!\r\n", WantRead - BytesRead);
			}
			// ReadFile() returned complete. It is not necessary to call GetOverlappedResults()
			bRead = TRUE;
		}
	} // close if (bRead)

	if (!bRead)
	{
		bRead = TRUE;
		bResult = GetOverlappedResult(port->m_hComm, // Handle to COMM port
									  &port->m_ov,	 // Overlapped structure
									  &BytesRead,	 // Stores number of bytes read
									  TRUE);		 // Wait flag

		// deal with the error code
		if (!bResult)
		{
			port->ProcessErrorMessage("GetOverlappedResults() in ReadFile()");
		}
	} // close if (!bRead)

	LeaveCriticalSection(&port->m_csCommunicationSync);

	if (bRead)
	{
		if (port->m_hInput != NULL)
		{
			PipeWritten = 0;
			WriteFile(port->m_hInput, pbuf, BytesRead, &PipeWritten, NULL);
			if (BytesRead > PipeWritten)
			{
				AfxMessageBox(_T("Pipe data flow out!"));
			}
			// else
			//::SendMessage((port->m_pOwner)->m_hWnd, WM_COMM_RXCHAR, (WPARAM)PipeWritten, (LPARAM)port->m_nPortNr);
			//::PostMessage((port->m_pOwner)->m_hWnd, WM_COMM_RXCHAR, (WPARAM)PipeWritten, (LPARAM)port->m_nPortNr);
		}
	}
	if (pbuf != port->m_szReceiveBuffer && pbuf != NULL)
	{
		delete pbuf;
	}

	// notify parent that a byte was received
	//::SendMessage((port->m_pOwner)->m_hWnd, WM_COMM_RXCHAR, (WPARAM) RXBuff, (LPARAM) port->m_nPortNr);
}

//
// Write a string to the port
//
void CSerialPort::WriteToPort(char *string)
{
	assert(m_hComm != 0);

	memset(m_szWriteBuffer, 0, sizeof(m_szWriteBuffer));
	// The actual size is determined by the parameters passed in during the initialization of the serial port
	strcpy_s((char *)m_szWriteBuffer, 1000, string);

	// set event for write
	SetEvent(m_hWriteEvent);
}

//
// Return the device control block
//
DCB CSerialPort::GetDCB()
{
	return m_dcb;
}

//
// Return the communication event masks
//
DWORD CSerialPort::GetCommEvents()
{
	return m_dwCommEvents;
}

//
// Return the output buffer size
//
DWORD CSerialPort::GetWriteBufferSize()
{
	return m_nWriteBufferSize;
}
