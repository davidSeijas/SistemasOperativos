#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#define N_PARADAS 5 // número de paradas de la ruta
#define EN_RUTA 0 // autobús en ruta
#define EN_PARADA 1 // autobús en la parada
#define MAX_USUARIOS 40 // capacidad del autobús
#define USUARIOS 4 // numero de usuarios
// estado inicial
int estado = EN_RUTA;
int parada_actual = 0; // parada en la que se encuentra el autobus
int n_ocupantes = 0; // ocupantes que tiene el autobús

// personas que desean subir en cada parada
int esperando_parada[N_PARADAS]; //= {0,0,...0};

// personas que desean bajar en cada parada
int esperando_bajar[N_PARADAS]; //= {0,0,...0};

// Otras definiciones globales (comunicación y sincronización)
pthread_mutex_t mtxSubir[N_PARADAS];
pthread_mutex_t mtxBajar[N_PARADAS];
pthread_cond_t suben;
pthread_cond_t bajan;
pthread_cond_t hanSubido;
pthread_cond_t hanBajado;


void Autobus_En_Parada(){
	pthread_mutex_lock(&mtxSubir[parada_actual]);
	pthread_mutex_lock(&mtxBajar[parada_actual]);

	estado = EN_PARADA;
	printf("EL bus está en la parada %d\n", parada_actual);
	
	pthread_cond_broadcast(&bajan); //todo el que quiera bajar podrá cuando le llegue el turno (mtx)
	pthread_cond_broadcast(&suben); //todo el que quiera/quepa bajar podrá cuando le llegue el turno (mtx)
	
	//mientras haya gente esperando en la parada para subir o para bajar
	while(esperando_parada[parada_actual] > 0 || esperando_bajar[parada_actual] > 0){
	
		if(esperando_bajar[parada_actual] > 0 && esperando_parada[parada_actual] == 0)
			pthread_cond_wait(&hanBajado, &mtxBajar[parada_actual]); //suelto mtx y dejo que los pasajeros bajen hasta que no quede ninguno (me devuelve el mtx con el signal)

		else if(esperando_bajar[parada_actual] == 0 && esperando_parada[parada_actual] > 0)
			pthread_cond_wait(&hanSubido, &mtxSubir[parada_actual]); //suelto mtx y dejo que los pasajeros suban hasta que no quede ninguno (me devuelve el mtx con el signal)
		else{
			//si pongo las dos cond_wait, hasta que no hayan subido todos no podrán empezar a bajar los otros
			pthread_mutex_unlock(&mtxSubir[parada_actual]);
			pthread_cond_wait(&hanBajado, &mtxBajar[parada_actual]);
			pthread_mutex_lock(&mtxSubir[parada_actual]);
		}
	}
	
	//ya nadie más quiere bajar y/o subir así que retomo el viaje 

	estado = EN_RUTA;
	printf("El bus sale de la parada %d y está en ruta\n", parada_actual);

	pthread_mutex_unlock(&mtxBajar[parada_actual]); 
	pthread_mutex_unlock(&mtxSubir[parada_actual]);
}


void Conducir_Hasta_Siguiente_Parada(){	
	sleep((random() % 7) + 1); //retardo que simula trayecto a la siguiente parada
	
	for(int i = 0; i < N_PARADAS; ++i){
		pthread_mutex_lock(&mtxSubir[i]);
		pthread_mutex_lock(&mtxBajar[i]);
	}
	parada_actual = (parada_actual + 1) % N_PARADAS;
	for(int i = 0; i < N_PARADAS; ++i){
		pthread_mutex_unlock(&mtxSubir[i]);
		pthread_mutex_unlock(&mtxBajar[i]);
	}
}


