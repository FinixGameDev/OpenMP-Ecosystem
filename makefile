ecosystem:
	gcc -fopenmp main.c -o ecosystem 
run:
	./ecosystem 16 input5x5
clean:
	rm -f ecosystem output