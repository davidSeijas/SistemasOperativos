#include <stdlib.h>
#include <stdio.h>

int main(int argc, char* argv[]){
    if(argc != 2){
        fprintf(stderr, "Elige modo: 1 (circular) o 2 (contador binario)");
        exit(1);
    }

    if(argv[1] == 1){
        circular();
    }
    else if(argv[1] == 2){
        contBinario();
    }
    else{
        fprintf(stderr, "Modo no valido");
        exit(1);
    }
}


void circular(){
    FILE * file;
    char* leds[100] = ""; 

    int i = 1;
    while(1){
        if (file = open ("/dev/chardev_leds", "r+") == NULL){
            printf("Error al abrir el fichero de dispositivo");
            exit(1);
        }

        sprintf(leds, "%d", i);
        fwrite(leds, sizeof(char), strlen(leds), file);
        printf("Se enciende el led %d", i);

        fclose(file);
        sleep(1);

        i = (i+1)%3;
    }
}


void contBinario(){
    FILE * file;
    char* leds[100] = "000";

    int i = 0;
    while(1){
        if (file = open ("/dev(chardev_leds", "r+") == NULL){
            printf("Error al abrir el fichero de dispositivo");
            exit(1);
        }

        if(i%2 == 1){  //si es impar es que en binario acaba en 1
            leds[2] = '3';
        }

        if(i%4 == 2 || i%4 == 3){  //si se cumple entonces es 2,3,6,7 en binario (los que tienen 1 en la 2ª posicion)
            leds[1] = '2';
        }

        if(i >= 4){  //si es mayor que 4 tienen un 1 en la posicion de la izquierda
            leds[0] = '1';
        }        

        fwrite(leds, sizeof(char), strlen(leds), file);
        printf("Se encienden los leds %s", leds);

        fclose(file);
        sleep(1);

        i = (i+1)%8;
        leds[0] = '0';
        leds[1] = '0';
        leds[2] = '0';
    } 
}