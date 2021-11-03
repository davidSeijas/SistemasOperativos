#!/bin/bash
if ![ -x mytar ]; then
	echo "ERROR: Mi fichero mtar no se encuentra o no se puede ejecutar"
	exit 1
fi

if [ -d tmp ]; then
	rm -r tmp
fi

mkdir tmp
cd tmp

echo "Hello world!" > file1.txt
head -10 /etc/passwd > file2.txt
head -c 1024 /dev/urandom > file3.dat

../mytar -c -f filetar.mtar file1.txt file2.txt file3.dat

mkdir out
cp filetar.mtar ./out

cd out
../../mytar -x -f ./filetar.mtar

if diff file1.txt ../file1.txt && diff file2.txt ../file2.txt && diff file3.dat ../file3.dat ; then
	cd ../..
	echo "Correct"
	exit 0
else 
	echo "ERROR: Los archivos no son iguales"
	exit 1
fi
