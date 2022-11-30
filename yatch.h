#ifndef OUTPUT_H_INCLUDED
#define OUTPUT_H_INCLUDED



#endif // OUTPUT_H_INCLUDED
#include <stdio.h>

struct ScoreBoard{
    int ones;
    int tows;
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

static void print_board(struct ScoreBoard player1, struct ScoreBoard player2)
{
    printf("-------------------------------------------------\n");
    printf("|                  Yatch Dice                   |\n");
    printf("-------------------------------------------------\n");
    printf("                                            \n");
    printf("-------------------------------------------------\n");
    printf("|                 |  Player 1     |  Player 2   |\n");
    printf("------------------------------------------------|\n");
    printf("|  Ones           |      %3d     |      %3d     |\n", player1.ones, player2.ones);
    printf("|  Tows           |      %3d     |      %3d     |\n", player1.tows, player2.tows);
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