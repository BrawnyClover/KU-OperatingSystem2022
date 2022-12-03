#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include <time.h>
#include "yatch.h"

#define MODE_CHATTING 0             // 채팅 모드를 나타내는 상수
#define MODE_YATCH 1                // Yatch 모드를 나타내는 상수

// 게임 진행도를 나타내는 상수
#define FIRST_ROLL 0                // 첫 번째 주사위 굴리는 단계
#define SELECTION 1                 // 고정할 주사위 선택하는 단계
#define SECOND_ROLL 2               // 두 번째 주사위 굴리는 단계
#define SCORING 3                   // 점수 넣을 공간을 선택하는 단계

// 게임 방의 상태를 나타내는 상수
const int IDLE = 0;                 // 빈 방
const int WAITING_FOR_PLAYER = 1;   // 한 사람이 입장해 다른 플레이어의 입장을 기다리는 상태
const int IN_GAME = 2;              // 게임 진행 중인 상태

const int GAME_STAGE_AMOUNT = 12;   // 한 게임에 진행할 스테이지 수, 기본 12

/*
 * 구조체 이름 : ClientInfo
 * 구조체 역할 : client의 정보를 저장하는 구조체
 * uid : client 고유 번호
 * socket : client 소켓 번호
 * address : client 소켓 정보
*/
struct ClientInfo{
    int uid;
    int socket;
    struct sockaddr_in address;
};

/*
 * 구조체 이름 : ClientList
 * 구조체 역할 : client 목록을 저장하는 구조체
*/
struct ClientList{
    struct ClientInfo *clients[5];  // client 목록을 저장할 배열
    int count;                      // client 수
    int nextUid;                    // 다음 client가 배정받을 번호
};

/*
 * 구조체 이름 : YatchRoom
 * 구조체 역할 : Yatch를 진행하는 방의 정보를 저장하는 구조체
*/
struct YatchRoom{
    struct ClientInfo * clients[2];     // Yatch에 참여한 client 배열
    struct ScoreBoard playerBoard[2];   // 각 Player의 점수판
    struct ScoreBoard temp;             // 이번 stage에 사용된 임시 점수판
    int roomStatus;                     // 현재 방 상태
    int gameStatus;                     // 현재 게임 진행 단계
    int stage;                          // 현재 게임 진행 stage
    int turn;                           // 현재 차례인 플레이어의 playerId

    int check[5];                       // 현재 플레이어가 고정한 주사위
    int diceResult[5];                  // 현재 플레이가 던진 주사위 결과
};

struct ClientList clientList;           // ClientList 인스턴스
struct YatchRoom yatchRoomList[5];      // 최대 방 5개
pthread_mutex_t client_list_mutex = PTHREAD_MUTEX_INITIALIZER;  // clientList에 대한 상호 배제에 필요한 mutex
int closeServer;                        // 서버 종료 flag

int getEmptyRoomNumber();               // 빈 방을 배정받는 함수
void writeWithUID(int socket, int uid, char msg[]); // 프로토콜에 맞게 write하는 함수


/*
 * 함수 이름 : addClient
 * 함수 역할 : clientList에 새로운 client 인스턴스를 추가한다
*/
void addClient(struct ClientInfo *new){
    pthread_mutex_lock(&client_list_mutex); // Critical section 진입
    int i=0;
    for(i=0; i<5; i++){
        if(clientList.clients[i] == NULL){  // clientList의 빈 슬롯에 신규 클라이언트 추가
            clientList.clients[i] = new;
            clientList.count++;
            break;
        }
    }
    pthread_mutex_unlock(&client_list_mutex);   // Critical section 종료
}

/*
 * 함수 이름 : removClient
 * 함수 역할 : clientList에서 특정 uid를 갖는 client를 제거한다
*/
void removClient(int uid){
    pthread_mutex_lock(&client_list_mutex);         // Critical section 진입
    int i=0;
    for(i=0; i<5; i++){
        if(clientList.clients[i] != NULL){
            if(clientList.clients[i]->uid == uid){  // uid가 일치하면
                clientList.clients[i] = NULL;       // Null로 치환
                clientList.count--;                 // client 수 감소
                break;
            }
        }
    }
    pthread_mutex_unlock(&client_list_mutex);       // critical section 종료
}

