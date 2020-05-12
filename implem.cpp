#include "SDI12.cpp"
using namespace std;

char* sComm (int num, char *cmd[]) //melhorar retorno das informacoes
{
	//lock(50);
	long start, dif, ret;
	int i=1, c=0;
	char res[100];
	struct timespec tRef, tempo;
	start = tRef.tv_nsec; //guarda tempo de inicio
	
	clock_gettime(CLOCK_MONOTONIC, &tRef);
	incNano(&tRef, mili);
	wake(&tRef);
	//reconhece comando em ate 100ms a partir daqui
	
	do
	{
		
		//enviar comando... ---->> Testes indicam precisão em tempo maior que 4%
		cmdSend(&tRef, cmd[i], strlen(cmd[i]));
		
		clock_gettime(CLOCK_MONOTONIC, &tempo); 
		if ((tempo.tv_nsec)-(tRef.tv_nsec) > 100000) printf("cmdSend took too long to finish: %d\n\n", (tempo.tv_nsec)-(tRef.tv_nsec));
		else printf("cmdSend sucessful\n\n");
		

		
		int j=0;
		*(mapa+OE)=in;


		
		//clock_gettime(CLOCK_MONOTONIC, &tRef);	//preciso zerar isso aqui? delay menor que 100000
		
		/*
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
		*/
		i++;
		dif = tRef.tv_nsec-start;
		if(dif<=sec/1000) 
		{
			start = tRef.tv_nsec;
			printf("time\n");	
			wake(&tRef);
		}
	}
	while (i<num);
	
	
return 0;
}





void* monitor(void*)
{
	//lock(50);
	int k, tmr=0, dir;
    ofstream myfile;
    myfile.open ("saida.txt");
    struct timespec tRef;
    clock_gettime(CLOCK_MONOTONIC, &tRef);
    
    while(tmr<=200)
    {
    	tmr++;
        dir = (int)*(mapa+OE);
        //int dataout = *(mapa+DATAOUT);
        //int datain = *(mapa+DATAIN);
        
        *(mapa+OE) = in;
        k=pin;
        myfile << k;
        *(mapa+OE) = dir;
        incNano(&tRef, cycle);
    }
    
    myfile.close();
    printf("monitor end\n");
    return 0;
}
