#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include "yatch.h"

#define MODE_YATCH 1
#define MODE_CHAT 0

// gameStatus codes
#define FIRST_ROLL 0
#define SELECTION 1
#define SECOND_ROLL 2
#define SCORING 3

int clientSocket;
int endFlag = 0;
int mode = 0; // 0 : chatting, 1 : yatch
int gameStatus = 0;

struct ScoreBoard myBoard;
struct ScoreBoard counterBoard;

void *sender(void* arg){
    char sendMessage[50] = {};
    while(1){
        // scanf("%[^\n]s", sendMessage);
        gets(sendMessage);
        if(mode == MODE_YATCH){
            if(strcmp(sendMessage, "help") == 0){
                print_help();
                continue;
            }
        }

        write(clientSocket, sendMessage, (int)strlen(sendMessage));
        if(strcmp(sendMessage, "quit") == 0){
            endFlag = 1;
            break;
        }
        if(strcmp(sendMessage, "yatch") == 0){
            mode = 1;
        }
        bzero(sendMessage, sizeof(sendMessage));
    }
}

void *receiver(void* arg){
    char recvMessage[50] = {};
    while(1){
        int recvLen = read(clientSocket, recvMessage, sizeof(recvMessage));
        int uid = recvMessage[recvLen-1] - '0';
        recvMessage[recvLen-1] = '\0';

        if(uid<0 || uid > 10){
            endFlag = 1;
            break;
        }
        if((int)strlen(recvMessage) == 0){
            
            printf("ERROR : Server side error\n");
            endFlag = 1;
            break;
        }

        if(uid == 0){
            printf("Message from server : ");
        }
        else if(mode == MODE_YATCH){
            printf("Message from player %d : ", uid);
        }
        else{
            printf("Message from %d : ", uid);
        }
        if(strcmp(recvMessage, "quit")==0){
            printf("Client %d has left channel.\n", uid);
        }
        else{
            printf("%s\n", recvMessage);
        }

        bzero(recvMessage, sizeof(recvMessage));
    }
}

int main()
{
    char ipAddr[16] = "127.0.0.1";
    int portNum = 12345;

    pthread_t senderThread;
    pthread_t receiverThread;
    
    // printf("Input Server IP Address : ");
    // scanf("%s", ipAddr);
    // printf("Input Server Port Number : ");
    // scanf("%d", &portNum);

    struct sockaddr_in servAddr;
    memset( &servAddr, 0, sizeof(servAddr));
    servAddr.sin_family = AF_INET;
    servAddr.sin_addr.s_addr = inet_addr(ipAddr);
    servAddr.sin_port = htons(portNum);

    
    clientSocket = socket(PF_INET, SOCK_STREAM, 0);
    if(clientSocket == -1){
        printf("ERROR : socket()\n");
        return 0;
    }
    int err = connect(clientSocket, (struct sockaddr*)&servAddr, sizeof(servAddr));
    if(err == -1){
        printf("ERROR : connect\n");
        return 0;
    }

    err = pthread_create(&senderThread, NULL, (void *) sender, NULL);
    if(err != 0){
        printf("ERROR : creating sender thread.\n");
        return 0;
    }
    else{
        printf("Initializing client...sender established\n");
    }

    err = pthread_create(&receiverThread, NULL, (void *) receiver, NULL);
    if(err != 0){
        printf("ERROR : creating receiver thread.\n");
        return 0;
    }
    else{
        printf("Initializing client...receiver established\n");
    }
    printf("Client initialized, disconnect command is 'quit'.\n\n\n");

    while(1){
        if(endFlag == 1){
            printf("Disconnecting...\n");
            close(clientSocket);
            break;
        }
    }
    return 0;
}