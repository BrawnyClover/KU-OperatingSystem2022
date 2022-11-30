#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include "yatch.h"

int clientSocket;
int endFlag = 0;
int mode = 0; // 0 : chatting, 1 : yatch

void *sender(void* arg){
    char sendMessage[50] = {};
    while(1){
        scanf("%s", sendMessage);
        write(clientSocket, sendMessage, (int)strlen(sendMessage));
        if(strcmp(sendMessage, "quit") == 0){
            endFlag = 1;
            break;
        }
    }
}

void *receiver(void* arg){
    char recvMessage[50] = {}; // 점수를 ascii로 받기
    while(1){
        int recvLen = read(clientSocket, recvMessage, sizeof(recvMessage) - 1);
        int uid = recvMessage[recvLen-2] - '0';
        recvMessage[recvLen-2] = '\0';
        if(uid == 0){
            printf("Message from server : ");
        }
        else{
            printf("Message from %d : ", uid);
        }
        printf("%s\n", recvMessage);
    }
}

int main()
{
    char ipAddr[16] = "127.0.0.1";
    int portNum = 1234;

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
    printf("Client initialized, disconnect command is 'quit'.\n");

    while(1){
        if(endFlag == 1){
            printf("Disconnecting...\n");
            close(clientSocket);
            break;
        }
    }
    return 0;
}