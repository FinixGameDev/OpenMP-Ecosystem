ecosystem:
	gcc -fopenmp main.c -o ecosystem 
run:
	./ecosystem 4 input100x100
clean:
	rm -f ecosystem output