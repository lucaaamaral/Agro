#include "implem.cpp"



int main(int argc, char *argv[])
{
	lock(80);
	int fd = open("/dev/mem", O_RDWR);
	mapa = (int*)mmap(0, GPIO0_LEN, PROT_READ | PROT_WRITE, MAP_SHARED, fd, GPIO0_START);
	if(mapa == MAP_FAILED) 
	{
			close(fd);
			perror("Unable to map /dev/mem");
			exit(EXIT_FAILURE);
	}
	
	
	pthread_t t;
	pthread_create(&t, 0, monitor, 0);
	
	sComm(argc, argv);
	pthread_join(t, 0);
	
	close(fd);
	munmap(mapa, GPIO0_LEN);
	
	printf("Beaware of time delays, they might kill your application\n\n");

return 0;
}














