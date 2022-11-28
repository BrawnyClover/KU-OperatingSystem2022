#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

const int UNINITIALIZED = 0;

struct ClientInfo{
    int uid;
    int socket;
    struct sockaddr_in address;
};

struct ClientList{
    struct ClientInfo *clients[5];
    int count;
    int nextUid;
};

struct ClientList clientList;
pthread_mutex_t client_list_mutex = PTHREAD_MUTEX_INITIALIZER;

void addClient(struct ClientInfo *new){
    pthread_mutex_lock(&client_list_mutex);
    int i=0;
    for(i=0; i<5; i++){
        if(clientList.clients[i] == NULL){
            clientList.clients[i] = new;
            clientList.count++;
            break;
        }
    }
    pthread_mutex_unlock(&client_list_mutex);
}

void removClient(int uid){
    pthread_mutex_lock(&client_list_mutex);
    int i=0;
    for(i=0; i<5; i++){
        if(clientList.clients[i] != NULL){
            if(clientList.clients[i]->uid == uid){
                clientList.clients[i] = NULL;
                clientList.count--;
                break;
            }
        }
    }
    pthread_mutex_unlock(&client_list_mutex);
}

void broadcast(char* msg, int uid){
    pthread_mutex_lock(&client_list_mutex);
    int i=0;
    for(i=0; i<5; i++){
        if(clientList.clients[i] != NULL){
            if(clientList.clients[i]->uid != uid){
                char uidStr[2];
                sprintf(uidStr, "%d", uid);
                strcat(msg, uidStr);

                write(clientList.clients[i]->socket, msg, (int)strlen(msg)+1);
            }
        }
    }
    pthread_mutex_unlock(&client_list_mutex);
}

void *serverProcess(void* arg)
{
    char sendMessage[50] = {};
    char recvMessage[50] = {};
    int closeConnect = 0;
    struct ClientInfo *client = (struct ClientInfo *)arg;
    write(client->socket, "welcome to chatting server!0", (int)strlen("welcome to chatting server!0")+1);

    while(1){
        if(closeConnect == 1){
            break;
        }
        read(client->socket, recvMessage, sizeof(recvMessage) - 1);
        printf("From client %d : ", client->uid);
        printf("%s\n", recvMessage);

        if(strcmp(recvMessage, "quit") == 0){
            printf("Client %d has left.\n", client->uid);
            closeConnect = 1;
        }
        else{
            broadcast(recvMessage, client->uid);
        }
        printf("\n");
        bzero(sendMessage, sizeof(sendMessage));
        bzero(recvMessage, sizeof(recvMessage));
    }

    close(client->socket);
    removClient(client->uid);
    free(client);
    pthread_detach(pthread_self());
    return NULL;
}

int main()
{
    char ipAddr[16] = "127.0.0.1";
    int portNum = 1234;

    int serverSocket;
    int clientSocket = UNINITIALIZED;
    struct sockaddr_in serverAddr, clientAddr;
    int clientAddrSize;
    pthread_t tid;

    clientList.count = 0;
    clientList.nextUid = 1;
    // printf("Input Server IP Address : ");
    // scanf("%s", ipAddr);
    // printf("Input Server Port Number : ");
    // scanf("%d", &portNum);

    struct sockaddr_in servAddr;

    serverSocket = socket(PF_INET, SOCK_STREAM, 0);
    if(serverSocket == -1){
        printf("ERROR : Generating socket\n");
        return 0;
    }

    memset( &servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr(ipAddr);
    servAddr.sin_port = htons(portNum);

    int err = bind(serverSocket, (struct sockaddr *)&servAddr, sizeof(servAddr));
    if(err == -1){
        printf("ERROR : bind\n");
        return 0;
    }
    printf("Server initialized.\n");
    
    err = listen(serverSocket, 5);
    if(err == -1){
        printf("ERROR : listen\n");
        return 0;
    }
    while(1){
        
        clientAddrSize = sizeof(clientAddr);
        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrSize);
        if(clientSocket == -1){
            printf("ERROR : accept\n");
        }
        else{
            if(clientList.count == 5){
                printf("Client count limit exceeded\n");
                close(clientSocket);
                continue;
            }
            else{
                struct ClientInfo *new = (struct ClientInfo *)malloc(sizeof(struct ClientInfo));
                new->uid = clientList.nextUid++;
                new->socket = clientSocket;
                new->address = clientAddr;
                
                printf("Client %d connected to server.\n", new->uid);
                addClient(new);
                pthread_create(&tid, NULL, &serverProcess, (void*)new);
            }
        }
    }
    close(serverSocket);
    return 0;
}