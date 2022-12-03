/*
 * 파일 이름 : yatch.h
 * 파일 설명 : Yatch 게임을 진행하는데 필요한 구조체와 함수를 기술
 * 작성 날짜 : 2022/12/02
 * 헤더파일에 기술된 함수는 snake case를 적용
*/

#ifndef OUTPUT_H_INCLUDED
#define OUTPUT_H_INCLUDED



#endif // OUTPUT_H_INCLUDED
#include <stdio.h>

/*
 *
 * 구조체 이름 : ScoreBoard
 * 구조체 역할 : Yatch 게임에 사용되는 점수판
 * 자세한 게임 룰은 Wikipedia 문서(https://en.wikipedia.org/wiki/Yacht_(dice_game)) 참조
*/
struct ScoreBoard{
    int ones;
    int twos;
    int threes;
    int fours;
    int fives;
    int sixes;
    int bonus;     // 위의 값들의 총합이 63 이상이면 추가로 35점
    int yatch;
    int lStraight;
    int sStraight;
    int foKind;
    int fHouse;
    int tot;       // 총점
};

/*
 * 함수 이름 : mv
 * 함수 인자 : int x, int y
 * 함수 역할 : dos창의 원하는 좌표로 커서를 옮긴다
 */
void mv(int x, int y)
{
    printf("\033[%d;%df",y,x);
	fflush(stdout);
}


/*
 * 함수 이름 : print_board
 * 함수 인자 : ScoreBoard player1, ScoreBoard player2, int stage, int turn
 * 함수 역할 : 지금까지의 게임 진행 결과를 출력한다.
 */
static void print_board(struct ScoreBoard player1, struct ScoreBoard player2, int stage, int turn)
{
    printf("-------------------------------------------------\n");
    printf("|                  Yatch Dice                   |\n");
    printf("-------------------------------------------------\n");
    printf("               stage : %d    turn : %d          \n", stage, turn);
    printf("-------------------------------------------------\n");
    printf("|                 |  Player 1     |  Player 2   |\n");
    printf("------------------------------------------------|\n");
    printf("|  Ones           |      %3d     |      %3d     |\n", player1.ones, player2.ones);
    printf("|  Tows           |      %3d     |      %3d     |\n", player1.twos, player2.twos);
    printf("|  Threes         |      %3d     |      %3d     |\n", player1.threes, player2.threes);
    printf("|  Fours          |      %3d     |      %3d     |\n", player1.fours, player2.fours);
    printf("|  Fives          |      %3d     |      %3d     |\n", player1.fives, player2.fives);
    printf("|  Sixes          |      %3d     |      %3d     |\n", player1.sixes, player2.sixes);
    printf("|  Bonus          |      %3d     |      %3d     |\n", player1.bonus, player2.bonus);
    printf("|  Yatch          |      %3d     |      %3d     |\n", player1.yatch, player2.yatch);
    printf("|  Large Straight |      %3d     |      %3d     |\n", player1.lStraight, player2.lStraight);
    printf("|  Small Straight |      %3d     |      %3d     |\n", player1.sStraight, player2.sStraight);
    printf("|  Four of a kind |      %3d     |      %3d     |\n", player1.foKind, player2.foKind);
    printf("|  Full House     |      %3d     |      %3d     |\n", player1.fHouse, player2.fHouse);
    printf("------------------------------------------------|\n");
    printf("|  Total score    |      %3d     |      %3d     |\n", player1.tot, player2.tot);
    printf("------------------------------------------------|\n");
    
}

/*
 * 함수 이름 : print_help
 * 함수 인자 : 없음
 * 함수 역할 : Yatch게임과 관련된 도움말을 출력한다.
 */
static void print_help(){
    printf("-------------------------------------------------\n");
    printf("|                  Yatch Dice                   |\n");
    printf("-------------------------------------------------\n");
    printf("1. FIRST ROLL : roll\n");
    printf("2. SELECTION : sel 00000~11111, 1 means keep, 0 means rolling again\n");
    printf("3. SECOND ROLL : roll\n");
    printf("4. SCORING : enter a item title. ex)ones, twos, ... , full_house\n");
}

/*
 * 함수 이름 : clear_temp
 * 함수 인자 : ScoreBoard *board
 * 함수 역할 : ScoreBoard 인스턴스를 초기화한다
 */