/*
 * 함수 이름 : broadcast
 * 함수 역할 : 현재 서버에 접속한 모든 클라이언트에게 broadcast한다
*/
void broadcast(char* msg, int uid){
    pthread_mutex_lock(&client_list_mutex);         // Critical section 진입
    int i=0;
    for(i=0; i<5; i++){
        if(clientList.clients[i] != NULL){
            if(clientList.clients[i]->uid != uid){  // 메세지를 보낸 client가 아닌 다른 client에 대해
                writeWithUID(
                    clientList.clients[i]->socket,  // 메세지 전달
                    uid, msg
                    );
            }
        }
    }
    pthread_mutex_unlock(&client_list_mutex);       // critical section 종료
}

/*
 * 함수 이름 : writeWithUID
 * 함수 역할 : 프로토콜에 맞게 write한다
 * 프로토콜 : 메세지의 맨 끝부분에 메세지를 송신한 쪽의 uid를 붙임
 * 0은 서버
*/
void writeWithUID(int socket, int uid, char msg[]){
    char uidStr[2];
    char sendMessage[50]={};
    strcpy(sendMessage, msg);                               
    sprintf(uidStr, "%d", uid);                             // uid를 문자로 치환
    strcat(sendMessage, uidStr);                            // 메세지 끝에 uid 붙임
    write(socket, sendMessage, (int)strlen(sendMessage));   // 전송
    sleep(0.1);                                             // 전송이 겹치지 않도록 delay 설정
}

/*
 * 함수 이름 : roomBroadcast
 * 함수 역할 : 같은 Yatch 방 안에 있는 client에게만 broadcast
*/
void roomBroadcast(int roomNumber, char msg[]){
    for(int i=0; i<2; i++){
        writeWithUID(yatchRoomList[roomNumber].clients[i]->socket, 0, msg);
    }
    sleep(0.1);
}


