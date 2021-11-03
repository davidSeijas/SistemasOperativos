#include <stdlib.h>
#include <stdio.h>


int system (const char* command){
    int status;
    pid_t pid;

    pid = fork();
    if (pid > 0){ //Proceso padre
        while (pid != wait(&status)); //pid vale el pid del hijo en el proceso padre. Esto espera a que acabe un hijo concreto (wait devuelve el pid del hijo que termina)
    }
    else if (pid == 0){ //Proceso hijo
        execl("/bin/bash", "bash", "-c", command, NULL);
        fprintf(stderr, "Error en la ejecución del exec");
        exit(-1);
    }
    else{ //tratamiento de error
        fprintf(stderr, "Error en la creación del proceso hijo (pid < 0)");
        exit(-1);
    }
}


int main(int argc, char* argv[])
{
	if (argc!=2){
		fprintf(stderr, "Usage: %s <command>\n", argv[0]);
		exit(1);
	}

	return system(argv[1]);
}

