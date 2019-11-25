#ifndef __SOLARLIBH__
#define __SOLARLIBH__
#include <windows.h>
#include <stdio.h>
#include "DataSolar.h"
#include "debug.h"
#define MAX_MODEM 1

#ifdef _DLL_EXPORTS
#define VCPP_API __declspec(dllexport)
#else
#define VCPP_API __declspec(dllimport)
#endif

/*////////////////////////////////////////////////////////////////////////
* �������� �����(���������� �����) ����������(������) ��� ������ � ���������� ���������
* �������:
* (1) ����� Com �����
* (2) ����� ���� ��� �������� ���������,���� NULL �������� ��������� �� ����������
* (3) ������� ������� (������ ���� debug.h),���� 0 ������� ���������.
* ��������:
* ���������� ����� ������ ��� INVALID_HANDLE_VALUE ��� ��������� ������
*
* ����������
* ����������� ����� � ������ �����.
*/
extern "C" HANDLE VCPP_API   OpenModem(int com_port,HANDLE hwnd, uint32_t debug_level);

/*///////////////////////////////////////////////////////////////////////
�������� �������� ��������  �� ��������������
* �������:
* (1)����� ������ ���������� ����� �������  OpenModem
* (2) ����� UTC
* (3) ������� �������� � flx
* ����������:
* ������� ������������ �� �������  ��������������
* !!!!  ������ ������ ���������� !!!!!!
* ������� ����������� �� ������ ������ ��  �����  *.flx
*
*/
extern "C" void VCPP_API    SetDataModem(HANDLE handle,DWORD Time_UTC,float flx);

/*
* ������� �� �������� �������
*
*/

/*///////////////////////////////////////////////////////
�������� ������
* (1)����� ������ ���������� ����� �������  OpenModem
*  ���������� ����� �� �������
*/
extern "C" DWORD VCPP_API   GetDataModem(HANDLE handle, flx_data *pData);

/*//////////////////////////////////////////////////////
* ������ �������� �������
* (1)����� ������ ���������� ����� �������  OpenModem
*
*/
extern "C" BOOL	 VCPP_API   GetStatusModem(HANDLE handle);

/*//////////////////////////////////////////////////
��������� ������ ������������
* (1)����� ������ ���������� ����� �������  OpenModem
* (2)����� ������������
*/
extern "C" BOOL	 VCPP_API   SetTresHold(HANDLE handle, float treshold);


/*//////////////////////////////////////////////////
������ ��� �������
*/
extern "C" BOOL	  VCPP_API   TestSMS(HANDLE handle,uint8_t *data,size_t size);

/*///////////////////////////////////////////////////////////////////////
�������� �����
* (1)����� ������ ���������� ����� �������  OpenModem
*
*/
extern "C" BOOL	  VCPP_API    CloseModem(HANDLE handle);

#endif


