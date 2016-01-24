#include <pthread.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "libs/data.h"
#include "libs/user.h"
#include "libs/error.h"
#include "libs/logger.h"
/* Cantidad máxima de conexiones pendientes */
#define MAXPENDING 100

/* Prototipos */
void checkstartparams(int argc);
void *thread_main(void *arg);
void handle_client( int client_socket, pthread_t thread_id);
int accept_connection(int server_sock);
int create_server_socket(unsigned short port);
int verifypackage(package *receive_msn);
void printbanner();


/* Struct independiente para cada thread, va a almacenar su socket local, de forma que cada thread pueda
manejar un socket independiente */
struct thread_args
{
    int client_sock;
};

Logger *l;

/* Lista enlazada de usuarios conectados */
user_t * users = NULL;
pthread_mutex_t lock;

int main(int argc, char *argv[])
{
    
    int server_sock;//Socket para el servidor
    int client_sock;//Socket para el cliente
    unsigned short server_port;//Puerto del servidor
    pthread_t thread_id;//Id del thread
    struct thread_args *thread_args; //Estructura para enviar al thread con el fd de socket
    //Pedimos memoria para la lista de usuarios
    users = malloc(sizeof(user_t));
    users->next = NULL;
    
    l = Logger_create();
    l->level = LOG_DEBUG;
    
    checkstartparams(argc);//Comprobamos que se haya pasado el puerto
    printbanner();//Bienvenida :)
    server_port = atoi(argv[1]);//Obtenemos el valor del puerto
    server_sock = create_server_socket(server_port);
    
    while(1)
    {
        client_sock = accept_connection(server_sock);
        
        //Reserva memoria para el parametro (socket independiente) de cada thread
        if ((thread_args = (struct thread_args *) malloc(sizeof(struct thread_args))) == NULL)
        {
            panic_with_system_message("Fallo en malloc() al pedir memoria para el socket del thread");
        }
        thread_args -> client_sock = client_sock;
        
        //Pthread para el cliente
        if (pthread_create(&thread_id, NULL, thread_main, (void *) thread_args) != 0)
        {
            panic_with_system_message("Fallo en pthread_create() a la hora de crear el thread ");
        }
        printf("con el thread %ld\n", (long int) thread_id);
    }
}

/*la funcion que ejecutara cada thread */
void *thread_main(void *thread_args)
{
    int client_sock;
    pthread_t thread_id = pthread_self(); //obtiene su propio id
    
    pthread_detach(pthread_self());
    
    client_sock = ((struct thread_args *) thread_args) -> client_sock;
    free(thread_args);
    
    handle_client(client_sock, thread_id);
    
    return (NULL);
}


void handle_client(int client_socket, pthread_t thread_id)
{
    char message_buffer[BUFFER_SIZE];
    int received_message_size;
    int registered = 0;
    int running = 1;
    char *message = (char *)malloc(BUFFER_SIZE);
    
    
    received_message_size = read(client_socket, &message_buffer, sizeof(package));
    
    while (received_message_size > 0)      /* Mientras se reciban datos */
    {
        package receive_msn;
        memcpy(&receive_msn, message_buffer, sizeof(package));
        log_debug(l, "=> Paquete Recibido: Origen [%s] Destino [%s] Tipo [%c] Contenido [%s]",receive_msn.origin,receive_msn.destiny,receive_msn.type,receive_msn.data);
        if (verifypackage(&receive_msn)>0){
            log_error(l,"Error. Paquete recibido inválido");
        }
        switch( receive_msn.type )
        {
            case 'C': //connect
            if ( registered == 0 )//Si no se ha añadido anteriormente un usuario
            {
                pthread_mutex_lock(&lock);
                push(users, receive_msn.origin, thread_id, client_socket);
                pthread_mutex_unlock(&lock);
                package send_msn = new_package("server", receive_msn.origin, 'O', "Conexion OK");
                message = prepare_to_send(send_msn);
                write(client_socket, message, received_message_size);
                registered = 1;
            }
            break; //show users
            case 'L':
            {
                int num_users = 0;
                char num_user_c[3];
                user_t * current = users->next;
                while (current != NULL) { current = current->next; num_users++; }
                
                sprintf(num_user_c, "%d", num_users);
                
                package send_msn = new_package("server", receive_msn.origin, 'N', num_user_c);
                message = prepare_to_send(send_msn);
                write(client_socket, message, received_message_size);
                
                current = users->next;
                
                while (current != NULL)
                {
                    package send_msn = new_package("server", receive_msn.origin,
                    'U', current->name);
                    message = prepare_to_send(send_msn);
                    write(client_socket, message, received_message_size);
                    current = current->next;
                }
                
                registered = 1;
                break;
            }
            case 'S': //send user message
            {
                int success = 0;
                user_t * current = users->next;
                while (current != NULL) {
                    if ( strncmp(current->name, receive_msn.destiny, 7) == 0 )
                    {
                        package send_msn = new_package(receive_msn.origin, receive_msn.destiny,
                        'R', receive_msn.data);
                        message = prepare_to_send(send_msn);
                        write(current->sock, message, received_message_size);
                        
                        success = 1;
                        break;
                    }
                    current = current->next;
                }
                if (success)
                {
                    package send_msn = new_package("server", receive_msn.origin, 'O', "SEND OK");
                    message = prepare_to_send(send_msn);
                    write(client_socket, message, received_message_size);
                }
                else
                {
                    package send_msn = new_package("server", receive_msn.origin, 'E', "ERROR: usuario inexistente");
                    message = prepare_to_send(send_msn);
                    write(client_socket, message, received_message_size);
                }
                break;
            }
            case 'B': //broadcast
            {
                user_t * current = users->next;
                while (current != NULL) {
                    package send_msn = new_package(receive_msn.origin, current->name,
                    'Q', receive_msn.data);
                    message = prepare_to_send(send_msn);
                    write(current->sock, message, received_message_size);
                    current = current->next;
                }
                break;
            }
            case 'Q': //exit
            {
                remove_by_name(&users, receive_msn.origin);
                running = 0;
                break;
            }
            default:
            break;
        }
        
        if (!running ){//Si el usuario ha hecho exit salimos del bucle
            break;
        }
        
        received_message_size = read(client_socket, &message_buffer, sizeof(package));
        if((received_message_size<0)){ //Si no se han leido datos del cliente
            panic_with_system_message("Error en la transmision de datos!");
            
        }
        
    }
    
    close(client_socket);
}


