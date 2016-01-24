#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include "libs/data.h"
#include "libs/error.h"
#define configfile "config.dat"

/* Prototipos */
int get_client_socket( const char * , const char * );
void LeerParametroHasta(int fd, char separacion, char * param);
void printbanner();
void t_read(void* sock);
void t_write(void* sock);


char user[7];

int main()
{
    int fd_in;
    char server[15];
    char port[15];
    pthread_t read_thread, write_thread;
    
    if ((fd_in = open(configfile, O_RDONLY)) < 0){
        panic_with_system_message("Error no se ha podido abrir el fichero de configuración");
    }
    
    LeerParametroHasta(fd_in,'\n',server);
    LeerParametroHasta(fd_in,'\n',port);
    LeerParametroHasta(fd_in,'\n',user);
    
    int sock = get_client_socket(server, port);
    if (sock < 0)
    {
        panic_with_user_message("Fallo en get_client_socket() ", "No se puede conectar");
    }
    printbanner();//Bienvenida si no han habido errores de arranque
    pthread_create(&read_thread, NULL,(void *) t_read, (void *) (uintptr_t)sock);
    pthread_create(&write_thread, NULL,(void *) t_write, (void *)(uintptr_t) sock);
    
    pthread_join(read_thread, NULL);
    pthread_join(write_thread, NULL);
    
    close(sock);
    exit(0);
}

void t_read(void* sock)
{
    int* sockfd = (int*) &sock;
    ssize_t number_of_bytes;
    char buffer[BUFFER_SIZE];
    
    while(1)
    {
        number_of_bytes = read(*sockfd, buffer, BUFFER_SIZE);
        if (number_of_bytes < 0)
        {
            panic_with_system_message("Fallo en la funcion recv() dentro de un thread");
        }
        else if (number_of_bytes == 0)
        {
            panic_with_user_message("Mensaje:", " Ha salido del chat");
        }
        
        package receive_msn;
        memcpy(&receive_msn, buffer, sizeof(package));
        
        int i;
        for( i = 0; i < (int)strlen(user) + 1; i++){//Añadimos backspace por motivos esteticos
            printf("\b");
        }
        switch(receive_msn.type)
        {
            case 'N':
            {
                printf("%s usuarios conectados:\n\n", receive_msn.data);
                break;
            }
            case 'U':
            {
                printf("   -%s\n\n", receive_msn.data);
                break;
            }
            case 'O':
            {
                printf("%s: %s\n", receive_msn.origin, receive_msn.data);
                break;
            }
            case 'E':
            {
                printf("%s: %s\n", receive_msn.origin, receive_msn.data);
                break;
            }
            case 'R':
            {
                printf("%s: %s\n", receive_msn.origin, receive_msn.data);
                break;
            }
            case 'Q':
            {
                printf("%s:%s\n", receive_msn.origin, receive_msn.data);
                break;
            }
            default:
            printf("%s>%s\n", receive_msn.origin, receive_msn.data);
        }
        printf("%s>", user);
        fflush(stdout);
    }
    pthread_exit(0);
}

void t_write(void* sock)
{
    int* sockfd = (int*) &sock;
    size_t number_of_bytes;
    char buffer[BUFFER_SIZE];
    char *message = (char *)malloc(BUFFER_SIZE);
    int error = 0;
    char destiny[7];
    char data[80];
    
    while(1)
    {
        printf("%s>", user);
        scanf("%s", buffer);
        if (strncasecmp(buffer, "connect", 7) == 0)
        {
            package send_msn = new_package(user, "server", 'C', "CONEXION");
            message = prepare_to_send(send_msn);
        }
        else if (strncasecmp(buffer, "exit", 4) == 0)
        {
            package send_msn = new_package(user, "server", 'Q', "QUIT");
            message = prepare_to_send(send_msn);
        }
        else if (strncasecmp(buffer, "show", 10) == 0)
        {
            scanf("%s", destiny);
            package send_msn = new_package(user, "server", 'L', "PETICION LISTA USUARIOS");
            message = prepare_to_send(send_msn);
        }
        else if (strncasecmp(buffer, "send", 4) == 0)
        {
            scanf("%s", destiny);
            fgets(data, 80, stdin);
            package send_msn = new_package(user, destiny, 'S', data);
            message = prepare_to_send(send_msn);
        }
        else if (strncasecmp(buffer, "broadcast", 9) == 0)
        {
            fgets(data, 80, stdin);
            package send_msn = new_package(user, "server", 'B', data);
            message = prepare_to_send(send_msn);
        }else{
            package send_msn = new_package(user, "server", 'E', "Error. El comando no existe");
            message = prepare_to_send(send_msn);
            printf("Error.El comando no existe.\n");
            error =1;
        }
        number_of_bytes = write(*sockfd, message, BUFFER_SIZE);
        
        /*if (number_of_bytes < 0)
        {
        panic_with_system_message("Error al escribir datos en el servidor.");
        }*/
        
    }
    pthread_exit(0);
}


int get_client_socket(const char *host, const char *service)
{
    struct addrinfo address_criteria;                   // criterio para el matching de direcciones
    memset(&address_criteria, 0, sizeof(address_criteria)); // llena el struct de 0's
    address_criteria.ai_family = AF_UNSPEC;             // ipv4 o ipv6
    address_criteria.ai_socktype = SOCK_STREAM;         // solo sockets con tramas (streams)
    address_criteria.ai_protocol = IPPROTO_TCP;         // unicamente protocolo TCP
    
    // Obtenemos las direcciones
    struct addrinfo *server_address; // Lista de direcciones del servidor
    int return_val = getaddrinfo(host, service, &address_criteria, &server_address);
    if (return_val != 0)
    {
        panic_with_user_message("Fallo en getaddrinfo() dentro de get_client_socket()", gai_strerror(return_val)); //Devuelve un error especifico de <netdb.h>
    }
    
    int sock = -1;
    struct addrinfo *addr;
    for (addr = server_address; addr != NULL; addr = addr->ai_next)
    {
        sock = socket(addr->ai_family, addr->ai_socktype, addr->ai_protocol);
        if (sock < 0)
        {
            continue;  //Error al crear socket. Intentando con siguiente dirección
        }
        
        //Conexión al servidor
        if (connect(sock, addr->ai_addr, addr->ai_addrlen) == 0)
        {
            break;     //Socket creado. Salimos del bucle
        }
        
        close(sock); //Error al crear socket. Intentando con siguiente dirección
        sock = -1;//Si no se ha podido crear el socket retornamos
    }
    
    freeaddrinfo(server_address); // Libera memoria del struct addrinfo
    return sock;
}

void LeerParametroHasta(int fd, char separacion, char * param){ //Dado un file descriptor lee hasta la separacion y lo almacena en param
    int i=0;
    do{
        read(fd, &(param[i]), 1);
        i++;
    }while(param[i-1]!=separacion);
    param[i-1]='\0';
    
}


void printbanner(){
    printf("db      .d8888. d888888b db    db d888888b d888888b \n");
    printf("88      88'  YP `~~88~~' 88    88   `88'   `~~88~~' \n");
    printf("88      `8bo.      88    88    88    88       88    \n");
    printf("88        `Y8b.    88    88    88    88       88    \n");
    printf("88booo. db   8D    88    88b  d88   .88.      88    \n");
    printf("Y88888P `8888Y'    YP    ~Y8888P' Y888888P    YP    \n");
}
