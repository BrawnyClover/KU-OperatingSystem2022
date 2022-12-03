#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <string.h>
#include "yatch.h"

#define MODE_YATCH 1                    // Yatch 모드를 의미하는 상수
#define MODE_CHAT 0                     // Chatting 모드를 의미하는 상수

int clientSocket;                       // Client 소켓
int endFlag = 0;                        // 연결 해제 명령
int mode = 0;                           // 0 : chatting, 1 : yatch     


/*
 * 함수 이름 : sender
 * 함수 역할 : 서버로 패킷을 보낸다
*/
void *sender(void* arg){
    char sendMessage[50] = {};                                          // 보낼 메세지를 담는 문자형 배열
    while(1){                                                           // 소켓이 살아있는동안 계속 반복한다
        gets(sendMessage);                                              // 메세지를 키 입력으로 받음.
        if(mode == MODE_YATCH){                                         // Yatch 플레이 중일때 입력 명령어에 대한 처리 부분
            if(strcmp(sendMessage, "help") == 0){                       // help : Yatch 관련 명령어에 대한 도움말을 출력한다
                print_help();
                continue;
            }
        }

        write(clientSocket, sendMessage, (int)strlen(sendMessage));     // write로 sendMessage를 서버에 전송
        if(strcmp(sendMessage, "quit") == 0){                           // sendMessage가 quit인 경우 클라이언트 종료
            endFlag = 1;                                                // "quit"를 서버에도 보내는 이유는, 클라이언트 종료를 다른 클라이언트에게도
            break;                                                      // 알리기 쉽게 하기 위해서임
        }
        if(strcmp(sendMessage, "yatch") == 0){                          // sendMessage가 yatch인 경우, Yatch 플레이 모드로 변경
            mode = MODE_YATCH;
        }
        bzero(sendMessage, sizeof(sendMessage));                        // sendMessage 초기화
    }
}


/*
 * 함수 이름 : receiver
 * 함수 역할 : 서버로부터 패킷을 받아 처리, 출력한다.
*/
void *receiver(void* arg){
    char recvMessage[50] = {};                                              // 받은 메세지를 담는 문자형 배열
    while(1){
        int recvLen = read(clientSocket, recvMessage, sizeof(recvMessage)); // read로 서버로부터 패킷을 받음
        int uid = recvMessage[recvLen-1] - '0';                             // 프로토콜에 따라 메세지의 맨 끝 글자가 uid임, uid 정보 저장
        recvMessage[recvLen-1] = '\0';                                      // uid 부분을 널문자로 치환

        if(uid<0 || uid > 10){                                              // 오류 처리 구문
            endFlag = 2;                                                    // 서버로부터 잘못된 메세지를 받은 경우 종료
            break;
        }
        if((int)strlen(recvMessage) == 0){                                  // 오류 처리 구문
            endFlag = 2;                                                    // 서버로부터 잘못된 메세지를 받은 경우 종료
            break;
        }

        if(uid == 0){                                                       // uid 0번은 서버로 고정
            printf("Message from server : ");
        }
        else if(mode == MODE_YATCH){                                        // Yatch play mode인 경우
            printf("Message from player %d : ", uid);                       // Player 번호 출력
        }
        else{
            printf("Message from Client %d : ", uid);                       // Chatting mode인 경우 Client 번호 출력
        }

        if(strcmp(recvMessage, "quit")==0){                                 // 다른 Client로부터 quit 메세지를 받은 경우
            printf("Client %d has left channel.\n", uid);                   // 해당 Client가 떠났음을 출력
        }
        else{                                                               // quit이 아닌 경우
            printf("%s\n", recvMessage);                                    // 받은 메세지를 출력
        }

        bzero(recvMessage, sizeof(recvMessage));                            // 메세지 저장 배열 초기화
    }
}

int main()
{
    char ipAddr[16] = "127.0.0.1";                                          // 서버 IP 주소
    int portNum = 12345;                                                    // 서버 포트 번호

    pthread_t senderThread;                                                 // 송신 담당 스레드
    pthread_t receiverThread;                                               // 수신 담당 스레드
    
    printf("Input Server IP Address : ");                                   // IP, 포트번호 입력 부분
    scanf("%s", ipAddr);
    printf("Input Server Port Number : ");
    scanf("%d", &portNum);

    struct sockaddr_in servAddr;                                            // 소켓 연결에 필요한 서버 정보
    memset( &servAddr, 0, sizeof(servAddr));                                // 서버 정보 초기화
    servAddr.sin_family = AF_INET;                                          // IPv4 인터넷 프로토콜
    servAddr.sin_addr.s_addr = inet_addr(ipAddr);                           // 서버 주소 등록
    servAddr.sin_port = htons(portNum);                                     // 포트 번호 등록

    
    clientSocket = socket(PF_INET, SOCK_STREAM, 0);                         // 소켓 생성
    if(clientSocket == -1){
        printf("ERROR : socket()\n");
        return 0;
    }

    int err 
    = connect(clientSocket, (struct sockaddr*)&servAddr, sizeof(servAddr)); // 소켓에 연결
    if(err == -1){
        printf("ERROR : connect\n");
        return 0;
    }

    err = pthread_create(&senderThread, NULL, (void *) sender, NULL);       // 송신 스레드 생성 및 연결
    if(err != 0){
        printf("ERROR : creating sender thread.\n");
        return 0;
    }
    else{
        printf("Initializing client...sender established\n");
    }

    err = pthread_create(&receiverThread, NULL, (void *) receiver, NULL);   // 수신 스레드 생성 및 연결
    if(err != 0){
        printf("ERROR : creating receiver thread.\n");
        return 0;
    }
    else{
        printf("Initializing client...receiver established\n");
    }

    printf("Client initialized, disconnect command is 'quit'.\n\n\n");      // 클라이언트 설정 완료

    while(1){
        if(endFlag == 1){                                                   // 종료 flag가 활성화되면 종료 및 소켓 연결 해제
            printf("Disconnecting...\n");
            close(clientSocket);
            break;
        }
        else if(endFlag == 2){
            printf("ERROR : Server side error\n");                          // 비정상 종료 flag가 활성화되면 종료, 비정상 종료 문구 출력 및
            printf("Disconnecting...\n");                                   // 소켓 해제
            close(clientSocket);
            break;
        }
    }
    return 0;
}