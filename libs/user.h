#ifndef USER_H
#define USER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>

/*este struc va a definir a los usuarios, tal y cual los manipula el servidor */

typedef struct user 
{
	char name[7];
	pthread_t thread;
	int sock;
	struct user *next;
} user_t;

/*lo que se ha hecho es crear una lista enlazada simple, la cual manipulara el servidor para gestionar a los usuarios */

/*insertar a la lista */
void push(user_t* head, char name[7], pthread_t thread, int sock) 
{
	user_t * current = head;
	while (current->next != NULL) 
	{
		current = current->next;
	}

	current->next = malloc(sizeof(user_t));
	strcpy(current->next->name, name);
	current->next->thread = thread;
	current->next->sock = sock;
	current->next->next = NULL;
}

/*elimina el primer elemento de la lista */
int pop(user_t ** head) 
{
	user_t * next_node = NULL;

	if (*head == NULL) 
	{
		return 0;
	}

	next_node = (*head)->next;
	free(*head);
	*head = next_node;

	return 1;
}

/*permite remover un usuario de la lista en base a su nombre de usuario */
/*es usado para cuando un usuario se desconecta del chat */
int remove_by_name(user_t ** head, char name[7]) 
{
	user_t * current = *head;
	user_t * temp_node = NULL;

	while(1)
	{
		if (current->next == NULL) 
		{
			return 0;
		}
		if ( strncmp((current->next)->name, name, 7) == 0)
		{
			break;
		}
		current = current->next;
	}

	temp_node = current->next;
	current->next = temp_node->next;
	free(temp_node);

	return 1;
}

#endif
