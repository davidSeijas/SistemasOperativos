#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "mytar.h"

extern char *use;

/** Copy nBytes bytes from the origin file to the destination file.
 *
 * origin: pointer to the FILE descriptor associated with the origin file
 * destination:  pointer to the FILE descriptor associated with the destination file
 * nBytes: number of bytes to copy
 *
 * Returns the number of bytes actually copied or -1 if an error occured.
 */



int copynFile(FILE * origin, FILE * destination, int nBytes)
{	
	int c;
	int d;
	long int bytes = 0;

	while(bytes < nBytes && (c = getc(origin)) != EOF){
		d = putc(c, destination);
		if(d == EOF || c != d){
			return -1;
		}
		bytes++;
	}

	return bytes;
}

/** Loads a string from a file.
 *
 * file: pointer to the FILE descriptor 
 * 
 * The loadstr() function must allocate memory from the heap to store 
 * the contents of the string read from the FILE. 
 * Once the string has been properly built in memory, the function returns
 * the starting address of the string (pointer returned by malloc()) 
 * 
 * Returns: !=NULL if success, NULL if error
 */
char* loadstr(FILE *file)
{
	int n = 0;
	char aux = getc(file);
	while (aux != '\0'){ //leemos el string hasta el final para saber la longitud
		if(aux == EOF){
			return NULL;
		}

		aux = getc(file);
		++n;
	}
	char* str = malloc(n + 1);

	if(fseek(file, -n-1, SEEK_CUR) != 0){ //volvemos al incio para leer el nombre del file
		return NULL;
	}

	if(fread(str, sizeof(char), n + 1, file) != n + 1){ 
		return NULL;
	}
	
	return str;
}

/** Read tarball header and store it in memory.
 *
 * tarFile: pointer to the tarball's FILE descriptor 
 * nFiles: output parameter. Used to return the number 
 * of files stored in the tarball archive (first 4 bytes of the header).
 *
 * On success it returns the starting memory address of an array that stores 
 * the (name,size) pairs read from the tar file. Upon failure, the function returns NULL.
 */
stHeaderEntry* readHeader(FILE * tarFile, int *nFiles)
{ 
	stHeaderEntry* array = NULL;
	int nr_files = 0;
	
	if(fread(&nr_files, sizeof(nr_files), 1, tarFile) != 1){ //vemos que lee un numero
		return NULL;
	};
	array = malloc(sizeof(stHeaderEntry)*nr_files);

	char* auxR;
	unsigned int auxT;
	for(int i = 0; i < nr_files; ++i){
		auxR = loadstr(tarFile);
		if(auxR == NULL){
			return NULL;
		}
		fread(&auxT, sizeof(unsigned int), 1, tarFile);

		array[i].name = auxR;
		array[i].size =  auxT;
	}

	(*nFiles) = nr_files;
	
	return array;
}

/** Creates a tarball archive 
 *
 * nfiles: number of files to be stored in the tarball
 * filenames: array with the path names of the files to be included in the tarball
 * tarname: name of the tarball archive
 * 
 * On success, it returns EXIT_SUCCESS; upon error it returns EXIT_FAILURE. 
 * (macros defined in stdlib.h).
 *
 * HINTS: First reserve room in the file to store the tarball header.
 * Move the file's position indicator to the data section (skip the header)
 * and dump the contents of the source files (one by one) in the tarball archive. 
 * At the same time, build the representation of the tarball header in memory.
 * Finally, rewind the file's position indicator, write the number of files as well as 
 * the (file name,file size) pairs in the tar archive.
 *
 * Important reminder: to calculate the room needed for the header, a simple sizeof 
 * of stHeaderEntry will not work. Bear in mind that, on disk, file names found in (name,size) 
 * pairs occupy strlen(name)+1 bytes.
 *
 */
int createTar(int nFiles, char *fileNames[], char tarName[])
{
	//Abrimos el tar, dejamos espacio para la cabecera y reservamos espacio en el array

	FILE* tarFile = fopen(tarName, "w");
	if(tarFile == NULL){
		return EXIT_FAILURE;
	}

	unsigned int sizeH = 0;
	sizeH += sizeof(int);
	for(int i = 0; i < nFiles; ++i){
		sizeH += strlen(fileNames[i]) + 1;
		sizeH += sizeof(unsigned int);
	}

	if(fseek(tarFile, sizeH, SEEK_SET) != 0){
		return EXIT_FAILURE;
	}

	stHeaderEntry* array = malloc(sizeof(stHeaderEntry)*nFiles); //cabecera // malloc(space) ??


	//Copiamos los inputFiles en tarFile y guardamos la info de estos en el strHeaderEntry array

	for(int i = 0; i < nFiles; ++i){
		FILE* inputFile = fopen(fileNames[i], "r");
		if (inputFile == NULL){
			return EXIT_FAILURE;
		}

		array[i].name = (char *) malloc(strlen(fileNames[i]) + 1);
		strcpy(array[i].name, fileNames[i]);

		array[i].size = copynFile(inputFile, tarFile, INT_MAX); //escribe todo el input en el tar y devuelve los bytes escritos
		if(array[i].size == -1){
			return EXIT_FAILURE;
		}
		
		fclose(inputFile);
	}

	//Volvemos al principio del tar y escribimos la información de los archivos

	rewind(tarFile);

	fwrite(&nFiles, sizeof(int), 1, tarFile);
	for(int i = 0; i < nFiles; ++i){ //si concatenamos antes el 0 al array.name no sumamos 1 ??
		fwrite(array[i].name, sizeof(char), strlen(array[i].name) + 1, tarFile);
		fwrite(&array[i].size, sizeof(unsigned int), 1, tarFile);
	}

	free(array);
	fclose(tarFile);

	printf("Fichero mitar creado con exito\n");

	return EXIT_SUCCESS;
}

/** Extract files stored in a tarball archive
 *
 * tarName: tarball's pathname
 *
 * On success, it returns EXIT_SUCCESS; upon error it returns EXIT_FAILURE. 
 * (macros defined in stdlib.h).
 *
 * HINTS: First load the tarball's header into memory.
 * After reading the header, the file position indicator will be located at the 
 * tarball's data section. By using information from the 
 * header --number of files and (file name, file size) pairs--, extract files 
 * stored in the data section of the tarball.
 *
 */
int extractTar(char tarName[])
{
	//Abrimos el tar y guardamos en array la info de los archivos

	FILE* tarFile = fopen(tarName, "r");
	if(tarFile == NULL){
		return EXIT_FAILURE;
	}

	int nFiles = 0;
	stHeaderEntry* array = readHeader(tarFile, &nFiles); //cabecera


	//Abrimos los archivos destino y copiamos los datos correspondientes del tar

	for(int i = 0; i < nFiles; ++i){
		printf("[%d]: Creando fichero %s, tamano %d Bytes...\n", i, array[i].name, array[i].size);

		FILE * outPut = fopen(array[i].name, "w");
		if(outPut == NULL){
			return EXIT_FAILURE;
		}

		int n = copynFile(tarFile, outPut, array[i].size);
		if(n != array[i].size){
			return EXIT_FAILURE;
		}

		fclose(outPut);
	}

	free(array);
	fclose(tarFile);

	return EXIT_FAILURE;
}
