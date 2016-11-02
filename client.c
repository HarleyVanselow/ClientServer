#include <sys/types.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/queue.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>

int client_socket; 

void * check_connection(unsigned char * buf){
    if (buf[0] == 207 && buf[1] == 167){
        printf("Connection Established!\n");
        fflush(stdout); 
    } else {
        printf("Connection closed by server\n");
        fflush(stdout); 
        exit(-1);
    }
}

void * read_user_input(void * client_socket){

    char message[255];

    while(1){

        // This is blocking -> should make this non-blocking
        fgets(message, 255, stdin);
        char message_to_send[256];
        snprintf(message_to_send, sizeof message_to_send, "%d%s", htons(strlen(message)), message);

        printf("Sending: %s", message_to_send); fflush(stdout);
        send(*((int *)client_socket),message_to_send,256,0);//Shouldnt really be 256
        memset(message_to_send,0,256);
    }
}

void close_client(int sig){
    printf("Closing client\n");fflush(stdout);
    close(client_socket);
    shutdown(client_socket, 2);
    exit(1);
}

void * send_username(void * client_socket, const char * username){
    char message_to_send[256];
    snprintf(message_to_send, sizeof message_to_send, "%d%s", strlen(username), username);
    printf("Your Username: %s\n",message_to_send );
        send(*((int *)client_socket),message_to_send,256,0);//Shouldnt really be 256
    }

int main(int argc, char const *argv[]){
    if (argc != 4){
        printf("Invalid parameters!\n");
        return -1;
    }

    const char * hostname = argv[1];
    int port_number = atoi(argv[2]);
    const char * username = argv[3];


    uint32_t number;

    unsigned char buf[256];

    struct  sockaddr_in server;

    struct  hostent     *host;

    TAILQ_HEAD(tailhead, entry) head;
    struct tailhead *headp;                
    struct entry {
        TAILQ_ENTRY(entry) entries;        
    } *n1, *n2, *np;

    TAILQ_INIT(&head);    


    host = gethostbyname (hostname);

    if (host == NULL) {
        perror ("Client: cannot get host description");
        exit (1);
    }

    struct sigaction onSigInt;
    onSigInt.sa_handler = close_client;
    sigemptyset(&onSigInt.sa_mask);
    onSigInt.sa_flags = 1;

    sigaction(SIGINT, &onSigInt, NULL);


    client_socket = socket (AF_INET, SOCK_STREAM, 0);

    if (client_socket < 0) {
        perror ("Client: cannot open socket");
        exit (1);
    }

    bzero (&server, sizeof (server));
    bcopy (host->h_addr, & (server.sin_addr), host->h_length);
    server.sin_family = host->h_addrtype;
    server.sin_port = htons (port_number);

    if (connect (client_socket, (struct sockaddr*) & server, sizeof (server))) {
        perror ("Client: cannot connect to server");
        exit (1);
    }

    read (client_socket, &buf, 2);
    check_connection(buf);
    uint16_t number_of_connected_users;
    read (client_socket, &number_of_connected_users, 2);
    printf("There are already %d users connected!\n", number_of_connected_users);
    fflush(stdout);
    // Now get the list of usernames 
    int counter = 0;
    for (counter; counter < number_of_connected_users; counter++){
        read(client_socket, &buf, 1);
        int username_length = buf[0];
        char current_username[username_length];
        read(client_socket, &current_username, username_length);
        current_username[username_length] = '\0';
        printf("%s\n", current_username); fflush(stdout);
    } 
    
    // send username
    send_username(&client_socket, username);

    //thread for handling user input
    pthread_t user_input;
    pthread_create(&user_input, NULL, read_user_input, &client_socket);

    int message_type;
    int username_length;
    uint16_t message_length;

    while (1){

        read (client_socket, &message_type, 1);
        if (message_type == 0) {

            read(client_socket, &username_length, 1);
            char sender_username[username_length];
            read(client_socket, &sender_username, username_length);
            sender_username[username_length] = '\0';
            
            read(client_socket, &message_length, 2);
            char received_message[message_length];
            read(client_socket, &received_message, message_length);
            received_message[message_length] = '\0';

            printf("%s: %s\n", sender_username, received_message);
            fflush(stdout);
        } else if (message_type == 1) {
            read(client_socket, &username_length, 1);
            char updated_username[username_length];
            read(client_socket, &updated_username, username_length);
            updated_username[username_length] = '\0';
            printf("%s has joined the chat\n", updated_username);
            fflush(stdout);
        } else if (message_type == 2) {
            read(client_socket, &username_length, 1);
            char updated_username[username_length];
            read(client_socket, &updated_username, username_length);
            updated_username[username_length] = '\0';
            printf("%s has left the chat\n", updated_username);
            fflush(stdout);
        }
    }
}