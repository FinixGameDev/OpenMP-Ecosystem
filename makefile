ecosystem:
	gcc -fopenmp main.c -o ecosystem 
run:
	./ecosystem 12 input5x5
clean:
	rm -f ecosystem output.txt