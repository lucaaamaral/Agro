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

#define out (*(mapa+OE))&(~(1<<saida))	//pin output
#define in  (*(mapa+OE))|(1<<saida)		//pin input

#define low (*(mapa+DATAOUT))|(1<<saida)
#define high  (*(mapa+DATAOUT))&(~(1<<saida))

#define pin  (*(mapa+DATAIN)&(1<<saida))>>saida


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
void incNano(struct timespec*, long); //incrementa e espera nanosegundos
void cmdSend(struct timespec, char*, int); //send command
void charStream (struct timespec*, char*);	//receive a char
void cmdRecv (struct timespec, char*); //receive response
char* sComm (int, char**);	//sensor communication

int main(int argc, char *argv[])
{
	if(mlockall(MCL_FUTURE | MCL_CURRENT) !=0) perror("Erro no mlockall");
	struct sched_param sp;
	memset(&sp, 0, sizeof(sp));
	sp.sched_priority = 80;
	if(sched_setscheduler(0, SCHED_FIFO, &sp) !=0 ) perror("Erro no setscheduler");
	
	int fd = open("/dev/mem", O_RDWR);
	mapa = (int*)mmap(0, GPIO0_LEN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, GPIO0_START);
	if(mapa == MAP_FAILED) 
	{
			close(fd);
			perror("Unable to map /dev/mem");
			exit(EXIT_FAILURE);
	}
	
	sComm(argc, argv);
	
	close(fd);
	munmap(mapa, GPIO0_LEN);
	
	printf("Beaware of time delays, they might kill your application\n\n");

return 0;
}



void incNano(struct timespec *tRef, long t) //Ok
{
	tRef->tv_nsec+=t;
		if(tRef->tv_nsec > sec)
		{
			//printf("-- swap\n");
			tRef->tv_nsec -= sec;
			tRef->tv_sec++;
		}
	clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, tRef, 0);
return;
}
	
//enviar comando
void cmdSend(struct timespec *tRef, char *cmd, int len) //Ok
{
	printf("\nSending command... - %s\n", cmd);
	int c=0;
	bool par = 0;
	
	*(mapa+OE) = out;	//set pin as output
	while(len!=c)
	{
		incNano(tRef, cycle+300); //acrescimo entre caracteres
		*(mapa+DATAOUT) = high;	//start bit espera um ciclo
		incNano(tRef, cycle);
		
		for(int i=0; i<8; i++) //data bits
		{
			*(mapa+DATAOUT) = ( ((1&(cmd[c]>>i))==1)? high:low); //envia bit por bit invertido
			par ^= (bool)pin;	//xor//set parity
			incNano(tRef, cycle); //espera um ciclo
		}
		//parity bit e espera um ciclo
		*(mapa+DATAOUT) = ((par==true)? high: low);
		incNano(tRef, cycle);
		*(mapa+DATAOUT) = low; //stop bit
		c++;
	}
	incNano(tRef, cycle);
	//abandonar controle da linha de dados em ate 7.5ms
	//colocar pino em tree-state
	*(mapa+OE) = in;
	
	
return;
}

void charStream(struct timespec *tRef, char *c)
{
	struct timespec agora;
	clock_gettime(CLOCK_MONOTONIC, &agora);
	printf("charStream in... now is: %d, delay of %d\n\n", agora.tv_nsec, (agora.tv_nsec)-(tRef->tv_nsec));
	long dif, now;
	bool par=0;
	now = tRef->tv_nsec;
	do
	{
		
		incNano(tRef, cycle/3);
		//ver se start bit esta ativado e sincronizar o clock
		clock_gettime(CLOCK_MONOTONIC, &agora);
		dif = (agora.tv_nsec)-(tRef->tv_nsec);
		printf("pin: %d %d\n\n", dif, pin);
	}while(pin==0 && dif<15000); //espera ate 15ms
	if(dif>=15000)
	{	//indicar erro
		printf("Timeout charStream\n\n");
		*c='\n';
		*(c+1)='-';
		*(c+2)=0;
		return;
	}
	par=0;
	printf("\n%d ", pin);
	dif = 7;
	*c=0;
	while(dif>0)
	{
		dif--;
		incNano(tRef, cycle); //espera e le
		printf("%d", pin);
		*c+=pin<<(dif);
		par^=(bool)pin;
	}
	printf(" ");
	incNano(tRef, cycle); //paritybit
	if(par != (bool)pin); //erro - configurar comportamento depois
	{	//indicar erro
		printf("Invalid parity check - %d %d\n\n", (int)par, 0^0);
		*c='\n';
		*(c+1)='-';
		*(c+2)=1;
		return;
	}
	
return;
}




