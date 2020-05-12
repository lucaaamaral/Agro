#include "SDI12.hpp"


void lock(int lvl)
{
	if(mlockall(MCL_FUTURE | MCL_CURRENT) !=0) perror("Erro no mlockall");
	struct sched_param sp;
	memset(&sp, 0, sizeof(sp));
	sp.sched_priority = lvl;
	if(sched_setscheduler(0, SCHED_FIFO, &sp) !=0 ) perror("Erro no setscheduler");
	
}


void incNano(struct timespec *tRef, long t) //Ok
{
	tRef->tv_nsec+=t;
		if(tRef->tv_nsec > sec)
		{
			tRef->tv_nsec -= sec;
			tRef->tv_sec++;
		}
	clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, tRef, 0);
return;
}

void wake(struct timespec *tRef)
{
	*(mapa+OE) = out; //direction
	clock_gettime(CLOCK_MONOTONIC, tRef);
	*(mapa+DATAOUT) = high; //set datapin as high and wait 12ms
	incNano(tRef, (long)(13*mili));
	*(mapa+DATAOUT) = low; //set datapin as low and wait 8,33ms
	incNano(tRef, 10*cycle);//espera 10, mas um esta em cmdSend
return;
}


//enviar comando
void cmdSend(struct timespec *tRef, char *cmd, int len) //nao ok
{
	printf("\nSending command... - %s\n", cmd);
	int c=0;
	int par = 0;
	//*(mapa+OE) = out;	//set pin as output
	
	while(len!=c)	//um caractere por vez
	{
		incNano(tRef, cycle); //acrescimo entre caracteres
		*(mapa+DATAOUT) = high;	//start bit espera um ciclo
		printf("1");
		incNano(tRef, cycle);
		par=0;
		
		for(int i=0; i<7; i++) //data bits
		{
			*(mapa+DATAOUT) = ( ((1&(cmd[c]>>i))==1)? low:high); //envia bit por bit invertido
			printf("%d",(*(mapa+DATAOUT)>>saida)&1);
			par ^= pin;	//xor//set parity
			incNano(tRef, cycle); //espera um ciclo
		}
		//parity bit e espera um ciclo
		*(mapa+DATAOUT) = ((par&1==1)? high: low);
		printf("%d",(*(mapa+DATAOUT)>>saida)&1);
		incNano(tRef, cycle);
		*(mapa+DATAOUT) = low; //stop bit
		printf("%d ",(*(mapa+DATAOUT)>>saida)&1);
		c++;
	}
	
	incNano(tRef, cycle);
	//abandonar controle da linha de dados em ate 7.5ms
	//colocar pino em tree-state
	*(mapa+OE) = in;
return;
}











