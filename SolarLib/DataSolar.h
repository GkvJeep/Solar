//---------------------------------------------------------------------------

#ifndef DataSolarH
#define DataSolarH
#include <time.h>
#include "SolarLib.h"

#define MAX_ESP 10000.f      //Константа нормирования к диапазону 0.0 ... 1.0
#define MAX_LEN_DATA 120     //Максимальная длина буфера для данных
#define	TRESHOLD   1000      //Порог срабатывания

//Cобытия
#define	ESP_EVENT  WM_USER+100
#define RX_SERIAL_EVENT (ESP_EVENT+1)
#define SET_DATA  (RX_SERIAL_EVENT+1)

//==============================================================
 enum {
	 CMD_HOST=0,
	 SET_PARAM,       // команды управления от ведушего
	 GET_STATUS,   	  // Запрос  текущего статуса
	 RET_STATUS,      // Возврат текущего статуса
	 ESP_DATA     	  //данные от радиотелескопа
 };
 enum {
	 START_TASK = 0,
	 STOP_TASK,
	 RUN_TASK
 };

///////////////////////////////////////////////////////////////////////////////////////////
typedef unsigned char  CVSD_t;    // 8  bits

#pragma pack(push)
#pragma pack(1)

 /*
 Максимальный размер одной SMS ограничен (по стандарту) 140 байт,
 определяем структуру от радиотелескопа к хосту
 20 байт заголовок,120 байт данные (8 bit format).
 */
 typedef struct {
						  //Index
 uint8_t Start_byte;      //[0]  всегда 0x01 (по ASCII код символа SOH )
 uint8_t  Name[2];        //[1,2] ( 'R','K' )
 uint8_t  Mode;           //[3]  CMD_HOST,STATUS,FLM .....
 uint32_t Time;           //[4,5,6,7] (Время по UTC для события)
 uint16_t Len_Data;       //[8,9] (количество байт в обьединение (bData )
 uint16_t MaxValue;       //[10,11] (Значение по которому произошло событие)
 uint16_t Treshold;       //[12,13] Текущее значение порога
 float    Ref_CVSD;       //[14,15,16,17] Текущее значение  CVSD
 uint8_t  bitref_CVSD;    //[18] 
 uint8_t  CS;             //[19] (Контрольная сумма по модулю 256)
						  //[20....139]  данные max 120 байт
   CVSD_t  bData[MAX_LEN_DATA];   // данные в кодировке  CVSD (смотри ниже класс CVSD )

 }Input_Data,*pInput_Data;

 

 typedef struct {
	 DWORD timeU;                   //4
	 uint16_t treshold;             //2
	 uint16_t max_value;            //2 
	 float flx[MAX_LEN_DATA * 8];   //4 * 960
	 uint16_t cnt_sms;
 }flx_data, *pflx_data;
#pragma pack(pop)

 
 /*==============================================================
	От телескопа работает энкодер,на приемной стороне декодер.
	Based  CVSD encodes at 1 bit per sample
	Continuously variable slope delta modulation
	https://en.wikipedia.org/wiki/Continuously_variable_slope_delta_modulation
   ============================================================== */
 class CVSD{
  private:
	uint32_t num_bits;
	uint8_t bitref;   		// historical bit reference
	uint8_t bitmask;  		// historical bit reference mask
	float ref;              // internal reference

	float zeta;             // delta step factor
	float delta;            // current step size
	float delta_min;        // minimum delta
	float delta_max;        // maximum delta

  public:
   //constructor
   CVSD(uint32_t _num_bits=3,float _zeta = 1.5f):
		num_bits(_num_bits),
		bitref(0),
		ref(0.0f),
		zeta(_zeta),
		delta(0.001f),delta_min(0.001f),
		delta_max(1.0f)
   {
		bitmask = (1<<(num_bits)) - 1;
   }
 //===================================================================
   float get_ref() { return ref; }
   void set_ref(float new_ref){ref = new_ref;}

   uint8_t get_bitref() { return bitref; }
   void set_bitref(uint8_t new_bitref){bitref = new_bitref;}



	// encode single sample
	unsigned char cvsd_encode(float sample)
	{
		// determine output value
		unsigned char bit = (ref > sample) ? 0 : 1;

		// shift last value into buffer
		bitref <<= 1;
		bitref |= bit;
		bitref &= bitmask;

		// update delta
		if (bitref == 0 || bitref == bitmask)
			delta *= zeta;  // increase delta
		else
			delta /= zeta;  // decrease delta

		// limit delta
		delta = (delta > delta_max) ? delta_max : delta;
		delta = (delta < delta_min) ? delta_min : delta;
		// update reference
		ref += (bit) ? delta : -delta;
		// limit reference
		ref = (ref >  1.0f) ?  1.0f : ref;
		ref = (ref < -1.0f) ? -1.0f : ref;

		return bit;
	}
	//===================================================================
	// decode single sample
	float cvsd_decode( unsigned char bit)
	{
		// append bit into register
		bitref <<= 1;
		bitref |= (bit & 0x01);
		bitref &= bitmask;

		// update delta
		if (bitref == 0 || bitref == bitmask)
			delta *= zeta;  // increase delta
		else
			delta /= zeta;  // decrease delta

		// limit delta
		delta = (delta > delta_max) ? delta_max : delta;
		delta = (delta < delta_min) ? delta_min : delta;

		// update reference
		ref += (bit & 0x01) ? delta : -delta;

		// limit reference
		ref = (ref >  1.0f) ?  1.0f : ref;
		ref = (ref < -1.0f) ? -1.0f : ref;

		return ref;
	}
	//==============================================================================
	//encode 8 samples
	unsigned char cvsd_encode8(  float * samples)
	{
	unsigned char data=0x00;
	unsigned int i;
	for (i=0; i<8; i++) {
		data <<= 1;
		data |= cvsd_encode(samples[i]);
	}

		// set return value
		return data;
	}
	//====================
	// decode 8 samples
	void cvsd_decode8(unsigned char _data,
					float * samples)
{
	unsigned char bit;
	unsigned int i;
	for (i=0; i<8; i++) {
		bit = (_data >> (8-i-1)) & 0x01;
		samples[i] = cvsd_decode(bit);
	}
}
 };
//=============================================================
//Рекурсивный фильтр скользящего среднего
// https://habr.com/ru/post/325590/
class Filter
{
	 private:
			int pnt;
			float *prevData;
			float y;
			size_t tap;
 public:
 //constructor
	Filter(int Tap): pnt(0),y(0.f),tap(Tap)	{
		prevData = new float[Tap];
		//clear history
		for(size_t i = 0;i<tap;i++) prevData[i] = 0.f;
	}

	~Filter(){ delete []prevData; }

	float  run_filter(float x)	{
		y = y + (x - prevData[pnt]);
		prevData[pnt] = x;
		pnt = (pnt + 1) % tap;
		return y/tap;
	}

	float  get_current(){return y/tap;}
};

#endif


