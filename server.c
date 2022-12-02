#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>
#include "yatch.h"

#define MODE_CHATTING 0
#define MODE_YATCH 1

// gameStatus codes
#define FIRST_ROLL 0
#define SELECTION 1
#define SECOND_ROLL 2
#define SCORING 3

// roomStatus codes
const int IDLE = 0;
const int WAITING_FOR_PLAYER = 1;
const int IN_GAME = 2;

const int GAME_STAGE_AMOUNT = 12;

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

struct YatchRoom{
    struct ClientInfo * clients[2];
    struct ScoreBoard playerBoard[2];
    struct ScoreBoard temp;
    int roomStatus;
    int gameStatus;
    int stage;
    int turn;

    int check[5];
    int diceResult[5];
};

struct ClientList clientList;
struct YatchRoom yatchRoomList[5];
pthread_mutex_t client_list_mutex = PTHREAD_MUTEX_INITIALIZER;
int closeServer;

int getEmptyRoomNumber();
void writeWithUID(int socket, int uid, char msg[]);

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
                writeWithUID(clientList.clients[i]->socket, uid, msg);
            }
        }
    }
    pthread_mutex_unlock(&client_list_mutex);
}

void writeWithUID(int socket, int uid, char msg[]){
    char uidStr[2];
    char sendMessage[50]={};
    strcpy(sendMessage, msg);
    sprintf(uidStr, "%d", uid);
    strcat(sendMessage, uidStr);
    write(socket, sendMessage, (int)strlen(sendMessage));
    sleep(0.1);
}

void roomBroadcast(int roomNumber, char msg[]){
    for(int i=0; i<2; i++){
        writeWithUID(yatchRoomList[roomNumber].clients[i]->socket, 0, msg);
    }
    sleep(0.1);
}