void cmdRecv(struct timespec *tRef, char *res)
{
	printf("Waiting for response...\n");
	char r[80];//max 75 char
	int i=0;
	struct timespec agora;

	long dif, now = tRef->tv_nsec;
	
	//testa o start bit do sensor
	//sensor deve responder em ate 15ms [18 ciclos]
	do
	{	
		incNano(tRef, cycle/3);
		dif = (tRef->tv_nsec) - now;
		clock_gettime(CLOCK_MONOTONIC, &agora);
		//if(agora.tv_nsec<tRef->tv_nsec)printf("erro %d", (agora.tv_nsec-tRef->tv_nsec));
		//ver se start bit esta ativado e sincronizar o clock
	}while( (dif<cycle*18 )&&(pin == 0) );
	
	if (dif >= cycle*18)	//timeout
	{
		printf("Timeout cmdRecv\n\n");
		res = 0;
		return;
	}
	
	
	incNano(tRef, cycle);	//start bit
	printf("--> pin if zero, error detected: %d %ld\n", pin, dif);
	do
	{
		charStream(tRef, &r[i]); // ==========>>>>>>>> entrando com delay grande
		incNano(tRef, cycle-200); //espera ate pouco antes da proxima leitura de letra
		i++;
	}while(r[i-1] != '\n');
	
	res = r;
	if(r[i]=='-') res = 0;
	incNano(tRef, cycle*8);	//espera antes de poder enviar outro comando
	if (res==0) printf("Error on response\n%s\n", r);
	else printf("Response was: %s\n", r);
return;
}

void wake(struct timespec *tRef)
{
	*(mapa+OE) = in; //direction
	clock_gettime(CLOCK_MONOTONIC, tRef);
	*(mapa+DATAOUT) = high; //set datapin as high and wait 12ms
	incNano(tRef, (long)(14.42*cycle));
	*(mapa+DATAOUT) = low;//set datapin as low and wait 8,33ms
	incNano(tRef, cycle*9);//espera 10, mas um esta em cmdSend
return;
}

char* sComm (int num, char *cmd[]) //melhorar retorno das informacoes
{
	long start, dif, ret;
	int i=1, c=0;
	char res[100];
	struct timespec tRef, tempo;
	
	wake(&tRef);
	start = tRef.tv_nsec;	//guarda tempo de inicio
	//reconhece comando em ate 100ms a partir daqui
	
	do
	{
		//enviar comando...
		cmdSend(&tRef, cmd[i], strlen(cmd[i]));
		clock_gettime(CLOCK_MONOTONIC, &tempo); 
		if ((tempo.tv_nsec)-(tRef.tv_nsec) > 100000) printf("cmdSend took too long to finish: %d\n\n", (tempo.tv_nsec)-(tRef.tv_nsec));
		else printf("cmdSend sucessful\n\n");
		
		int j=0;
		while( j<300)
		{
			j++;
			clock_gettime(CLOCK_MONOTONIC, &tempo); 
			incNano(&tempo, cycle);
			printf("%d", pin);
		}
		
		//clock_gettime(CLOCK_MONOTONIC, &tRef);	//preciso zerar isso aqui? delay menor que 100000
		
		ret=tRef.tv_nsec;
		//sensor deve responder em ate 15ms [18 ciclos]
		cmdRecv(&tRef, res);
		clock_gettime(CLOCK_MONOTONIC, &tempo); 
		if ((tempo.tv_nsec)-(tRef.tv_nsec) > 100000) printf("cmdRecv took too long to finish: %d\n\n", (tempo.tv_nsec)-(tRef.tv_nsec));
		else printf("cmdRecv sucessful\n\n");
		
		dif=tRef.tv_nsec-ret;
		if(cmd==0 && dif<87000 && dif>16670) //retry
		{
			printf("error - reattempting to issue command...\n");
			cmdSend(&tRef, cmd[i], strlen(cmd[i]));
			cmdRecv(&tRef, res);
		}
		else
		{
			printf("%s\n", res);	//imprimir resposta para verificar
			cmd[i] = res;
		}
		i++;
		dif = tRef.tv_nsec-start;
	}
	while (i<num && dif<=sec/1000);
	
	
return 0;
}