/*
 * 함수 이름 : serverProcess
 * 함수 역할 : 서버쪽 동작을 담당하는 함수
*/
void *serverProcess(void* arg)
{
    char sendMessage[50] = {};                                                  // 보낼 메세지를 저장하는 배열
    char recvMessage[50] = {};                                                  // 받은 메세지를 저장하는 배열
    int closeConnect = 0;                                                       // 연결 종료 flag
    int mode = 0;                                                               // 0 : chatting , 1 : yatch
    int roomNumber = 0;                                                         // 현재 Yatch 방 번호
    int playerId = 0;                                                           // Yatch 플레이어 번호
    
    int i;
    int flag = 1;
    
    int winner = 0;                                                             // yatch 게임 승리자 번호
    
    struct ClientInfo *client = (struct ClientInfo *)arg;                       // 현재 Thread에서 다룰 client
    bzero(sendMessage, sizeof(sendMessage));                                    // sendMessage 초기화
    bzero(recvMessage, sizeof(recvMessage));                                    // recvMessage 초기화

    writeWithUID(client->socket, 0, "welcome to chatting server!");
    sleep(0.5);
    
    sprintf(sendMessage, "Your client number is %d.", client->uid);
    writeWithUID(client->socket, 0, sendMessage);

    while(1){
        if(closeConnect == 1){                                                  // 연결 종료 flag가 활성화 되면 연결 종료
            break;
        }

        bzero(sendMessage, sizeof(sendMessage));
        bzero(recvMessage, sizeof(recvMessage));

        read(client->socket, recvMessage, sizeof(recvMessage) - 1);             // 소켓으로부터 메세지 읽음

        if( (int)strlen(recvMessage) == 0){                                     // 오류 제어 부분, client에서 ctrl+c가 발생한 경우
            closeConnect = 1;                                                   // 연결 종료 flag 활성화
            strcpy(recvMessage, "Interrupt");                                   // interrupt 메세지 생성
            broadcast("quit", client->uid);                                     // 다른 client들에게 quit전송
            if(mode == MODE_YATCH){                                             // yatch를 하고 있는 도중이었을 경우
                if(yatchRoomList[roomNumber].roomStatus == WAITING_FOR_PLAYER){ // 새로운 방에서 다른 player를 기다리고 있는 경우
                    broadcast("Yatch Room Destroyed", 0);                       // 방 파괴
                    yatchRoomList[roomNumber].roomStatus = IDLE;                
                    mode = MODE_CHATTING;                                       // 채팅 모드로 복귀
                }
            }
        }

        printf("From client %d : ", client->uid);                               // 어느 client에서 어떤 메세지가 왔는지 출력
        printf("%s\n", recvMessage);

        if(strcmp(recvMessage, "quit") == 0){                                   // quit 메세지를 받은 경우
            printf("Client %d has left.\n", client->uid);                       // client 연결 해제 출력
            closeConnect = 1;                                                   // 연결 종료 flag 활성화

            if(mode == MODE_YATCH){                                             // yatch를 하고 있는 도중이었을 경우
                if(yatchRoomList[roomNumber].roomStatus == WAITING_FOR_PLAYER){ // 새로운 방에서 다른 player를 기다리고 있는 경우
                    broadcast("Yatch Room Destroyed", 0);                       // 방 파괴
                    yatchRoomList[roomNumber].roomStatus = IDLE;
                    mode = MODE_CHATTING;                                       // 채팅 모드로 복귀
                }
                else{                                                           // 게임 진행 도중이었을 경우
                    winner = 3-playerId;                                        // 상대의 승리로 판정
                    sprintf(sendMessage, "One player resigned, Winner is Player %d\n", winner);
                    roomBroadcast(roomNumber, sendMessage);                     // 상대에게 승리 메세지 출력
                    mode = MODE_CHATTING;                                       // 채팅 모드로 복귀
                    yatchRoomList[roomNumber].roomStatus = IDLE;                // 방 종료
                }
            }
        }

        switch(mode){                                                                               // 이외의 메세지의 경우 mode에 맞게 처리
            case MODE_CHATTING:                                                                     // 채팅 모드에서
                if(strcmp(recvMessage, "yatch") == 0){                                              // 1. yatch 명령어를 받은 경우
                    roomNumber = getEmptyRoomNumber();                                              // 빈 방 할당 혹은 방에 참여
                    if(roomNumber == -1){
                        writeWithUID(client->socket, 0, "Room limit exceeded");                     // 빈 방이 없는 경우 할당 못받음을 전송
                    }
                    else{                                                                           // 빈 방 할당 혹은 참여에 성공한 경우
                        printf("Client %d has joined yatch room %d.\n", client->uid, roomNumber);
                        
                        if(yatchRoomList[roomNumber].roomStatus == WAITING_FOR_PLAYER){             // 빈 방을 할당받은 경우
                            broadcast("Yatch Room Created", 0);                                     // 새 방이 열렸음을 다른 client에게 broadcast
                            printf("Room %d, Player1 is Client %d.\n", roomNumber,client->uid);
                            playerId = 1;                                                           // 1번 player가 됨
                            yatchRoomList[roomNumber].clients[0] = client;
                            writeWithUID(client->socket, 0, "You are Player 1");
                            writeWithUID(client->socket, 0, "Waiting for another player...");       // 다른 플레이어 대기
                        }
                        else{                                                                       // 방에 참여한 경우
                            broadcast("Yatch Room Closed", 0);                                      // 해당 방이 찼음을 다른 client에게 broadcast
                            printf("Room %d, Player2 is %d.\n", roomNumber,client->uid);
                            playerId = 2;                                                           // 2번 플레이어가 됨
                            yatchRoomList[roomNumber].clients[1] = client;
                            writeWithUID(client->socket, 0, "Joined to a room");
                            writeWithUID(client->socket, 0, "You are Player 2");
                            sleep(1);
                            roomBroadcast(roomNumber, "Start Game!");                               // 방이 찼으므로 게임 시작
                            roomBroadcast(roomNumber, "It's Player 1's turn");
                        }
                        mode = MODE_YATCH;                                                          // Yatch mode로 전환
                        yatchRoomList[roomNumber].turn = 1;                                         // player 1의 차례로 초기화
                    }
                }
                else{
                    broadcast(recvMessage, client->uid);                                            // 다른 메세지의 경우 broadcast
                }
                break;

            case MODE_YATCH:                                                                        // Yatch를 play 하고 있는 경우
                if(yatchRoomList[roomNumber].turn == playerId){                                     // 내 차례인 경우
                    switch(yatchRoomList[roomNumber].gameStatus){                                   // gameStatus에 따라 메세지 처리
                        
                        case FIRST_ROLL:                                                            // 첫 번째 주사위 굴리기 단계에서
                        srand(time(NULL));
                        if(strcmp(recvMessage, "roll") == 0){                                       // roll 명령어를 받은 경우
                            for(i=0; i<5; i++){
                                yatchRoomList[roomNumber].diceResult[i] = rand()%5+1;               // 주사위 굴리기
                                sendMessage[i] = yatchRoomList[roomNumber].diceResult[i]+'0';
                            }
                            roomBroadcast(roomNumber, sendMessage);                                 // 플레이어들에게 주사위 굴린 결과 전송
                            printf("Dice result : %s\n", sendMessage);

                            roomBroadcast(roomNumber, "Select Numbers");
                            yatchRoomList[roomNumber].gameStatus = SELECTION;                       // 주사위 선택 단계로 전환
                        }
                        else{
                            writeWithUID(                                                           // 다른 메세지의 경우
                                yatchRoomList[roomNumber].clients[1-(playerId-1)]->socket,          // 다른 플레이어에게 전달
                                playerId, recvMessage
                            );
                        }
                        break;
                        
                        case SELECTION:                                                             // 주사위 선택 단계의 경우
                        if(
                            recvMessage[0] == 's' &&                                                // 앞부분이 sel 명령어로 시작하는 경우
                            recvMessage[1] == 'e' &&
                            recvMessage[2] == 'l'
                        ){
                            yatchRoomList[roomNumber].check[0] = recvMessage[4]-'0';                // 뒤의 5개의 값으로 주사위 선택여부 확인
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
                            if(checksum == 5){                                                      // 5개를 모두 선택한 경우
                                yatchRoomList[roomNumber].gameStatus = SCORING;                     // 두 번째 주사위 굴리기 단계로 넘어가지 않고 바로 점수 단계로 전환
                                clear_temp(&yatchRoomList[roomNumber].temp);
                                calc_point(&yatchRoomList[roomNumber].temp,                         // 점수 계산
                                    yatchRoomList[roomNumber].diceResult);
                                roomBroadcast(roomNumber, "Select Score slot");
                            }
                            else{
                                yatchRoomList[roomNumber].gameStatus = SECOND_ROLL;                 // 5개보다 적게 선택한 경우
                                roomBroadcast(roomNumber, "Roll again");                            // 두 번째 주사위 굴리기 단계로 전환
                            }
                        }
                        else{                                                                       // 명령어가 아닌 메세지는 다른 플레이어에게 전달
                            writeWithUID(
                                yatchRoomList[roomNumber].clients[1-(playerId-1)]->socket, 
                                playerId, recvMessage
                            );
                        }
                        break;

                        case SECOND_ROLL:                                                               // 두 번째 주사위 굴리기 단계에서
                        if(strcmp(recvMessage, "roll") == 0){                                           // roll 명령어를 받은 경우
                            srand(time(NULL));
                            for(i=0; i<5; i++){
                                if(yatchRoomList[roomNumber].check[i] == 0){                            // 체크되지 않은 주사위를 다시 굴림
                                    
                                    yatchRoomList[roomNumber].diceResult[i] = rand()%5+1;
                                    sendMessage[i] = yatchRoomList[roomNumber].diceResult[i]+'0';
                                    printf("change %d : %d\n", i, sendMessage[i]-'0');
                                }
                                else{
                                    sendMessage[i] = yatchRoomList[roomNumber].diceResult[i] + '0';     // 체크된 경우 주사위 굴리지 않음
                                    printf("not change %d : %d\n", i, sendMessage[i]-'0');
                                }
                            }
                            roomBroadcast(roomNumber, sendMessage);                                     // 다시 굴린 결과를 플레이어에게 전달
                            printf("Dice result : %s\n", sendMessage);

                            clear_temp(&yatchRoomList[roomNumber].temp);                                // 점수 계산
                            calc_point(
                                &yatchRoomList[roomNumber].temp, 
                                yatchRoomList[roomNumber].diceResult
                                );
                            yatchRoomList[roomNumber].gameStatus = SCORING;                             // 점수 반영 단계로 전환
                            roomBroadcast(roomNumber, "Select Score slot");

                        }
                        else{
                            writeWithUID(                                                               // 명령어가 아닌 메세지의 경우
                                yatchRoomList[roomNumber].clients[1-(playerId-1)]->socket,              // 다른 플레이어에게 전달
                                playerId, recvMessage
                            );
                        }
                        break;

                        case SCORING:                                                                   // 점수 반영 단계에서
                        flag = 1;
                        // ones에 반영하는 경우
                        if(strcmp(recvMessage, "ones") == 0){
                            // player의 playerBoard의 ones에 temp의 ones 값을 가산
                            yatchRoomList[roomNumber].playerBoard[playerId - 1].ones = yatchRoomList[roomNumber].temp.ones;
                            // player가 선택한 항목과 해당 항목에 대한 점수를 다른 플레이어에게 전달
                            sprintf(sendMessage, "Player %d earned %d points for %s", playerId, yatchRoomList[roomNumber].temp.ones, "ones");
                            roomBroadcast(roomNumber, sendMessage);

                            //이하 다른 항목에 대해서 동일한 조건문이 반복되므로 주석을 생략
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
                            writeWithUID(                                                                           // 점수 항목에 대한 메세지가 아닌 다른 메세지를 받은 경우
                                yatchRoomList[roomNumber].clients[1-(playerId-1)]->socket,                          // 다른 플레이어에게 전달
                                playerId, recvMessage
                            );
                            flag = 0;                                                                               // 점수 반영 flag를 0으로 설정
                        }
                        if(flag == 1){                                                                              // 점수 반영 flag가 1인 경우 -> 점수가 반영된 경우
                            yatchRoomList[roomNumber].gameStatus = FIRST_ROLL;                                      // 상대 차례의 첫 번째 단계로 넘어감
                            calc_final_point(&yatchRoomList[roomNumber].playerBoard[playerId-1]);                   // 해당 스테이지 최종 점수 계산
                            print_board(
                                yatchRoomList[roomNumber].playerBoard[0], yatchRoomList[roomNumber].playerBoard[1], // 서버에 보드판 출력
                                yatchRoomList[roomNumber].stage, 
                                yatchRoomList[roomNumber].turn
                            );

                            if(yatchRoomList[roomNumber].turn == 2){ yatchRoomList[roomNumber].stage++; }           // 해당 stage에서 player2의 차례가 끝난 경우 stage 증가
                            yatchRoomList[roomNumber].turn = 3 - yatchRoomList[roomNumber].turn;                    // turn 값을 상대 playerID로 설정
                            if(yatchRoomList[roomNumber].stage > GAME_STAGE_AMOUNT){                                // 게임이 끝난 경우
                                winner = yatchRoomList[roomNumber].playerBoard[0].tot > yatchRoomList[roomNumber].playerBoard[1].tot ? 1:2; // tot 값으로 승자 결정
                                sprintf(sendMessage, "Game over, Winner is Player %d", winner);
                                roomBroadcast(roomNumber, sendMessage);
                                mode = MODE_CHATTING;                                                               // 채팅 모드로 복귀
                                yatchRoomList[roomNumber].roomStatus = IDLE;                                        // 방 종료
                            }
                            else{
                                sprintf(sendMessage, "It's Player %d's turn.", yatchRoomList[roomNumber].turn);     // 게임이 안끝난 경우
                                roomBroadcast(roomNumber, sendMessage);                                             // 턴 넘어감 출력
                            }
                        }
                        
                        break;
                    }
                }
                else{
                    writeWithUID(                                                                                   // 어떤 명령어도 아닌 경우
                        yatchRoomList[roomNumber].clients[1-(playerId-1)]->socket, playerId, recvMessage);          // 다른 플레이어에게 메세지 전달
                }
                break;
        }
        
        printf("\n");
    }

    close(client->socket);                  // 클라이언트에 대한 소켓 종료
    removClient(client->uid);               // 클라이언트 제거
    free(client);                           // 메모리 release
    pthread_detach(pthread_self());         // thread 종료
    return NULL;
}