static void clear_temp(struct ScoreBoard *board){
    board->ones=0;
    board->twos=0;
    board->threes=0;
    board->fours=0;
    board->fives=0;
    board->sixes=0;
    board->bonus=0;
    board->yatch=0;
    board->lStraight=0;
    board->sStraight=0;
    board->foKind=0;
    board->fHouse=0;
    board->tot=0;
}

/*
 * 함수 이름 : calc_point
 * 함수 인자 : ScoreBoard *board, int dice[]
 * 함수 역할 : 주사위 결과(dice[])를 이용해 각 기준에 해당하는 점수를 계산하고 board에 저장한다.
 */
static void calc_point(struct ScoreBoard *board, int dice[])
{
    int num[5];                     
    int check[6] = {0,0,0,0,0,0};                                           // 주사위의 각 눈이 몇 개가 나왔는지를 저장하는 배열
    int dual = -1;                                                          // 2개가 나온 값
    int triple = -1;                                                        // 3개가 나온 값
    int total_dice = 0;
    for(int i=0; i<5; i++){                                                 // 일반적인 값 계산
        num[i] = dice[i];
        if(num[i] == 1){board->ones += num[i]; check[0]++;}                 // ones에 1 저장
        if(num[i] == 2){board->twos += num[i]; check[1]++;}                 // twos에 2 저장
        if(num[i] == 3){board->threes += num[i]; check[2]++;}               // threes에 3 저장
        if(num[i] == 4){board->fours += num[i]; check[3]++;}                // fours에 4 저장
        if(num[i] == 5){board->fives += num[i]; check[4]++;}                // fives에 5 저장
        if(num[i] == 6){board->sixes += num[i]; check[5]++;}                // sixes에 6 저장
        printf("%d ", dice[i]);
        total_dice += num[i];                                               // 주사위 눈의 총합은 full house나 four of a kind를 구할때 사용한다.
    }
    
    for(int i=0; i<6; i++){                                                 // 특수 족보 계산
        if(check[i] >= 4){                                                  // Four of a kind : 4개가 동일한 눈이 나온 경우
            board->foKind = total_dice;
        }
        if(check[i] == 5){                                                  // Yatch : 5개가 동일한 눈이 나온 경우 고정 50점
            board->yatch = 50;
            board->fHouse = (i+1)*5;                                        // Full house의 정의상 Yatch는 Full house가 될 수 있다.
        }
        if(check[i] == 2){dual = i+1;}                                      // 2개가 나온 값을 dual에 저장
        if(check[i] == 3){triple = i+1;}                                    // 3개가 나온 값을 triple에 저장
    }

    if(check[0] == 1 && check[1] == 1 && check[2] == 1 && check[3] == 1){ // Small Straight : 1 2 3 4, 2 3 4 5, 3 4 5 6가 나온 경우 고정 15점
        board->sStraight = 15;
    }
    if(check[1] == 1 && check[2] == 1 && check[3] == 1 && check[4] == 1){ // Small Straight : 1 2 3 4, 2 3 4 5, 3 4 5 6가 나온 경우 고정 15점
        board->sStraight = 15;
    }
    if(check[2] == 1 && check[3] == 1 && check[4] == 1 && check[5] == 1){ // Small Straight : 1 2 3 4, 2 3 4 5, 3 4 5 6가 나온 경우 고정 15점
        board->sStraight = 15;
    }
    if(check[0] == 1 && check[1] == 1 && check[2] == 1 && check[3] == 1 && check[4] == 1){ // Large Straight : 1 2 3 4 5, 2 3 4 5 6이 나온 경우 고정 30점
        board->lStraight = 30;
    }
    if(check[1] == 1 && check[2] == 1 && check[3] == 1 && check[4] == 1 && check[5] == 1){ // Large Straight : 1 2 3 4 5, 2 3 4 5 6이 나온 경우 고정 30점
        board->lStraight = 30;
    }
    if(dual!= -1 && triple != -1){                                           // Full house : 똑같은 눈이 3개, 2개 나온 경우
        board->fHouse = total_dice;
    }
}

/*
 * 함수 이름 : calc_final_point
 * 함수 인자 : struct ScoreBoard *board
 * 함수 역할 : bonus 항목과 tot 항목을 계산하여 최종 결과 보드를 반환한다.
 */
static void calc_final_point(struct ScoreBoard *board){ 
    int upperSum = board->ones + board->twos + board->threes + board->fours + board->fives + board->sixes;
    if(upperSum >= 63){ board->bonus = 35; }
    board->tot = upperSum + board->bonus + board->fHouse + board->foKind + board->sStraight + board->lStraight + board->yatch;
}