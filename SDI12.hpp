#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <unistd.h>
#include <sched.h>
#include <fstream>
#include <iostream>
#include <pthread.h>

#define GPIO0_START 0X44E07000
#define GPIO0_END   0X44E07FFF
#define GPIO0_LEN (GPIO0_END-GPIO0_START)

#define SYSCONFIG		0X010/4
#define CTRL 			0X130/4
#define OE 				0X134/4
#define DATAIN 			0X138/4
#define DATAOUT 		0x13C/4
#define CLEARDATAOUT	0X190/4
#define SETDATAOUT 		0X194/4

#define saida 20

#define cycle 833333 //nanoseconds
#define sec 1000000000 //second in nanoseconds
#define mili 1000000

#define out (*(mapa+OE))&(~(1<<saida))	//pin output
#define in  (*(mapa+OE))|(1<<saida)		//pin input

#define low		(*(mapa+DATAOUT))&(~(1<<saida))
#define high	(*(mapa+DATAOUT))|(1<<saida)

#define pin  (*(mapa+DATAIN)&(1<<saida))>>saida


void incNano(struct timespec*, long); //incrementa e espera nanosegundos
void cmdSend(struct timespec, char*, int); //send command
void charStream (struct timespec*, char*);	//receive a char
void cmdRecv (struct timespec, char*); //receive response
char* sComm (int, char**);	//sensor communication

int *mapa;

/*
Comunicacao 1200 baud
negative logic [logica inversa]
slew rate must not be grater than 1,5 volts per microsecond

inicio de comunicacao -> 12 ms 'break' [logico alto]
--14.4 ciclos de mudanca - pode arredondar para 14.5 ou 15?
envia nome do sensor [char - 7bits]
--
envia comando para realizar leitura
--
sensor responde em ate 15ms
-- todas as respostas terminam em <CR><LF> == <13><10> == <0D><0A>
Comando enviar leitura


comandos

a!
aI!
aAb!
?!
aM! -- start measurement
aMC!
aD0! [...] aD9! -- send data
aM1! [...] aM9!
aMC1! [...] aMC9!
aV!
aC!**


*/

