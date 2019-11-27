#ifndef __SOLARLIBH__
#define __SOLARLIBH__
#include <windows.h>
#include <stdio.h>
#define MAX_MODEM 1

typedef unsigned int uint32_t;
typedef unsigned short uint16_t;
typedef unsigned char uint8_t;



#ifdef _DLL_EXPORTS
#define VCPP_API __declspec(dllexport)
#else
#define VCPP_API __declspec(dllimport)
#endif

/*////////////////////////////////////////////////////////////////////////
* Получить хендл(уникальный номер) устройства(модема) для работы с остальными функциями
* Входные:
* (1) номер Com порта
* (2) хендл окна для отправки сообщений,если NULL отправка сообщений не происходит
* (3) уровень отладки (смотри файл debug.h),если 0 отладка отключена.
* Выходные:
* Возврашает хендл модема или INVALID_HANDLE_VALUE при отсутсвии модема
*
* Примечание
* Однократный вызов в начале сесии.
*/
extern "C" HANDLE VCPP_API   OpenModem(int com_port,HANDLE hwnd, uint32_t debug_level);

/*///////////////////////////////////////////////////////////////////////
Отправка текушего значения  от радиотелескопа
* Входные:
* (1)Хендл модема полученный через функцию  OpenModem
* (2) Время UTC
* (3) Текущее значение в flx
* Примечание:
* Функция используется на стороне  радиотелескопа
* !!!!  Шрамко Андрей Дмитриевич !!!!!!
* Функция реализована на основе данных по  файлу  *.flx
*
*/
extern "C" void VCPP_API    SetDataModem(HANDLE handle,DWORD Time_UTC,float flx);

////////////////////////////////////////////////////////////////////////
/*
* Функции на приемной стороне
*
*/
/*///////////////////////////////////////////////////////
Получить данные
* (1)Хендл модема полученный через функцию  OpenModem
*  Примечание вызов по событию
*/
extern "C" DWORD VCPP_API   GetDataModem(HANDLE handle, void *pData);

/*//////////////////////////////////////////////////////
* Запрос текушего статуса
* (1)Хендл модема полученный через функцию  OpenModem
*
*/
extern "C" BOOL	 VCPP_API   GetStatusModem(HANDLE handle);

/*//////////////////////////////////////////////////
Установка порога срабатывания
* (1)Хендл модема полученный через функцию  OpenModem
* (2)Порог срабатывания
*/
extern "C" BOOL	 VCPP_API   SetTresHold(HANDLE handle, float treshold);


/*//////////////////////////////////////////////////
Только для отладки
*/
extern "C" BOOL	  VCPP_API   TestSMS(HANDLE handle, void *data,size_t size);

/*///////////////////////////////////////////////////////////////////////
Закрытие сесии
* (1)Хендл модема полученный через функцию  OpenModem
*
*/
extern "C" BOOL	  VCPP_API    CloseModem(HANDLE handle);

#endif


