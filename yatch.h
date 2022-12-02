#ifndef OUTPUT_H_INCLUDED
#define OUTPUT_H_INCLUDED



#endif // OUTPUT_H_INCLUDED
#include <stdio.h>

struct ScoreBoard{
    int ones;
    int twos;
    int threes;
    int fours;
    int fives;
    int sixes;
    int bonus;
    int yatch;
    int lStraight;
    int sStraight;
    int foKind;
    int fHouse;
    int tot;
};

void mv(int x, int y)
{
    // COORD pos = {x,y};
    // SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
    printf("\033[%d;%df",y,x);
	fflush(stdout);
}

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

static void print_help(){
    printf("-------------------------------------------------\n");
    printf("|                  Yatch Dice                   |\n");
    printf("-------------------------------------------------\n");
    printf("1. FIRST ROLL : roll\n");
    printf("2. SELECTION : sel 00000~11111, 1 means keep, 0 means rolling again\n");
    printf("3. SECOND ROLL : roll\n");
    printf("4. SCORING : enter a item title. ex)ones, twos, ... , full_house\n");
}

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

static void calc_point(struct ScoreBoard *board, int dice[])
{
    int num[5];
    int check[6] = {0,0,0,0,0,0};
    int dual = -1;
    int triple = -1;

    for(int i=0; i<5; i++){
        num[i] = dice[i];
        if(num[i] == 1){board->ones += num[i]; check[0]++;} // ones
        if(num[i] == 2){board->twos += num[i]; check[1]++;} // twos
        if(num[i] == 3){board->threes += num[i]; check[2]++;} // threes
        if(num[i] == 4){board->fours += num[i]; check[3]++;} // fours
        if(num[i] == 5){board->fives += num[i]; check[4]++;} // fives
        if(num[i] == 6){board->sixes += num[i]; check[5]++;} // sixes
        printf("%d ", dice[i]);
    }
    
    for(int i=0; i<6; i++){
        if(check[i] >= 4){
            board->foKind = (i+1)*check[i]; // fokind
        }
        if(check[i] == 5){
            board->yatch = 50; // yatch
            board->fHouse = (i+1)*5;
        }
        if(check[i] == 2){dual = i+1;} // tmp for finding full house
        if(check[i] == 3){triple = i+1;}
    }

    if(check[0] == 1 && check[1] == 1 && check[2] == 1 && check[3] == 1 && check[4] == 1){ // small straight
        board->sStraight = 30;
    }
    if(check[1] == 1 &&check[2] == 1 &&check[3] == 1 &&check[4] == 1 &&check[5] == 1){ // large straight
        board->lStraight = 30;
    }
    if(dual!= -1 && triple != -1){
        board->fHouse = 2*dual + 3*triple;
    }
}

static void calc_final_point(struct ScoreBoard *board){
    int upperSum = board->ones + board->twos + board->threes + board->fours + board->fives + board->sixes;
    if(upperSum >= 63){ board->bonus = 35; }
    board->tot = upperSum + board->bonus + board->fHouse + board->foKind + board->sStraight + board->lStraight + board->yatch;
}