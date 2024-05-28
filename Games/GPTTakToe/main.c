#define BIN_TYPE BIN_CPM  // Define the binary type for Cloud CP/M

#include "../../NABULIB/NABU-LIB.h"

// Define grid size for the tic-tac-toe board
#define GRID_SIZE 3

// Function declarations
void initGame();
void drawBoard(char board[GRID_SIZE][GRID_SIZE]);
int checkWin(char board[GRID_SIZE][GRID_SIZE]);
void playGame();

int main() {
    playGame();     // Start the game loop
    return 0;
}

void initGame() {
    vt_clearScreen(); // Clear the screen using VT52 command
    vt_cursorHome();  // Move cursor to home position
}

void drawBoard(char board[GRID_SIZE][GRID_SIZE]) {
    vt_clearScreen(); // Clear the screen
    for (int row = 0; row < GRID_SIZE; row++) {
        for (int col = 0; col < GRID_SIZE; col++) {
            printf(" %c |", board[row][col]);  // Print the board character
        }
        printf("\b \n");  // Overwrite last '|' and move to new line
        if (row < GRID_SIZE - 1) {
            printf("---|---|---\n"); // Draw horizontal line between rows
        }
    }
}

int checkWin(char board[GRID_SIZE][GRID_SIZE]) {
    // Check horizontal, vertical, and diagonal lines for a win
    for (int i = 0; i < GRID_SIZE; i++) {
        if (board[i][0] == board[i][1] && board[i][0] == board[i][2] && board[i][0] != ' ')
            return (board[i][0] == 'X') ? 1 : 2;
        if (board[0][i] == board[1][i] && board[0][i] == board[2][i] && board[0][i] != ' ')
            return (board[0][i] == 'X') ? 1 : 2;
    }
    if (board[0][0] == board[1][1] && board[0][0] == board[2][2] && board[0][0] != ' ')
        return (board[0][0] == 'X') ? 1 : 2;
    if (board[0][2] == board[1][1] && board[0][2] == board[2][0] && board[0][2] != ' ')
        return (board[0][2] == 'X') ? 1 : 2;

    return 0; // No winner yet
}

void playGame() {

    puts("");
    puts("");
    puts("This game was created by Chat GPT on ");
    puts("May 1, 2024 prompted by DJ Sures. The ");
    puts("prompt was asked to create a CPM ");
    puts("text-based tik tak toe game using VT ");
    puts("Terminal commands and the NABU-LIB.H ");
    puts("file was pasted in.");
    puts("");
    puts("Press any key to begin");
    getchar();

    char board[GRID_SIZE][GRID_SIZE] = {{' ', ' ', ' '}, {' ', ' ', ' '}, {' ', ' ', ' '}};
    int currentPlayer = 1; // Player 1 starts
    int winner = 0;
    int moves = 0;

    initGame();

    while (winner == 0 && moves < 9) {

        int row, col;
        bool tryAgain = false;

        do {

            drawBoard(board);

            vt_setCursor(0, 6);  // Move cursor to input position

            if (tryAgain) 
              puts("Something already there, try again!");

            printf("Player %d", currentPlayer); puts("");
            puts("Enter row [1-3] and column [1-3]");
            puts("(type 0 0 to quit)");

            scanf("%d %d", &row, &col);
            
            if (row == 0 && col == 0) {

              puts("Exiting...");

              return;
            }

            row--; col--; // Adjust for zero-indexed array

            tryAgain = (board[row][col] != ' ');
        } while (row < 0 || row >= GRID_SIZE || col < 0 || col >= GRID_SIZE || board[row][col] != ' ');

        board[row][col] = (currentPlayer == 1) ? 'X' : 'O';
        drawBoard(board);
        winner = checkWin(board);
        currentPlayer = 3 - currentPlayer; // Switch player
        moves++;
    }

    if (winner != 0)
        printf("\033[10;10HPlayer %d wins!\n", winner);  // Move cursor and print winner
    else
        printf("\033[10;10HIt's a draw!\n");  // Move cursor and print draw
}
