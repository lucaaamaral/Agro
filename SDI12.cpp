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
		//printf("1");
		incNano(tRef, cycle);
		par=0;
		
		for(int i=0; i<7; i++) //data bits
		{
			*(mapa+DATAOUT) = ( ((1&(cmd[c]>>i))==1)? low:high); //envia bit por bit invertido
			//printf("%d",(*(mapa+DATAOUT)>>saida)&1);
			par ^= pin;	//xor//set parity
			incNano(tRef, cycle); //espera um ciclo
		}
		//parity bit e espera um ciclo
		*(mapa+DATAOUT) = ((par&1==1)? high: low);
		//printf("%d",(*(mapa+DATAOUT)>>saida)&1);
		incNano(tRef, cycle);
		*(mapa+DATAOUT) = low; //stop bit
		//printf("%d ",(*(mapa+DATAOUT)>>saida)&1);
		c++;
	}
	printf("\n");
	//abandonar controle da linha de dados em ate 7.5ms
	//colocar pino em tree-state
	*(mapa+OE) = in;
return;
}


bool read_bit(struct timespec *tRef)
{
	bool i1, i2, i3;
	i1=pin;
	incNano(tRef, (cycle/3));
	i2=pin;
	incNano(tRef, (cycle/3));
	i3=pin;
	
	incNano(tRef, (cycle/3));

	if (i1==i2) return i1;
	else if(i1==i3) return i1;
	else return i2;
	

}

bool charStream(struct timespec *tRef, char *c)
{
	long dif, now;
	bool par=0, k;
	now = tRef->tv_nsec;
	do
	{
		incNano(tRef, cycle/3);
		//ver se start bit esta ativado e sincronizar o clock
		dif = (tRef->tv_nsec) - now;
	}while(pin==0 && dif<15*mili); //espera ate 15ms
	
	if(dif>=15*mili)
	{	//indicar erro
		printf("Timeout charStream\n\n");
		*c='\n';
		*(c+1)='-';
		*(c+2)=0;
		return false;
	}
	
	incNano(tRef, cycle);
	par=0;
	dif = -1;
	*c=0;
	while(dif<6)
	{
		dif++;
		k=read_bit(tRef);
		*c+=(k^1)<<dif;
		par^=k;
	}
	//incNano(tRef, cycle); //paritybit
	if(par != read_bit(tRef)) //erro - configurar comportamento depois
	{	//indicar erro
		printf("Invalid parity check - %d %d\n\n", par, pin);
		*c='\n';
		*(c+1)='-';
		*(c+2)=1;
		return false;
	}
	
return true;
}




void cmdRecv(struct timespec *tRef, char *res)
{
	printf("Waiting for response...\n");
	char r[80];//max 75 char
	int i=0;

	long dif, now = tRef->tv_nsec;
	
	//espera marking do sensor 8,33ms
	//sensor deve responder em ate 15ms [18 ciclos]
	do
	{	
		dif = (tRef->tv_nsec) - now;
		//ver se start bit esta ativado e sincronizar o clock
	}while( (dif<15*mili )&& !charStream(tRef, &r[i]) );
	
	if (dif >= 15*mili)	//timeout
	{
		printf("Timeout cmdRecv\n\n");
		res = 0;
		return;
	}
	
	i++;
	do
	{
		charStream(tRef, &r[i]); // ====>>>>>>>> entrando com delay grande
		i++;
	}while(r[i-1] != 10);
	
	res = r;
	if(r[i]=='-') res = 0;
	incNano(tRef, cycle*8);	//espera antes de poder enviar outro comando
	if (res==0) printf("Error on response\n%s\n", r);
	else printf("Response was: %s\n", r);
return;
}