/*
 * 함수 이름 : getEmptyRoomNumber
 * 함수 역할 : 빈 방을 할당하는 함수
*/
int getEmptyRoomNumber()
{
    int i=0;
    for(i=0; i<5; i++){                                         // 플레이어를 기다리는 방이 있는지를 먼저 탐색
        if(yatchRoomList[i].roomStatus == WAITING_FOR_PLAYER){  // 그런 방이 있으면
            yatchRoomList[i].roomStatus = IN_GAME;              // in game으로 바꾸고 방 번호 반환
            return i;
        }
    }
    for(i=0; i<5; i++){                                         // 그런 방이 없으면 빈 방을 탐색
        if(yatchRoomList[i].roomStatus == IDLE){                // 빈 방이 있으면
            yatchRoomList[i].roomStatus = WAITING_FOR_PLAYER;   // 플레이어 대기 상태로 바꾸고 방 번호 반환
            return i;
        }
    }
    return -1;                                                  // 그런 방도 없으면 -1 반환
}

int main()
{
    char sendMessage[50] = {};                                  // client에 전송할 메세지
    char ipAddr[16] = "127.0.0.1";                              // 서버의 ip주소
    int portNum = 12345;                                        // 서버의 port 번호

    int serverSocket;                                           // 서버 소켓
    int clientSocket;                                           // 클라이언트쪽 소켓
    struct sockaddr_in clientAddr;                              // 클라이언트 소켓 정보
    struct sockaddr_in servAddr;                                // 서버 소켓 정보

    int clientAddrSize;                                         // 클라이언트 주소 크기

    pthread_t tid;                                              // pthread 인스턴스

    clientList.count = 0;                                       // clientList 초기화
    clientList.nextUid = 1;                                     // clientList 초기화
    printf("Input Server IP Address : ");
    scanf("%s", ipAddr);
    printf("Input Server Port Number : ");
    scanf("%d", &portNum);

    

    int i=0;
    for(i=0; i<5; i++){
        yatchRoomList[i].roomStatus = IDLE;                     // yatch 방 초기화
    }

    serverSocket = socket(PF_INET, SOCK_STREAM, 0);             // server 소켓 설정
    int option = 1;                                             // 중복된 포트에 bind 허용
    setsockopt(serverSocket, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));
    if(serverSocket == -1){                                     // 소켓 생성 오류 처리
        printf("ERROR : Generating socket\n");
        return 0;
    }

    memset( &servAddr, 0, sizeof(servAddr));                    // 서버 소켓 정보 초기화
    servAddr.sin_family = AF_INET;                              // 서버 소켓 정보 설정
    servAddr.sin_addr.s_addr = inet_addr(ipAddr);
    servAddr.sin_port = htons(portNum);

    int err = bind(serverSocket, (struct sockaddr *)&servAddr, sizeof(servAddr));
    if(err == -1){                                              // 서버 소켓 바인드 오류 처리
        printf("ERROR : bind\n");
        return 0;
    }
    printf("Server initialized.\n");
    
    err = listen(serverSocket, 5);                              // 서버 소켓 listen 오류 처리
    if(err == -1){
        printf("ERROR : listen\n");
        return 0;
    }
    while(1){
        if(clientList.count == 0 && closeServer == 1){          // 정당한 close server 명령이 전달된 경우
            break;                                              // server 종료
        }

        clientAddrSize = sizeof(clientAddr);
        clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddr, &clientAddrSize);

        if(clientSocket == -1){                                 // client accept 오류 처리
            printf("ERROR : accept\n");
        }
        else{
            if(clientList.count == 5){                          // client limit을 넘어선 경우
                printf("Client count limit exceeded\n");
                close(clientSocket);                            // 연결 해제
                continue;
            }
            else{
                struct ClientInfo *new = (struct ClientInfo *)malloc(sizeof(struct ClientInfo));    // 새로운 client 인스턴스 생성
                new->uid = clientList.nextUid++;
                new->socket = clientSocket;
                new->address = clientAddr;
                
                sprintf(sendMessage, "Client %d has joined to channel.\n", new->uid);
                printf("%s\n", sendMessage);
                broadcast(sendMessage, 0);                                                          // 새 client 접속 안내를 broadcasting
                addClient(new);                                                                     // clientList에 client 추가
                pthread_create(&tid, NULL, &serverProcess, (void*)new);                             // thread 생성 및 연결
            }
        }
        sleep(1);
    }
    close(serverSocket);                                                                            // 서버 소켓 비활성화
    printf("Server closed");
    return 0;                                                                                       // 종료
}