all:
	@mkdir -p bin bin/server bin/client
	@echo "127.0.0.1\n1234\ndani" > bin/client/config.dat
	@gcc -o bin/server/server -Wall -Wextra server.c -lpthread
	@gcc -o bin/client/client -Wall -Wextra client.c -lpthread
	@echo "Se ha compilado el programa correctamente y generado fichero de configuración"
clean:
	rm -rf bin/
	rm -rf server/
	rm -rf client/
