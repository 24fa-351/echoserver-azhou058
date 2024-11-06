s: compile run clean

compile: echo.c 
	gcc -pthread -o echo echo.c

run: 
	./echo -p 8080

clean:
	rm echo