int accept_connection(int server_sock)
{
    int client_sock;
    struct sockaddr_in client_address; /* direccion del cliente */
    unsigned int client_length;            /* tamano del struct de la direccion del cliente */
    
    client_length = sizeof(client_address);
    
    /* espera a que el cliente se conecte */
    if ((client_sock = accept(server_sock, (struct sockaddr *) &client_address, &client_length)) < 0)
    {
        panic_with_system_message("Fallo en la funcion accept()");
    }
    
    /* Cliente se ha conectado */
    printf("Handling client %s\n", inet_ntoa(client_address.sin_addr));
    
    return client_sock;
}


int create_server_socket(unsigned short port)
{
    int sock;
    struct sockaddr_in server_address; /* la direccion ip local */
    
    /* socket para las conexiones entrantes */
    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
        panic_with_system_message("socket() failed");
    }
    
    /* construye la direccion local */
    memset(&server_address, 0, sizeof(server_address)); //Rellena el struct con 0
    server_address.sin_family = AF_INET;//Address family
    server_address.sin_addr.s_addr = htonl(INADDR_ANY); //Escucha en todas las interfaces
    server_address.sin_port = htons(port);//Puerto de escucha
    
    
    if (bind(sock, (struct sockaddr *) &server_address, sizeof(server_address)) < 0)
    {
        panic_with_system_message("Fallo en la funcion bind() dentro de create_server_socket()");
    }
    
    if (listen(sock, MAXPENDING) < 0)//Comprueba que no hayan mas peticiones por aceptar del maximo
    {
        panic_with_system_message("Fallo en la funcion listen() dentro de create_server_socket()");
    }
    
    return sock;
}

void checkstartparams(int argc){
    //Comprueba si se han pasado los parametros necesarios
    
    if (argc < 2)
    {
        panic_with_system_message("Faltan parámetros necesarios. Uso: ./server puerto");
    }
}

int verifypackage(package *receive_msn){
    const char *msgtypes = "CQBLNUSEOR";
    int error = 0;
    //Comprobar tipo de mensaje
    if (strchr(msgtypes, receive_msn->type)==NULL){//Retorna la primera aparicion de un caracter en una cadena
        error = 1;
        log_info(l,"Tipo no encontrado\n");
    }
    //Comprobamos origen
    else if (!receive_msn->origin){
        error = 1;
        log_info(l,"Campo origen vacío\n");
    }
    //Comprobamos destino
    else if (!receive_msn->destiny){
        error = 1;
        log_info(l,"Campo destino vacío\n");
        
    }
    //Comprobamos tipo
    else if (!receive_msn->type){
        error = 1;
        log_info(l,"Campo tipo vacío\n");
        
    }
    //Comprobamos que hayan datos
    else if (!receive_msn->data){
        error = 1;
        log_info(l,"Datos del mensaje vacíos\n");
        
    }
    
    return error;
}

void printbanner(){
    printf("db      .d8888. .d8888. d88888b d8888b. db    db d88888b d8888b. \n");
    printf("88      88'  YP 88'  YP 88'     88  `8D 88    88 88'     88  `8D \n");
    printf("88      `8bo.   `8bo.   88ooooo 88oobY' Y8    8P 88ooooo 88oobY' \n");
    printf("88        `Y8b.   `Y8b. 88~~~~~ 88`8b   `8b  d8' 88~~~~~ 88`8b   \n");
    printf("88booo. db   8D db   8D 88.     88 `88.  `8bd8'  88.     88 `88. \n");
    printf("Y88888P `8888Y' `8888Y' Y88888P 88   YD    YP    Y88888P 88   YD \n");
    printf("\n\tDani Álvarez Welters - LS31045\n");
}