void *serverProcess(void* arg)
{
    char sendMessage[50] = {};
    char recvMessage[50] = {};
    int closeConnect = 0;
    int mode = 0; // 0 : chatting , 1 : yatch
    int roomNumber = 0;
    int playerId = 0;
    int i;
    int flag = 1;
    int winner = 0;
    struct ClientInfo *client = (struct ClientInfo *)arg;
    bzero(sendMessage, sizeof(sendMessage));
    bzero(recvMessage, sizeof(recvMessage));
    // strcpy(sendMessage, "welcome to chatting server!0");
    // write(client->socket, sendMessage, (int)strlen(sendMessage));
    writeWithUID(client->socket, 0, "welcome to chatting server!");
    sleep(0.5);
    sprintf(sendMessage, "Your client number is %d.", client->uid);
    writeWithUID(client->socket, 0, sendMessage);

    while(1){
        // switch로 yatch mode, general mode 나누기
        if(closeConnect == 1){
            break;
        }

        bzero(sendMessage, sizeof(sendMessage));
        bzero(recvMessage, sizeof(recvMessage));

        read(client->socket, recvMessage, sizeof(recvMessage) - 1);
        printf("From client %d : ", client->uid);
        printf("%s\n", recvMessage);

        if(strcmp(recvMessage, "quit") == 0){
            printf("Client %d has left.\n", client->uid);
            closeConnect = 1;

            if(mode == MODE_YATCH){
                winner = 3-playerId;
                sprintf(sendMessage, "One player resigned, Winner is Player %d", winner);
                roomBroadcast(roomNumber, sendMessage);
                mode = MODE_CHATTING;
                yatchRoomList[roomNumber].roomStatus = IDLE;
            }
        }

        switch(mode){
            case MODE_CHATTING:
                if(strcmp(recvMessage, "yatch") == 0){
                    roomNumber = getEmptyRoomNumber();
                    if(roomNumber == -1){
                        writeWithUID(client->socket, 0, "Room limit exceeded");
                    }
                    else{
                        printf("Client %d has joined yatch room %d.\n", client->uid, roomNumber);
                        broadcast("Yatch Room Created", 0);
                        if(yatchRoomList[roomNumber].roomStatus == WAITING_FOR_PLAYER){
                            printf("Room %d, Player1 is %d.\n", roomNumber,client->uid);
                            playerId = 1;
                            yatchRoomList[roomNumber].clients[0] = client;
                            writeWithUID(client->socket, 0, "You are Player 1");
                            writeWithUID(client->socket, 0, "Waiting for another player...");
                        }
                        else{
                            printf("Room %d, Player2 is %d.\n", roomNumber,client->uid);
                            playerId = 2;
                            yatchRoomList[roomNumber].clients[1] = client;
                            writeWithUID(client->socket, 0, "Joined to a room");
                            writeWithUID(client->socket, 0, "You are Player 2");
                            sleep(1);
                            roomBroadcast(roomNumber, "Start Game!");
                            roomBroadcast(roomNumber, "It's Player 1's turn");
                        }
                        mode = MODE_YATCH;
                        yatchRoomList[roomNumber].turn = 1;
                    }
                }
                else{
                    broadcast(recvMessage, client->uid);
                }
                break;

            case MODE_YATCH:
                if(yatchRoomList[roomNumber].turn == playerId){
                    switch(yatchRoomList[roomNumber].gameStatus){
                        case FIRST_ROLL:
                        
                        srand(time(NULL));
                        if(strcmp(recvMessage, "roll") == 0){
                            for(i=0; i<5; i++){
                                yatchRoomList[roomNumber].diceResult[i] = rand()%5+1;
                                sendMessage[i] = yatchRoomList[roomNumber].diceResult[i]+'0';
                            }
                            roomBroadcast(roomNumber, sendMessage);
                            printf("Dice result : %s\n", sendMessage);

                            roomBroadcast(roomNumber, "Select Numbers");
                            yatchRoomList[roomNumber].gameStatus = SELECTION;
                        }
                        else{
                            writeWithUID(
                                yatchRoomList[roomNumber].clients[1-(playerId-1)]->socket, playerId, recvMessage);
                        }
                        break;
                        
                        case SELECTION:
                        if(
                            recvMessage[0] == 's' &&
                            recvMessage[1] == 'e' &&
                            recvMessage[2] == 'l'
                        ){
                            yatchRoomList[roomNumber].check[0] = recvMessage[4]-'0';
                            yatchRoomList[roomNumber].check[1] = recvMessage[5]-'0';
                            yatchRoomList[roomNumber].check[2] = recvMessage[6]-'0';
                            yatchRoomList[roomNumber].check[3] = recvMessage[7]-'0';
                            yatchRoomList[roomNumber].check[4] = recvMessage[8]-'0';
                            int checksum = 0;
                            for(int i=0; i<5; i++){
                                checksum += yatchRoomList[roomNumber].check[i];
                                if(yatchRoomList[roomNumber].check[i] == 1){
                                    printf("%d checked\n", i);
                                }
                            }
                            if(checksum == 5){
                                yatchRoomList[roomNumber].gameStatus = SCORING;
                                clear_temp(&yatchRoomList[roomNumber].temp);
                                calc_point(&yatchRoomList[roomNumber].temp, yatchRoomList[roomNumber].diceResult);
                                roomBroadcast(roomNumber, "Select Score slot");
                            }
                            else{
                                yatchRoomList[roomNumber].gameStatus = SECOND_ROLL;
                                roomBroadcast(roomNumber, "Roll again");
                            }
                        }
                        else{
                            writeWithUID(
                                yatchRoomList[roomNumber].clients[1-(playerId-1)]->socket, playerId, recvMessage);
                        }
                        break;

                        case SECOND_ROLL:
                        if(strcmp(recvMessage, "roll") == 0){
                            srand(time(NULL));
                            for(i=0; i<5; i++){
                                if(yatchRoomList[roomNumber].check[i] == 0){
                                    
                                    yatchRoomList[roomNumber].diceResult[i] = rand()%5+1;
                                    sendMessage[i] = yatchRoomList[roomNumber].diceResult[i]+'0';
                                    printf("change %d : %d\n", i, sendMessage[i]-'0');
                                }
                                else{
                                    sendMessage[i] = yatchRoomList[roomNumber].diceResult[i] + '0';
                                    printf("not change %d : %d\n", i, sendMessage[i]-'0');
                                }
                            }
                            roomBroadcast(roomNumber, sendMessage);
                            printf("Dice result : %s\n", sendMessage);
                            clear_temp(&yatchRoomList[roomNumber].temp);
                            calc_point(&yatchRoomList[roomNumber].temp, yatchRoomList[roomNumber].diceResult);
                            yatchRoomList[roomNumber].gameStatus = SCORING;
                            roomBroadcast(roomNumber, "Select Score slot");

                        }
                        else{
                            writeWithUID(
                                yatchRoomList[roomNumber].clients[1-(playerId-1)]->socket, playerId, recvMessage);
                        }
                        break;

                        case SCORING:
                        flag = 1;
                        if(strcmp(recvMessage, "ones") == 0){
                            yatchRoomList[roomNumber].playerBoard[playerId - 1].ones = yatchRoomList[roomNumber].temp.ones;
                            sprintf(sendMessage, "Player %d earned %d points for %s", playerId, yatchRoomList[roomNumber].temp.ones, "ones");
                            roomBroadcast(roomNumber, sendMessage);
                        }
                        else if(strcmp(recvMessage, "twos") == 0){
                            yatchRoomList[roomNumber].playerBoard[playerId - 1].twos = yatchRoomList[roomNumber].temp.twos;
                            sprintf(sendMessage, "Player %d earned %d points for %s", playerId, yatchRoomList[roomNumber].temp.twos, "twos");
                            roomBroadcast(roomNumber, sendMessage);
                        }
                        else if(strcmp(recvMessage, "threes") == 0){
                            yatchRoomList[roomNumber].playerBoard[playerId - 1].threes = yatchRoomList[roomNumber].temp.threes;
                            sprintf(sendMessage, "Player %d earned %d points for %s", playerId, yatchRoomList[roomNumber].temp.threes, "threes");
                            roomBroadcast(roomNumber, sendMessage);
                        }
                        else if(strcmp(recvMessage, "fours") == 0){
                            yatchRoomList[roomNumber].playerBoard[playerId - 1].fours = yatchRoomList[roomNumber].temp.fours;
                            sprintf(sendMessage, "Player %d earned %d points for %s", playerId, yatchRoomList[roomNumber].temp.fours, "fours");
                            roomBroadcast(roomNumber, sendMessage);
                        }
                        else if(strcmp(recvMessage, "fives") == 0){
                            yatchRoomList[roomNumber].playerBoard[playerId - 1].fives = yatchRoomList[roomNumber].temp.fives;
                            sprintf(sendMessage, "Player %d earned %d points for %s", playerId, yatchRoomList[roomNumber].temp.fives, "fives");
                            roomBroadcast(roomNumber, sendMessage);
                        }
                        else if(strcmp(recvMessage, "sixes") == 0){
                            yatchRoomList[roomNumber].playerBoard[playerId - 1].sixes = yatchRoomList[roomNumber].temp.sixes;
                            sprintf(sendMessage, "Player %d earned %d points for %s", playerId, yatchRoomList[roomNumber].temp.sixes, "sixes");
                            roomBroadcast(roomNumber, sendMessage);
                        }
                        else if(strcmp(recvMessage, "large straight") == 0){
                            yatchRoomList[roomNumber].playerBoard[playerId - 1].lStraight = yatchRoomList[roomNumber].temp.lStraight;
                            sprintf(sendMessage, "Player %d earned %d points for %s", playerId, yatchRoomList[roomNumber].temp.lStraight, "lStraight");
                            roomBroadcast(roomNumber, sendMessage);
                        }
                        else if(strcmp(recvMessage, "small straight") == 0){
                            yatchRoomList[roomNumber].playerBoard[playerId - 1].sStraight = yatchRoomList[roomNumber].temp.sStraight;
                            sprintf(sendMessage, "Player %d earned %d points for %s", playerId, yatchRoomList[roomNumber].temp.sStraight, "sStraight");
                            roomBroadcast(roomNumber, sendMessage);
                        }
                        else if(strcmp(recvMessage, "four of a kind") == 0){
                            yatchRoomList[roomNumber].playerBoard[playerId - 1].foKind = yatchRoomList[roomNumber].temp.foKind;
                            sprintf(sendMessage, "Player %d earned %d points for %s", playerId, yatchRoomList[roomNumber].temp.foKind, "foKind");
                            roomBroadcast(roomNumber, sendMessage);
                        }
                        else if(strcmp(recvMessage, "full house") == 0){
                            yatchRoomList[roomNumber].playerBoard[playerId - 1].fHouse = yatchRoomList[roomNumber].temp.fHouse;
                            sprintf(sendMessage, "Player %d earned %d points for %s", playerId, yatchRoomList[roomNumber].temp.fHouse, "fHouse");
                            roomBroadcast(roomNumber, sendMessage);
                        }
                        else if(strcmp(recvMessage, "yatch") == 0){
                            yatchRoomList[roomNumber].playerBoard[playerId - 1].yatch = yatchRoomList[roomNumber].temp.yatch;
                            sprintf(sendMessage, "Player %d earned %d points for %s", playerId, yatchRoomList[roomNumber].temp.yatch, "yatch");
                            roomBroadcast(roomNumber, sendMessage);
                        }
                        else{
                            writeWithUID(
                                yatchRoomList[roomNumber].clients[1-(playerId-1)]->socket, playerId, recvMessage);
                            flag = 0;
                        }
                        if(flag == 1){
                            yatchRoomList[roomNumber].gameStatus = FIRST_ROLL;
                            calc_final_point(&yatchRoomList[roomNumber].playerBoard[playerId-1]);
                            print_board(yatchRoomList[roomNumber].playerBoard[0], yatchRoomList[roomNumber].playerBoard[1], yatchRoomList[roomNumber].stage, yatchRoomList[roomNumber].turn);
                            if(yatchRoomList[roomNumber].turn == 2){yatchRoomList[roomNumber].stage++;}
                            yatchRoomList[roomNumber].turn = 3 - yatchRoomList[roomNumber].turn;
                            if(yatchRoomList[roomNumber].stage > GAME_STAGE_AMOUNT){
                                winner = yatchRoomList[roomNumber].playerBoard[0].tot > yatchRoomList[roomNumber].playerBoard[1].tot ? 1:2;
                                sprintf(sendMessage, "Game over, Winner is Player %d", winner);
                                roomBroadcast(roomNumber, sendMessage);
                                mode = MODE_CHATTING;
                                yatchRoomList[roomNumber].roomStatus = IDLE;
                            }
                            else{
                                sprintf(sendMessage, "It's Player %d's turn.", yatchRoomList[roomNumber].turn);
                                roomBroadcast(roomNumber, sendMessage);
                            }
                        }
                        
                        break;
                    }
                }
                else{
                    writeWithUID(
                        yatchRoomList[roomNumber].clients[1-(playerId-1)]->socket, playerId, recvMessage);
                }
                break;
        }
        

        printf("\n");
    }

    close(client->socket);
    removClient(client->uid);
    free(client);
    pthread_detach(pthread_self());
    return NULL;
}

int getEmptyRoomNumber()
{
    int i=0;
    for(i=0; i<5; i++){
        if(yatchRoomList[i].roomStatus == WAITING_FOR_PLAYER){
            yatchRoomList[i].roomStatus = IN_GAME;
            return i;
        }
    }
    for(i=0; i<5; i++){
        if(yatchRoomList[i].roomStatus == IDLE){
            yatchRoomList[i].roomStatus = WAITING_FOR_PLAYER;
            return i;
        }
    }
    return -1;
}

int main()
{
    char ipAddr[16] = "127.0.0.1";
    int portNum = 12345;

    int serverSocket;
    int clientSocket;
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

    int i=0;
    for(i=0; i<5; i++){
        yatchRoomList[i].roomStatus = IDLE;
    }

    serverSocket = socket(PF_INET, SOCK_STREAM, 0);
    int option = 1;
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
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
        if(clientList.count == 0 && closeServer == 1){
            break;
        }
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
    printf("Server closed");
    return 0;
}