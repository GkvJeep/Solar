//==================================================
#include <windows.h>
#include <conio.h>
#include "SolarLib.h"
#include "getopt.h"
#include "debug.h"

static void display_help(void)
{
	printf("Usage: \r\n"
		"\r\n"
		"  -h,  help\r\n"
		"  -b,  Baud rate, 115200, etc (115200 is default)\r\n"
		"  -p,  Port (1,2, etc) (must be specified)\r\n"
		"  -f,  File Name  (must be specified)\r\n"
		"  -t,  time delay (100 mS default)\r\n"
		"  -d,  debug\r\n"
		"\r\n"
	);
}
uint32_t debug_level;
int main(int argc, char **argv)
{
	
	int opt;
	int port=0;
	int baud = CBR_115200;
	int time = 100;
	char file[256] = { "D:\\Solar\\RFlux_3.flx" };
	if (argc == 1) { // если запускаем без аргументов, выводим справку)
		display_help();
		printf("Press any key..");
		while (!_kbhit()) {};
		return -1;
	}
	
	
	while ((opt = getopt(argc, argv, ":hp:b:f:t:d:")) != -1) {
		//printf("-%c %s ", opt, optarg);
		switch (opt) {
		case 'p': port = atoi(optarg); break;
		case 'b': baud = atoi(optarg); break;
		case 'f': sprintf(file,"%s",optarg); break;
		case 't': time = atoi(optarg); break;
		case '?':
		case 'h': display_help(); return -1; break;
		case 'd':
		{
			struct {
				const char *name;
				uint32_t value;
			} debug_options[] = {
			  {"all", 0xffffffffUL},
			  {"trace", DEBUG_TRACE},

/*			  {"io", DEBUG_IO},
			  {"mem", DEBUG_MEM},
			  {"warn", DEBUG_WARN},
			  {"op", DEBUG_OP},
			  {"int", DEBUG_INT},
			  {"lcd", DEBUG_LCD},
			  {"serial", DEBUG_SERIAL},
			  {"abridged", DEBUG_ABRIDGED},
			  {"iface", DEBUG_IFACE},
			  {"os", DEBUG_OS},
			  {"key", DEBUG_KEY},
			  {"hsio", DEBUG_HSIO},
			  {"event", DEBUG_EVENT},
			  {"ui", DEBUG_UI},
			  {"hints", DEBUG_HINTS}, */
			  {NULL, 0},
			};
			char *d = strtok(optarg, ",");
			do {
				int i;
				for (i = 0; debug_options[i].name; i++) {
					if (d[0] == '-' && !strcmp(debug_options[i].name, d + 1)) {
						debug_level &= ~debug_options[i].value;
						break;
					}
					else if (!strcmp(debug_options[i].name, d)) {
						debug_level |= debug_options[i].value;
						break;
					}
				}
			} while ((d = strtok(NULL, ",")));
		}
			break;
		}
	}

	if (!port) {
		display_help(); return -1;
	}
	if (time < 100)
		time = 100;

	SYSTEMTIME sm;
	GetLocalTime(&sm);
	
	HANDLE handle = OpenModem(port, NULL, debug_level);

	if (handle != INVALID_HANDLE_VALUE) {
		int Len;
		int ch=0;
		FILE *stream;
		int cnt = 0;
		printf("..Press any key to exit\r\n");
		do{

//			if(fopen_s(&stream, "D:\\Solar\\RFlux_3.flx", "r"))
			if (fopen_s(&stream, file, "r"))
				break;

			float esp;
			char cmd[128];

			while ((Len = fscanf_s(stream, "%s", cmd, sizeof(cmd))) != EOF) 
			{
				fscanf_s(stream, "%f", &esp);


				char t_cmd[64] = { 0 };
				unsigned j = 0;

				for (size_t i = 0; i < strlen(cmd); i++)
					if (cmd[i] == '_') continue;
					else
					{
						t_cmd[j++] = cmd[i];
					}

				t_cmd[j] = 0;
				//получить время
				unsigned TimeU;
				sscanf_s(t_cmd, "%d", &TimeU);
				
				
				//Send_Data
				SetDataModem(handle, TimeU, esp);
				if (++cnt >= 960) {
				    Sleep(time);
					cnt = 0;
				}


				//Обработка ввода
				if (_kbhit()) {
					 ch = _getch();
					break;
				} else
				ch = 0;


			} //!= EOF
			fclose(stream);
		} while (!ch);

		CloseModem(handle);
	}


 return -1;	
}	