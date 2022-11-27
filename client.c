#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>

int clientSocket;

void sender(){
    char sendMessage[10] = {};
    printf("To Server : ");
    scanf("%s", sendMessage);
    write(clientSocket, sendMessage, (int)strlen(sendMessage));
}

void receiver(){
    char recvMessage[10] = {};
    printf("From Server : ");
    int recvLen = read(clientSocket, recvMessage, sizeof(recvMessage) - 1);
    recvMessage[recvLen] = '\0';
    printf("%s\n", recvMessage);
}

int main()
{
    char ipAddr[16] = "127.0.0.1";
    int portNum = 12345;
    
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
    while(1){
        sender();
        receiver();
    }
    close(clientSocket);
    return 0;
}