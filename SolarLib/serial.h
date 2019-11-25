#pragma once

#ifndef __SERIALH__
#define __SERIALH__ 

#include <windows.h>
#include <stdio.h>
#include "DataSolar.h"



//---------------------------------------------------------------------------
struct _TBufferNode;

typedef struct _TBufferNode {
	_TBufferNode *Next;
	DWORD  BufferSize;
	PUCHAR Buffer;
}TBufferNode, *pBufferNode;

class SerialCell
{
private:
protected:

	CRITICAL_SECTION wr_rd;
	pBufferNode FirstReadBuffer;
	pBufferNode LastReadBuffer;
	DWORD FRxBytes;
	DWORD FReadOffset;
	DWORD Execute();
	int    commPortNumber;
	HANDLE hThread;
	DWORD curThrId;
	DWORD Thread_Msg;
	HANDLE CloseEvent;
	DCB dcbMasterInitState;
	DWORD LastError;

	void  KillReadBuffers(void);
	static DWORD WINAPI ThreadProc(LPVOID lpArg);

public:
	HANDLE hPortHandle;         // Handle to COM port
	SerialCell(int port,DWORD ThreadId,	DWORD BaudRate = CBR_115200);
	~SerialCell(void);
	DWORD  ReadData(PUCHAR Data, DWORD Len);
	bool WriteData(BYTE* data, DWORD length, DWORD* dwWritten);
	int OutError(DWORD err);
	DWORD  GetError() { return LastError; }
};
#endif