void Subir_Autobus(int id_usuario, int origen){ 
	//usuario solicita subir y coge el mutex (o entra en la cola de solicitar el mtx)
	pthread_mutex_lock(&mtxSubir[origen]);
	esperando_parada[origen]++;

	while(estado != EN_PARADA || parada_actual != origen || n_ocupantes >= MAX_USUARIOS){ //si alguien solicita subir cuando el bus está en esa parada puede subir sin problemas (siempre que tenga el mutex)
		pthread_cond_wait(&suben, &mtxSubir[origen]); //hasta que no le dejen subir (señal suben) no podrá subir
	}

	//usuario que tiene el control del mutex se sube y se actualizan las variables correspondientes
	esperando_parada[origen]--;
	n_ocupantes++;

	if(esperando_parada[origen] == 0 || n_ocupantes == MAX_USUARIOS){
		pthread_cond_signal(&hanSubido); //cuando todo el mundo que quería y cupiese haya subido envíamos una señal para que lo sepa el bus
	}

	printf("Usuario %d se ha subido en la parada %d\n", id_usuario, origen);
	pthread_mutex_unlock(&mtxSubir[origen]);
}


void Bajar_Autobus(int id_usuario, int destino){
	//usuario solicita bajar y coge el mutex (o entra en la cola de solicitar el mtx)
	pthread_mutex_lock(&mtxBajar[destino]);
	esperando_bajar[destino]++;

	while(estado != EN_PARADA || parada_actual != destino){ //si alguien solicita bajar cuando el bus está en esa parada puede bajar sin problemas (siempre que tenga el mutex)
		pthread_cond_wait(&bajan, &mtxBajar[destino]); //hasta que no le dejen bajar no puede
	}

	//usuario que tiene el control del mutex se baja y se actualizan las variables correspondientes
	esperando_bajar[destino]--; 
	n_ocupantes--;

	if(esperando_bajar[destino] == 0){ 
		pthread_cond_signal(&hanBajado); //cuando todo el mundo que quería haya bajado envíamos una señal para que lo sepa el bus
	}

	printf("Usuario %d se ha bajado en la parada %d\n", id_usuario, destino);
	pthread_mutex_unlock(&mtxBajar[destino]);
}


void Usuario(int id_usuario, int origen, int destino) {
	Subir_Autobus(id_usuario, origen); // Esperar a que el autobus esté en parada origen para subir
	Bajar_Autobus(id_usuario, destino); // Bajarme en estación destino
    sleep((random() % 10) + 1); //Retardo hasta que quiera hacer otro viaje
}


void * thread_autobus(void * args) {
	while (1) {
		Autobus_En_Parada(); // esperar a que los viajeros suban y bajen
		Conducir_Hasta_Siguiente_Parada(); // conducir hasta siguiente parada
	}
}


void * thread_usuario(void * arg) {
	int id_usuario, a, b;
	id_usuario = (int) arg;

	while (1) {
		a = rand() % N_PARADAS;
		do{
			b = rand() % N_PARADAS;
		} while(a == b);

		printf("Usuario %d se quiere subir al autobús en la parada %d y bajar en la parada %d\n", id_usuario, a, b);
		Usuario(id_usuario, a, b);
	}
}


int main(int argc, char *argv[]) {
	int i;
	// Definición de variables locales a main
	// Opcional: obtener de los argumentos del programa la capacidad del
	// autobus, el numero de usuarios y el numero de paradas
	pthread_t usuario[USUARIOS];
	pthread_t autobus;

	for(int i = 0; i < N_PARADAS; ++i){
		pthread_mutex_init(&mtxSubir[i], NULL);
		pthread_mutex_init(&mtxBajar[i], NULL);
	}
	pthread_cond_init(&suben, NULL);
	pthread_cond_init(&bajan, NULL);
	pthread_cond_init(&hanSubido, NULL);
	pthread_cond_init(&hanBajado, NULL);

	// Crear el thread Autobus
    pthread_create(&autobus, NULL, thread_autobus, NULL);

	// Crear thread para el usuario i
	for (i = 0; i < USUARIOS; ++i){
		pthread_create(&usuario[i], NULL, thread_usuario, (void *) i);
	}

	// Esperar terminación de los hilos
	pthread_join(autobus, NULL);
	for (i = 0; i < USUARIOS; ++i){
		pthread_join(usuario[i], NULL);
	}

	for(int i = 0; i < N_PARADAS; ++i){
		pthread_mutex_destroy(&mtxSubir[i]);
		pthread_mutex_destroy(&mtxBajar[i]);
	}
	pthread_cond_destroy(&suben);
	pthread_cond_destroy(&bajan);
	pthread_cond_destroy(&hanSubido);
	pthread_cond_destroy(&hanBajado);

	return 0;
}