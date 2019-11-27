///////////////////////////////////////////////////////////////////////////
#include <windows.h>
#include <shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
///////////////////////////////////////////////////////////////////////////
#include "SolarLib.h"
#include "serial.h"
#include "debug.h"
/////////////////////////////////////////////////////////////////////////
class espfifo
{
private:
	char  *fifo, *Top, *Que;
	int   numel, len;

public:
	espfifo(int num) {
		numel = num;
		len = sizeof(flx_data);

		fifo = (char *)VirtualAlloc(NULL, numel * len, MEM_COMMIT,
			PAGE_READWRITE);
		VirtualLock(fifo, numel * len);
		Top = Que = fifo;
	}

	~espfifo(void) {
		VirtualUnlock(fifo, numel * len);   //  * sizeof(byte) omitted...
		VirtualFree(fifo, 0, MEM_RELEASE);
	}

	bool getp(void* &el) {
		el = Que;
		Que += len;
		if (Que >= fifo + numel * len) Que = fifo;

		return true;    // error return to be done (overrun and underrun)
	}

	bool put(void* el) {
		memcpy(Top, (char *)el, len);
		Top += len;
		if (Top >= fifo + numel * len) Top = fifo;
		return true;
	}

	int  getnum(void) {
		int tmp = (Top - Que) / (len);
		if (tmp < 0)
			tmp += numel;
		return tmp;  // return number of elements, not length
	}
};
/////////////////////////////////////////////////////////////////////////
class Task {

public:
	
	HANDLE	   hMutex;
	DWORD	   error;
	HANDLE     m_hThread;
	HANDLE	   hwnd;
	DWORD	   m_idThread;
	DWORD      ComPort;
	SerialCell *pSerialCell;
	espfifo*   pEspFifo; 
	CRITICAL_SECTION CrSec = {0};
	float current_esp_lev;
	uint32_t  outsms;
	uint16_t treshold;
	CHAR Ini_File[MAX_PATH];

	void WriteInteger(char* szSection, char* szKey, int iValue)
	{
		char szValue[255];
		sprintf(szValue, "%d", iValue);
		WritePrivateProfileString(szSection, szKey, szValue, Ini_File);
	}
	void WriteFloat(char* szSection, char* szKey, float fltValue)
	{
		char szValue[255];
		sprintf(szValue, "%f", fltValue);
		WritePrivateProfileString(szSection, szKey, szValue, Ini_File);
	}
	int ReadInteger(char* szSection, char* szKey, int iDefaultValue)
	{
		int iResult = GetPrivateProfileInt(szSection, szKey, iDefaultValue, Ini_File);
		return iResult;
	}
	float ReadFloat(char* szSection, char* szKey, float fltDefaultValue)
	{
		char szResult[255];
		char szDefault[255];
		float fltResult;
		sprintf(szDefault, "%f", fltDefaultValue);
		GetPrivateProfileString(szSection, szKey, szDefault, szResult, 255, Ini_File);
		fltResult = (float)atof(szResult);
		return fltResult;
	}

	Task() :
		m_hThread(INVALID_HANDLE_VALUE),
		hwnd(INVALID_HANDLE_VALUE),
		m_idThread(0),
		ComPort(0),
		current_esp_lev(0.f),
		outsms(0),
		treshold(TRESHOLD),
		pSerialCell(NULL)
		//----------------------------------------------
		{
		::InitializeCriticalSection(&CrSec);

		hMutex = CreateMutex(
			NULL,                        // default security descriptor
			FALSE,                       // mutex not owned
			TEXT("SolarLibObject"));  // object name

		    if((error = ::GetLastError())== ERROR_ALREADY_EXISTS)
				return ;
			
			 ::GetModuleFileName(NULL, Ini_File, MAX_PATH);
			::PathRemoveFileSpec(Ini_File);
			StrCat(Ini_File, "\\solar.ini");

			if (!::PathFileExists(Ini_File)) {
				HANDLE hIni = CreateFile(Ini_File,
					GENERIC_WRITE,			// dwDesiredAccess
					0,					    // dwShareMode
					NULL,					// lpSecurityAttributes
					CREATE_NEW,				// dwCreationDisposition
					FILE_ATTRIBUTE_NORMAL,  // dwFlagsAndAttributes
					NULL);
               
					WriteInteger("Setting", "Treshold", treshold);
					WriteInteger("Setting", "OutSMS", outsms);
					WriteInteger("Setting", "CurValue", (int)current_esp_lev);
					::CloseHandle(hIni);
			}
			treshold = ReadInteger("Setting", "Treshold", treshold);
			outsms = ReadInteger("Setting", "OutSMS", outsms);
			current_esp_lev = ReadInteger("Setting", "CurValue", (int)current_esp_lev);
			
	    }

