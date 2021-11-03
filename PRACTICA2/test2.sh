#!/bin/bash

rm -r temp
mkdir temp

#a
echo "Copiando ficheros a SF..."
cp ./src/fuseLib.c ./mount-point
cp ./src/myFS.h ./mount-point
echo "Copiando ficheros a temp..."
cp ./src/fuseLib.c ./temp
cp ./src/myFS.h ./temp

#b
./my-fsck virtual-disk

if diff ./src/fuseLib.c ./mount-point/fuseLib.c && diff ./src/myFS.h ./mount-point/myFS.h; then
	echo "Archivos copiados correctamente"
else 
	echo "Error en la copia"
	exit 1	
fi

echo "Truncando fuseLib.c..."
truncate -o -s 1 ./temp/fuseLib.c
truncate -o -s 1 ./mount-point/fuseLib.c 

#c
./my-fsck virtual-disk
if diff ./src/fuseLib.c ./mount-point/fuseLib.c && diff ./src/fuseLib.c ./temp/fuseLib.c; then 
	echo "Hubo algún error al truncar"
 	exit 1
else 
 	echo "Truncamiento correcto"
fi

#d
echo "Copiando nuevo fichero a SF..."
echo "Este fichero es para el test" > file.txt
cp ./file.txt ./mount-point

#e
./my-fsck virtual-disk

if diff ./file.txt ./mount-point/file.txt; then 
	echo "Archivo copiado correctamente"
else 
	echo "Error en la copia"
	exit 1
fi

#f
echo "Truncando myFS.h..."
truncate -o -s 2 ./temp/myFS.h
truncate -o -s 2 ./mount-point/myFS.h


#g
./my-fsck virtual-disk

if diff ./src/fuseLib.c ./mount-point/myFS.h && diff ./src/myFS.h ./temp/myFS.h; then 
	echo "Hubo algún error al truncar"
	exit 1
else 
	echo "Truncamiento correcto"
fi
