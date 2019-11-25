#include "serial.h"
#include "debug.h"
//////////////////////////////////////////////////////////////////////////
SerialCell::SerialCell(int port, DWORD ThreadId, DWORD BaudRate):
	Thread_Msg(ThreadId),
	CloseEvent(NULL),
	FirstReadBuffer(NULL),
	LastReadBuffer(NULL),
	FRxBytes(0),
	FReadOffset(0),
	LastError(0)
{
	DEBUG(TRACE, "Create Serial Thread\r\n");

	char buf[14];
	if (port > 9)	//http://support.microsoft.com/default.aspx?scid=kb;EN-US;q115831#appliesto
		wsprintf(buf, "\\\\.\\COM%u", port);	   
	else
		wsprintf(buf, "COM%u", port);

	::InitializeCriticalSection(&wr_rd);
	
	hPortHandle = CreateFile(buf,
		GENERIC_READ | GENERIC_WRITE,
		0, 
		NULL,
		OPEN_EXISTING, 
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
		NULL);
	if (hPortHandle == INVALID_HANDLE_VALUE)
	{
		LastError = GetLastError();
		return;
	}
	Sleep(60);

	PurgeComm(hPortHandle, PURGE_TXABORT | PURGE_RXABORT | PURGE_TXCLEAR | PURGE_RXCLEAR);

	Sleep(60);

	if (!GetCommState(hPortHandle, &dcbMasterInitState))
	{
		LastError = GetLastError();
		CloseHandle(hPortHandle);
		hPortHandle = INVALID_HANDLE_VALUE;
		return;
	}

	DCB dcb = dcbMasterInitState;
	dcb.BaudRate = BaudRate;
	dcb.fBinary = 1;    /*set binary mode*/
	dcb.ByteSize = 8;
	dcb.Parity = NOPARITY;
	dcb.StopBits = ONESTOPBIT;
	dcb.fDtrControl = 1;
	dcb.fRtsControl = 1;
	
	dcb.fOutxCtsFlow = 0;
	dcb.fOutxDsrFlow = 0;

	COMMTIMEOUTS timeout;
	memset(&timeout, 0, sizeof(COMMTIMEOUTS));

	if (!GetCommTimeouts(hPortHandle, &timeout))
	{
		LastError = GetLastError();
		CloseHandle(hPortHandle);
		hPortHandle = INVALID_HANDLE_VALUE;
		return;
	}

	timeout.ReadTotalTimeoutConstant = 20; //old 20
	SetCommTimeouts(hPortHandle, &timeout);

	if (!SetCommState(hPortHandle, &dcb))
	{
		LastError = GetLastError();
		CloseHandle(hPortHandle);
		hPortHandle = INVALID_HANDLE_VALUE;
		return;
	}
	// Start the watcher thread
	 hThread = ::CreateThread(0, 0, ThreadProc, LPVOID(this), 0, &curThrId);
}
//==============================================================
SerialCell::~SerialCell(void)
{
	
	if (CloseEvent) {
		DEBUG(TRACE, "Destroyed Serial Thread\r\n");
		SetEvent(CloseEvent);
		for (;;)
			if (!CloseEvent)
				break;
	}
	KillReadBuffers();
	if (hPortHandle != INVALID_HANDLE_VALUE) {
		//restore
		SetCommState(hPortHandle, &dcbMasterInitState);
		Sleep(60);
		CloseHandle(hPortHandle);
		hPortHandle = INVALID_HANDLE_VALUE;
	}
	DEBUG(TRACE, "Destroyed SerialCell\r\n");
}
//---------------------------------------------------------------------------
DWORD SerialCell::ReadData(PUCHAR Data, DWORD Len)
{
	DWORD BytesInBuffer;
	pBufferNode TempBuffer;
	DWORD BytesRemaining = Len;
	DWORD  BytesRead = 0;
	DWORD  DataOffset = 0;

	if (FirstReadBuffer == NULL)
		return 0;
	EnterCriticalSection(&wr_rd);

	do {
		//Определим длину данных
		BytesInBuffer = FirstReadBuffer->BufferSize - FReadOffset;
		//Если запрашиваемое значение меньше
		if (BytesRemaining < BytesInBuffer)
		{
			memcpy(&Data[DataOffset],
				&FirstReadBuffer->Buffer[FReadOffset],
				BytesRemaining);

			BytesRead += BytesRemaining;
			//уменьшим длину в буфере
			FReadOffset += BytesRemaining;
			FRxBytes -= BytesRemaining;
			BytesRemaining = 0;
			break;
		}
		else
		{
			memcpy(&Data[DataOffset],
				&FirstReadBuffer->Buffer[FReadOffset],
				BytesInBuffer);

			BytesRead += BytesInBuffer;
			//Уменьшим длину
			BytesRemaining -= BytesInBuffer;

			FRxBytes -= BytesInBuffer;
			DataOffset += BytesInBuffer;

			TempBuffer = FirstReadBuffer->Next;

			delete[] FirstReadBuffer->Buffer;
			delete FirstReadBuffer;
			FirstReadBuffer = TempBuffer;
			FReadOffset = 0;
		};
	} while ((BytesRemaining != 0) && (FirstReadBuffer != NULL));

	LeaveCriticalSection(&wr_rd);
	return BytesRead;
}
//---------------------------------------------------------------------------
void SerialCell::KillReadBuffers(void)
{
	pBufferNode TempBuffer;
	while (FirstReadBuffer)
	{
		delete[] FirstReadBuffer->Buffer;   //delete current buffer
		TempBuffer = FirstReadBuffer->Next;  //get  pointer next struct
		delete FirstReadBuffer;              //delete current struct
		FirstReadBuffer = TempBuffer;          // current struct =  next struct
	}
	FirstReadBuffer = NULL;
	LastReadBuffer = NULL;
	FRxBytes = 0;

	DeleteCriticalSection(&wr_rd);
}
//============================================================================
bool SerialCell::WriteData(BYTE* data, DWORD length, DWORD* dwWritten)
{
	bool success = false;
	OVERLAPPED o = { 0 };
	if (hPortHandle == INVALID_HANDLE_VALUE)
		return success;
	o.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	if (!WriteFile(hPortHandle, (LPCVOID)data, length, dwWritten, &o))
	{
		if (GetLastError() == ERROR_IO_PENDING)
			if (WaitForSingleObject(o.hEvent, INFINITE) == WAIT_OBJECT_0)
				if (GetOverlappedResult(hPortHandle, &o, dwWritten, FALSE))
					success = true;
	}
	else
		success = true;

	if (*dwWritten != length)
		success = false;
	CloseHandle(o.hEvent);

	return success;
}
//---------------------------------------------------------------------------
int SerialCell::OutError(DWORD err)
{
	LPVOID lpMsgBuf;
	int ret = IDCANCEL;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL,
		err,
		MAKELANGID(LANG_ENGLISH, // LANG_NEUTRAL,
		SUBLANG_DEFAULT),		 // Default language
		(LPTSTR)&lpMsgBuf,
		0,
		NULL
	);

	// Display the string.
	ret = MessageBox(NULL, (const char*)lpMsgBuf, "Error Serial port",
		MB_OK		|      //button
		MB_ICONSTOP |      //icon 
		MB_SYSTEMMODAL 
	);    
	// Free the buffer.
	LocalFree(lpMsgBuf);
	return ret;
}
//==================================================================================
DWORD WINAPI SerialCell::ThreadProc(LPVOID lpArg)
{
	// Route the method to the actual object
	SerialCell* pThis = reinterpret_cast<SerialCell*>(lpArg);
	return pThis->Execute();
}
////---------------------------------------------------------------------------
DWORD SerialCell::Execute()
{
	BYTE dataIn[64];
	//---- Place thread code here ----
	DWORD  WaitEvent;
	DWORD dwRead;
	bool success;
	HANDLE WaitHandles[2] = { 0 };
	OVERLAPPED     o = { 0 };
	o.hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
	CloseEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	WaitHandles[0] = CloseEvent;
	WaitHandles[1] = o.hEvent;

	for (;;)
	{
		success = false;

		if (!ReadFile(hPortHandle, dataIn, sizeof(dataIn), &dwRead, &o))
		{
			DWORD LastError = GetLastError();
			if (LastError == ERROR_IO_PENDING)
				WaitEvent = WaitForMultipleObjects(2,WaitHandles, FALSE, INFINITE);
			else {
				OutError(LastError);
				//continue;
				break;
			}
					   		
			if (WaitEvent == WAIT_OBJECT_0) 
								break;  //Exit
			
			else {
				GetOverlappedResult(hPortHandle, &o, &dwRead, FALSE);
				if (dwRead) success = true;
			}
		}
		else     
				if (dwRead) success = true;


		if (success)
		{
			EnterCriticalSection(&wr_rd);


			if (!FirstReadBuffer) {
				FirstReadBuffer = new TBufferNode;
				LastReadBuffer = FirstReadBuffer;
			}
			else {
				LastReadBuffer->Next = new TBufferNode;
				LastReadBuffer = LastReadBuffer->Next;
			}

			LastReadBuffer->Next = NULL;
			LastReadBuffer->Buffer = new UCHAR[dwRead];
			memcpy(LastReadBuffer->Buffer, dataIn, dwRead);
			LastReadBuffer->BufferSize = dwRead;

			FRxBytes += dwRead;

			LeaveCriticalSection(&wr_rd);

			//PostMessage((HWND__*)WinHWND, RX_SERIAL_EVENT, WPARAM(dwRead), LPARAM(FRxBytes));
			PostThreadMessage(Thread_Msg, RX_SERIAL_EVENT, WPARAM(dwRead), LPARAM(FRxBytes));
		}

	}//for(;;)

	CloseHandle(o.hEvent);
	CloseHandle(CloseEvent);
	CloseEvent = NULL;
	return 0;
}