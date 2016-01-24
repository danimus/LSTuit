#ifndef DATA_H
#define DATA_H

#include <stdio.h>      
#include <sys/socket.h> 
#include <arpa/inet.h>  
#include <stdlib.h>     
#include <string.h>     
#include <unistd.h>     

/*estos campos representan las caracteristicas de la trama, segun la hoja de especificaciones */
#define NAME_SIZE 7 
#define MESSAGE_SIZE 80
#define BUFFER_SIZE 95

/*y esta es la trama */
typedef struct package
{
	char origin[NAME_SIZE];	
	char destiny[NAME_SIZE]; 
	char type;
	char data[MESSAGE_SIZE];
} package;


/*crea una nueva trama */
package new_package(char origin[NAME_SIZE], char destiny[NAME_SIZE], char type, char data[MESSAGE_SIZE])
{

  package *tmp = (package *)malloc(sizeof(package));
  strcpy(tmp->origin, origin);
  strcpy(tmp->destiny, destiny);
  tmp->type = type;
  strcpy(tmp->data, data);

  return *tmp;
}

/*empaqueta la trama, de forma que quepa en un array con celdas de memoria secuenciales */
char * prepare_to_send(package msn)
{
  char *buffer = (char *)malloc( BUFFER_SIZE * sizeof(char) );
  memcpy(buffer, &msn, sizeof(msn));
  return buffer;
}

#endif

