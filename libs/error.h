#ifndef ERROR_H
#define ERROR_H

/* Prototipos */
void panic_with_system_message(const char *errorMessage);
void panic_with_user_message(const char *msg, const char *detail);


/*panic() con la diferencia que emite un mensaje al usuario */
void panic_with_user_message(const char *msg, const char *detail) 
{
    fputs(msg, stderr);
    fputs(": ", stderr);
    fputs(detail, stderr);
    fputc('\n', stderr);
    exit(1);
}

void panic_with_system_message(const char *msg) 
{
    perror(msg);
    exit(1);
}

#endif

