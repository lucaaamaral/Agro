#include "SDI12.cpp"
using namespace std;

char* sComm (int num, char *cmd[]) //melhorar retorno das informacoes
{
	//lock(50);
	//int *mapa;
	lock(80);
	
	unsigned long start, dif, ret;
	int i=1, c=0;
	char res[100];
	struct timespec tRef, tempo;
	
	
	clock_gettime(CLOCK_MONOTONIC, &tRef);
	start = tRef.tv_nsec; //guarda tempo de inicio
	incNano(&tRef, mili);
	wake(&tRef);
	//reconhece comando em ate 100ms a partir daqui
	
	do
	{
		cmdSend(&tRef, cmd[i], strlen(cmd[i]));	//enviar comando...
		ret=tRef.tv_nsec;	//tempo referencia para reenviar comando
		clock_gettime(CLOCK_MONOTONIC, &tRef);	//preciso zerar isso aqui? delay menor que 100000
		cmdRecv(&tRef, res); //sensor deve responder em ate 15ms [18 ciclos]
		
		dif=tRef.tv_nsec-ret;
		
		if(res==0 && (dif<87*mili || dif>16.67*mili)) //retry
		{
			printf("error - reattempting to issue command...\nres = %s\ndif = %ld | %ld\n", res, dif, 87*mili);
			//incNano(&tRef, cycle);
			//wake(&tRef);
			cmdSend(&tRef, cmd[i], strlen(cmd[i]));
			cmdRecv(&tRef, res);
		}
		else
		{
			//printf("%s\n", res);	//imprimir resposta para verificar
			switch (cmd[i][1])
			{
				case 'M':
				{
					//strcpy(cmd[i], res);
					//printf("%s\n", res);
					long num = (res[1]-48)*100+(res[2]-48)*10+(res[3]-48);
					incNano(&tRef, num*sec);
					break;
				}
				case 'D':
				{
					//printar no arquivo
					to_file(res);
					break;
				}
				default:
				{
					break;
				}
				
			}
		}
		
		
		i++;
		dif = tRef.tv_nsec-start;
		if(dif>=sec/1000) 
		{
			start = tRef.tv_nsec;	
			wake(&tRef);
		}
		
		
		
	}
	while (i<num);
	
	//to_file(res);
return 0;
}



/*

void* monitor(void*)
{
	//lock(30);
	int k, tmr=0, dir;
    FILE *myfile = fopen ("saida.txt", "a+");
    struct timespec tRef;
    clock_gettime(CLOCK_MONOTONIC, &tRef);
    //incNano(&tRef, 20*mili);
    
    while(tmr<=1000)
    {
    	tmr++;
        dir = (int)*(mapa+OE);
        //int dataout = *(mapa+DATAOUT);
        //int datain = *(mapa+DATAIN);
        
        *(mapa+OE) = in;
        fprintf(myfile, "%d", pin);
        *(mapa+OE) = dir;
        incNano(&tRef, cycle/3);
        if(tmr%100==0) fprintf(myfile, "\n");
    }
    
    fclose(myfile);
    printf("monitor end\n");
    return 0;
}
*/