	~Task() {
		    ::DeleteCriticalSection(&CrSec);
			if (error != ERROR_ALREADY_EXISTS) {
				WriteInteger("Setting", "Treshold", treshold);
				WriteInteger("Setting", "OutSMS", outsms);
				WriteInteger("Setting", "CurValue", (int)current_esp_lev);
			}
			CloseHandle(hMutex);
			DEBUG(TRACE, "Treshold:%5i CurValue:%5i OutSMS:%5i\r\n", treshold, (int)current_esp_lev,outsms);
	}
};


Task mTask[MAX_MODEM];
uint32_t debug_level;

/////////////////////////////////////////////////////////////////////////
DWORD GetTime(){
	SYSTEMTIME sm;
	GetLocalTime(&sm);
	DWORD time = sm.wMilliseconds;
	time += sm.wSecond * 1000;
	time += sm.wMinute * 100000;
	time += sm.wHour   * 10000000;
	return time;
}
/////////////////////////////////////////////////////////////////////////
DWORD threadProc(LPVOID lParam)
{
	MSG msg;
	CVSD txCVSD; 
	CVSD rxCVSD;
	
	flx_data FLX_data;
	Filter mFilter(5);
	Task *pTask = (Task *)lParam;
	Input_Data Tx_Data[2];
	size_t n_samples = 0;
	uint16_t in_pnt = 0;
	size_t nData = 0;
	float samples[8];


	Input_Data inRx;
	uint16_t pParser = 0;
	uint8_t  *pinRx = (uint8_t *)&inRx;
	DWORD dwWritten;
	size_t get_status = 0;

	//Debug
	DEBUG(TRACE, "Create main Thread Com %i  \r\n", pTask->ComPort);
	//Синхронизация потоков
	while (pTask->m_hThread == INVALID_HANDLE_VALUE)
	{
		Sleep(0);
	}
	pTask->pEspFifo = new espfifo(4);
	pTask->pSerialCell = new SerialCell(pTask->ComPort, pTask->m_idThread);

	while (GetMessage(&msg, 0, 0, 0))
	{
		switch (msg.message)
		{
		case RX_SERIAL_EVENT:
			DWORD Len;
			UCHAR Data[64];

			if(pTask->pSerialCell)
				while ((Len = pTask->pSerialCell->ReadData(Data, sizeof(Data))) > 0)
				{
					int inpnt = 0;
					do {

						uint8_t inch = Data[inpnt++];
						Len--;

						if ((pParser < 1 ) && (inch == 1))
						{
							pinRx[pParser++] = inch;
						}
						else
						if (pParser < 2) {
								pinRx[pParser++] = inch;
								if (inch != 'R')
									pParser = 0;
						}else
						if (pParser < 10) { //Wait Len
							pinRx[pParser++] = inch;
						}
						else { //load data
								pinRx[pParser++] = inch;
									if (pParser >= inRx.Len_Data + 20)
									{
										pParser = 0;   //Stop,Wait Next

										switch (inRx.Mode)
										{
											case ESP_DATA:
												DEBUG(TRACE, "RX ESP_DATA %i\r\n",GetTime());
												{
													float *pData = FLX_data.flx;
													//Время
													FLX_data.timeU = inRx.Time;
													FLX_data.treshold = inRx.Treshold;
													FLX_data.max_value = inRx.MaxValue;
													//FLX_data.cnt_sms   = pCMD->outsms;

													//decode
													rxCVSD.set_ref(inRx.Ref_CVSD);
													rxCVSD.set_bitref(inRx.bitref_CVSD);
													for (int i = 0; i < inRx.Len_Data; i++) {
														rxCVSD.cvsd_decode8(inRx.bData[i], pData);
														pData += 8;
													}

													if (pTask->hwnd) {
														pTask->pEspFifo->put((void *)&FLX_data);
														PostMessage((HWND__*)pTask->hwnd, ESP_EVENT,
															WPARAM(pTask->pEspFifo->getnum()),
															LPARAM(inRx.Mode));
													}
													else { //write to File
														FILE *stream;
														
														char name[256];
														sprintf(name, "%i.esp", GetTime());
														if (!fopen_s(&stream, name, "w")) {
															fwrite(
																&FLX_data,
																sizeof(char),
																sizeof(FLX_data),
																stream);
															fclose(stream);
														}
														
													}
												}
											break;

											case SET_PARAM:
												pTask->treshold = inRx.Treshold;
												DEBUG(TRACE, "RX SET_PARAM  %i:%i\r\n", inRx.Treshold, inRx.Time);
											//break;

											case GET_STATUS:
												get_status++;
											break;

											case RET_STATUS:
												float *pData = FLX_data.flx;
												//Время
												FLX_data.timeU = inRx.Time;
												FLX_data.treshold = inRx.Treshold;
												FLX_data.max_value = inRx.MaxValue;
												//decode
												rxCVSD.set_ref(inRx.Ref_CVSD);
												rxCVSD.set_bitref(inRx.bitref_CVSD);

												// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
												for (int i = 0; i < inRx.Len_Data-2; i++) {
													rxCVSD.cvsd_decode8(inRx.bData[i], pData);
													pData += 8;
												}
												FLX_data.cnt_sms = inRx.bData[118] + inRx.bData[119] * 256;

												if (pTask->hwnd)
													PostMessage((HWND__*)pTask->hwnd, ESP_EVENT,
														WPARAM(&FLX_data),
														LPARAM(RET_STATUS));

												DEBUG(TRACE, "RX RET_STATUS  %i:%i\r\n", inRx.MaxValue, inRx.Time);
											break;

										}

									}
								}
					} while (Len);
				}
		break;

		case SET_DATA:
			samples[n_samples++] = (msg.lParam / 100.f) / MAX_ESP;
			pTask->current_esp_lev = mFilter.run_filter(msg.lParam / 100.f);
			if (!(n_samples % 8)) {

				n_samples = 0;
				//test new data
				if (!in_pnt) {
					Tx_Data[nData].Start_byte = 1;
					Tx_Data[nData].Name[0] = 'R';
					Tx_Data[nData].Name[1] = 'K';
					
					Tx_Data[nData].Mode = ESP_DATA;

					Tx_Data[nData].Time = msg.wParam;
					Tx_Data[nData].MaxValue = 0;
					Tx_Data[nData].Treshold = (uint16_t)pTask->treshold;
				//Текушее состояние CVSD
					Tx_Data[nData].Ref_CVSD = txCVSD.get_ref();
					Tx_Data[nData].bitref_CVSD = txCVSD.get_bitref();
					Tx_Data[nData].CS = 0;
				}
				Tx_Data[nData].bData[in_pnt++] = txCVSD.cvsd_encode8(samples);

				//Search MAX
				if (pTask->current_esp_lev > Tx_Data[nData].MaxValue)
					Tx_Data[nData].MaxValue = (uint16_t)pTask->current_esp_lev;

				if (in_pnt >= MAX_LEN_DATA) {
					Tx_Data[nData].Len_Data = in_pnt;

					if (Tx_Data[nData].MaxValue >= pTask->treshold) {
						if (pTask->pSerialCell) {
							EnterCriticalSection(&pTask->CrSec);
							pTask->pSerialCell->WriteData((BYTE*)&Tx_Data[nData], in_pnt + 20, &dwWritten);
							pTask->outsms++;
							LeaveCriticalSection(&pTask->CrSec);
							DEBUG(TRACE, "TX ESP_DATA %i\r\n", GetTime());
						}
					}else 
					if(get_status){
						get_status = 0;
						Tx_Data[nData].Mode = RET_STATUS;
						Tx_Data[nData].Len_Data = in_pnt;

						if (pTask->pSerialCell) {
							EnterCriticalSection(&pTask->CrSec);
							pTask->pSerialCell->WriteData((BYTE*)&Tx_Data[nData], in_pnt + 20, &dwWritten);
							pTask->outsms++;
							Tx_Data[nData].bData[118] = pTask->outsms;
							Tx_Data[nData].bData[119] = pTask->outsms>>8;

							LeaveCriticalSection(&pTask->CrSec);
							DEBUG(TRACE, "TX RET_STATUS  %i:%i\r\n", Tx_Data[nData].MaxValue, Tx_Data[nData].Time);
						}
					}

					in_pnt = 0;
					//Change bufferS  
					nData = (nData + 1) & 1;
				}
			}
			break;

// case WM_APP + 1: MessageBoxA(NULL, "Hello", "From Thread", MB_OK); break;

		default:
			DispatchMessage(&msg);
		}
	}

	/*
		pCMD->Start_byte = 1;
		pCMD->Name[0]   = 'R';
		pCMD->Name[1] = 'K';
		pCMD->Mode = RET_STATUS;
		pCMD->Time = GetTime();
		pCMD->Len_Data = 0;
		pCMD->Treshold = (uint16_t)treshold;
		pCMD->Curr_Value = (uint16_t)pTask->current_esp_lev;
		pCMD->Max_Value = (uint16_t)MAX_ESP;
		pCMD->Max_Len = MAX_LEN_DATA;
		pCMD->Flags = STOP_TASK;
		EnterCriticalSection(&pTask->CrSec);
		DWORD dwWritten;
		pTask->pSerialCell->WriteData(pinRx, sizeof(Cmd_Data), &dwWritten);
		LeaveCriticalSection(&pTask->CrSec);
*/


	if (pTask->pSerialCell)
		delete pTask->pSerialCell;
	pTask->pSerialCell = NULL;
	delete pTask->pEspFifo;
	DEBUG(TRACE, "Destroyed main Thread\r\n");
	pTask->m_hThread = INVALID_HANDLE_VALUE;
	return 0;
}
/////////////////////////////////////////////////////////////////////////
BOOL	 VCPP_API   GetStatusModem(HANDLE handle) {

	Input_Data mCmd;
	if (handle == INVALID_HANDLE_VALUE)
		return FALSE;
	Task *pTask = (Task*)handle;
	memset((void *)&mCmd, 0, sizeof(Input_Data));
	mCmd.Start_byte = 1;
	mCmd.Name[0] = 'R';
	mCmd.Name[1] = 'K';
	mCmd.Mode = GET_STATUS;
	mCmd.Len_Data = MAX_LEN_DATA;
	if (pTask->pSerialCell) {
		EnterCriticalSection(&pTask->CrSec);
		DWORD dwWritten;
		pTask->pSerialCell->WriteData((uint8_t *)&mCmd, sizeof(Input_Data), &dwWritten);
		pTask->outsms++;
		LeaveCriticalSection(&pTask->CrSec);
		DEBUG(TRACE, "CMD GetStatusModem :%ui\r\n", GetTime());
		return TRUE;
	}
	return FALSE;
}
/////////////////////////////////////////////////////////////////////////
BOOL	 VCPP_API   SetTresHold(HANDLE handle, float treshold)
{
 
 if (handle == INVALID_HANDLE_VALUE)
		    return FALSE;

    Input_Data mCmd;
	Task *pTask = (Task*)handle;
	memset((void *)&mCmd, 0, sizeof(Input_Data));
	mCmd.Start_byte = 1;
	mCmd.Name[0] = 'R';
	mCmd.Name[1] = 'K';
	mCmd.Mode = SET_PARAM;
	mCmd.Time = GetTime();
	mCmd.Treshold = (uint16_t)treshold;
	mCmd.Len_Data = MAX_LEN_DATA;

	if (pTask->pSerialCell) {
		EnterCriticalSection(&pTask->CrSec);
		DWORD dwWritten;
		pTask->pSerialCell->WriteData((uint8_t *)&mCmd, sizeof(Input_Data), &dwWritten);
		pTask->outsms++;
		LeaveCriticalSection(&pTask->CrSec);
		DEBUG(TRACE, "TX SET_PARAM  %i:%i\r\n", mCmd.Treshold, mCmd.Time);
		return TRUE;
	}
	return FALSE;

}
/////////////////////////////////////////////////////////////////////////
BOOL	  VCPP_API   TestSMS(HANDLE handle, void *data, size_t size) {
	if (handle == INVALID_HANDLE_VALUE)
		return FALSE;

	Task *pTask = (Task*)handle;
	if (pTask->pSerialCell) {
		EnterCriticalSection(&pTask->CrSec);
		DWORD dwWritten;
		pTask->pSerialCell->WriteData((BYTE *)data, size, &dwWritten);
		pTask->outsms++;
		LeaveCriticalSection(&pTask->CrSec);
		DEBUG(TRACE, "TestSMS:%i\r\n", GetTime());
		return TRUE;
	}
	return FALSE;
}
/////////////////////////////////////////////////////////////////////////
HANDLE VCPP_API   OpenModem(int port,HANDLE hwnd, uint32_t debug_lev) {
	

	Task *pTask = NULL;
	//Is Open ?
	for (size_t i = 0; i < MAX_MODEM; i++) {
		if (mTask[i].m_hThread == INVALID_HANDLE_VALUE)
			continue;
		//is open
		if (mTask[i].ComPort == port)
			if(mTask[i].hwnd= hwnd)
				return (HANDLE)&mTask[i];
			else return (HANDLE)INVALID_HANDLE_VALUE;
	}
	//Search empty
	for (size_t i = 0; i < MAX_MODEM; i++) {
		if (mTask[i].m_hThread == INVALID_HANDLE_VALUE) {
			pTask = &mTask[i];
			break;
		}
	}
	 if(!pTask)
		 return (HANDLE)INVALID_HANDLE_VALUE;
	 pTask->ComPort = port;
	 pTask->hwnd = hwnd;
	 debug_level = debug_lev;
	 if (debug_lev) {
		 if (AllocConsole()) {
			 FILE* fDummy;
//			 freopen_s(&fDummy, "CONOUT$", "w", stdout);
			 freopen_s(&fDummy, "CONOUT$", "w", stderr);
//			 freopen_s(&fDummy, "CONIN$", "r", stdin);
			 HANDLE hConOut = CreateFile("CONOUT$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
//			 HANDLE hConIn = CreateFile("CONIN$", GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
//			 SetStdHandle(STD_OUTPUT_HANDLE, hConOut);
			 SetStdHandle(STD_ERROR_HANDLE, hConOut);
//			 SetStdHandle(STD_INPUT_HANDLE, hConIn);
		 }

	 }


	 pTask->m_hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)threadProc,
	 pTask,
	 0, /*CREATE_SUSPENDED*/
	 &pTask->m_idThread);
	 Sleep(1);
     //wait start thread		
	 while(pTask->pSerialCell==NULL){
		 Sleep(0);
	 }

	 //Проверка на открытие порта 
	 DWORD error = pTask->pSerialCell->GetError();
	 if (error) {

		 pTask->pSerialCell->OutError(error);
		 DEBUG(TRACE, "Error open port COM%i\r\n", port);
		 CloseModem((HANDLE)pTask);
			return (HANDLE)INVALID_HANDLE_VALUE;
	 }
	 else   return (HANDLE)pTask;
}
/////////////////////////////////////////////////////////////////////////
BOOL VCPP_API  CloseModem(HANDLE handle)
{
    
	if (handle == INVALID_HANDLE_VALUE)
		return FALSE;

	Task *pTask = (Task*)handle;
	if (pTask->m_hThread != INVALID_HANDLE_VALUE) {
		PostThreadMessage(pTask->m_idThread, WM_QUIT, 0, 0);

		//Wait end Thread
		while (pTask->m_hThread != INVALID_HANDLE_VALUE)
		{
			Sleep(0);
		};

		pTask->ComPort = 0;
		pTask->m_idThread = 0;


	}

	return TRUE;
}
/////////////////////////////////////////////////////////////////////////
void VCPP_API SetDataModem(HANDLE handle,DWORD TimeU, float ESP)
{
	
	if (handle == INVALID_HANDLE_VALUE)
			return;

	Task *pTask = (Task*)handle;
	if (pTask->m_hThread == INVALID_HANDLE_VALUE)
				return;
	//Send_Data
	PostThreadMessage(
		pTask->m_idThread,
		SET_DATA,
		(WPARAM)TimeU,//WPARAM wParam, //time
		(LPARAM)(ESP * 100)// value
	);
}
///////////////////////////////////////////////////////////////
DWORD VCPP_API GetDataModem(HANDLE handle, void * pData) {
	if (handle == INVALID_HANDLE_VALUE)
		return 0;

	Task *pTask = (Task*)handle;
	int num = pTask->pEspFifo->getnum();
	if (num) {
		void * el;
		pTask->pEspFifo->getp(el);
		memcpy((flx_data *)pData, el, sizeof(flx_data));
	}
	return num;
    
}

/////////////////////////////////////////////////////////////////////////
	BOOL APIENTRY DllMain(HINSTANCE hinstDLL,
		DWORD fdwReason, LPVOID lpvReserved)
	{
		switch (fdwReason)
		{
		case DLL_PROCESS_ATTACH:  // Подключение DLL
		  // Выполняем все необходимые
		  // действия по инициализации
		  // если инициализация прошла
		  // успешно возвращаем TRUE
		  // в противном случае – FALSE
			return 1;

		case DLL_PROCESS_DETACH: // Отключение DLL
		  // Выполняем все действия
		  // по деинициализации
		   //fprintf(stdout, "DLL_PROCESS_DETACH\r\n");
			break;

		case DLL_THREAD_ATTACH: // Создание нового потока
		  // Переходим на многопоточную версию,
		  // если необходимо
		break;

		case DLL_THREAD_DETACH: // Завершение потока
		  // Освобождаем переменные, связанные с потоком
		break;
		}
		return TRUE;  // Возвращаем что-нибудь (все равно
			// код возврата игнорируется)
	}
