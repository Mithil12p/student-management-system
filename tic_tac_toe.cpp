/*
 * ============================================================
 *  TIC TAC TOE - Console Mini Game
 *  Language      : C++
 *  Concepts Used : 2D arrays, loops, conditional logic,
 *                  functions, basic AI (for single-player mode)
 *  Features      : Dynamic board display after every move,
 *                  win/draw detection, 2-player AND
 *                  1-player-vs-computer modes, replay option
 * ============================================================
 *
 *  Compile :  g++ -std=c++17 -O2 -o tictactoe tic_tac_toe.cpp
 *  Run     :  ./tictactoe          (Linux/Mac)
 *             tictactoe.exe        (Windows)
 * ============================================================
 */

#include <iostream>
#include <limits>
#include <cstdlib>
#include <ctime>

using namespace std;

const char EMPTY = ' ';
const char PLAYER_X = 'X';
const char PLAYER_O = 'O';

// ------------------------------------------------------------
//  Print the 3x3 board using the current state of the array.
//  Called after every single move so the board always reflects
//  the latest state (dynamic display).
// ------------------------------------------------------------
void printBoard(char board[3][3]) {
    cout << "\n";
    for (int row = 0; row < 3; row++) {
        cout << "   " << board[row][0] << " | " << board[row][1] << " | " << board[row][2] << "\n";
        if (row < 2) cout << "  ---+---+---\n";
    }
    cout << "\n";
}

// ------------------------------------------------------------
//  Show the numbered reference grid once at the start so the
//  player knows which number maps to which cell.
// ------------------------------------------------------------
void printReferenceGrid() {
    cout << "\nCells are numbered like this - enter a number to play there:\n";
    cout << "\n   1 | 2 | 3\n  ---+---+---\n   4 | 5 | 6\n  ---+---+---\n   7 | 8 | 9\n\n";
}

// ------------------------------------------------------------
//  Reset the board to all-empty cells (used at game start and
//  before every replay).
// ------------------------------------------------------------
void resetBoard(char board[3][3]) {
    for (int row = 0; row < 3; row++)
        for (int col = 0; col < 3; col++)
            board[row][col] = EMPTY;
}

// ------------------------------------------------------------
//  Convert a 1-9 cell number into (row, col). Returns false if
//  out of range or the cell is already occupied.
// ------------------------------------------------------------
bool applyMove(char board[3][3], int cellNumber, char symbol) {
    if (cellNumber < 1 || cellNumber > 9) return false;
    int row = (cellNumber - 1) / 3;
    int col = (cellNumber - 1) % 3;
    if (board[row][col] != EMPTY) return false;
    board[row][col] = symbol;
    return true;
}

// ------------------------------------------------------------
//  Check all 8 possible winning lines (3 rows, 3 cols, 2
//  diagonals) for the given symbol.
// ------------------------------------------------------------
bool checkWin(char board[3][3], char symbol) {
    // rows and columns
    for (int i = 0; i < 3; i++) {
        if (board[i][0] == symbol && board[i][1] == symbol && board[i][2] == symbol) return true;
        if (board[0][i] == symbol && board[1][i] == symbol && board[2][i] == symbol) return true;
    }
    // diagonals
    if (board[0][0] == symbol && board[1][1] == symbol && board[2][2] == symbol) return true;
    if (board[0][2] == symbol && board[1][1] == symbol && board[2][0] == symbol) return true;

    return false;
}

// ------------------------------------------------------------
//  Board is full -> if nobody has won, it's a draw.
// ------------------------------------------------------------
bool isBoardFull(char board[3][3]) {
    for (int row = 0; row < 3; row++)
        for (int col = 0; col < 3; col++)
            if (board[row][col] == EMPTY) return false;
    return true;
}

// ------------------------------------------------------------
//  Safely read an integer cell choice from a human player,
//  re-prompting on invalid or already-occupied cells.
// ------------------------------------------------------------
int readHumanMove(char board[3][3], const string& promptName) {
    int choice;
    while (true) {
        cout << promptName << ", enter your move (1-9): ";
        cin >> choice;

        if (cin.fail()) {
            cout << "That's not a number. Try again.\n";
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            continue;
        }

        if (choice < 1 || choice > 9) {
            cout << "Please enter a number between 1 and 9.\n";
            continue;
        }

        int row = (choice - 1) / 3;
        int col = (choice - 1) % 3;
        if (board[row][col] != EMPTY) {
            cout << "That cell is already taken. Choose another.\n";
            continue;
        }

        return choice;
    }
}

// ------------------------------------------------------------
//  Very small "AI" for single-player mode:
//   1. If the computer can win this turn, take that cell.
//   2. Else if the human is about to win, block that cell.
//   3. Else take the center, then a corner, then any free cell.
//  This is simple rule-based logic (no deep search) but plays
//  a reasonably sensible game and never crashes on a full board.
// ------------------------------------------------------------
int computeAiMove(char board[3][3], char aiSymbol, char humanSymbol) {
    // 1. Try to win
    for (int cell = 1; cell <= 9; cell++) {
        int row = (cell - 1) / 3, col = (cell - 1) % 3;
        if (board[row][col] != EMPTY) continue;
        board[row][col] = aiSymbol;
        bool wins = checkWin(board, aiSymbol);
        board[row][col] = EMPTY;
        if (wins) return cell;
    }
    // 2. Block the human's win
    for (int cell = 1; cell <= 9; cell++) {
        int row = (cell - 1) / 3, col = (cell - 1) % 3;
        if (board[row][col] != EMPTY) continue;
        board[row][col] = humanSymbol;
        bool humanWins = checkWin(board, humanSymbol);
        board[row][col] = EMPTY;
        if (humanWins) return cell;
    }
    // 3. Take the center
    if (board[1][1] == EMPTY) return 5;

    // 4. Take a corner
    int corners[4] = {1, 3, 7, 9};
    for (int c : corners) {
        int row = (c - 1) / 3, col = (c - 1) % 3;
        if (board[row][col] == EMPTY) return c;
    }

    // 5. Take any remaining free cell
    for (int cell = 1; cell <= 9; cell++) {
        int row = (cell - 1) / 3, col = (cell - 1) % 3;
        if (board[row][col] == EMPTY) return cell;
    }

    return -1; // should never happen if called with a non-full board
}

// ------------------------------------------------------------
//  Plays one full game (until win/draw). Returns nothing; all
//  win/draw messages are printed from here.
// ------------------------------------------------------------
void playOneGame(bool vsComputer) {
    char board[3][3];
    resetBoard(board);

    char currentSymbol = PLAYER_X; // X always starts
    printReferenceGrid();
    printBoard(board);

    while (true) {
        int cell;

        if (vsComputer && currentSymbol == PLAYER_O) {
            // Computer's turn
            cell = computeAiMove(board, PLAYER_O, PLAYER_X);
            cout << "Computer (O) plays cell " << cell << ".\n";
            applyMove(board, cell, PLAYER_O);
        } else {
            // Human's turn (Player X, or Player O in 2-player mode)
            string label = (currentSymbol == PLAYER_X) ? "Player X" : "Player O";
            cell = readHumanMove(board, label);
            applyMove(board, cell, currentSymbol);
        }

        printBoard(board);

        if (checkWin(board, currentSymbol)) {
            if (vsComputer && currentSymbol == PLAYER_O)
                cout << "Computer (O) wins! Better luck next time.\n";
            else
                cout << (currentSymbol == PLAYER_X ? "Player X" : "Player O") << " wins! Congratulations!\n";
            return;
        }

        if (isBoardFull(board)) {
            cout << "It's a draw! Well played both sides.\n";
            return;
        }

        // switch turns
        currentSymbol = (currentSymbol == PLAYER_X) ? PLAYER_O : PLAYER_X;
    }
}

// ------------------------------------------------------------
//  main() - mode selection + replay loop
// ------------------------------------------------------------
int main() {
    srand(static_cast<unsigned int>(time(nullptr)));

    cout << "============================================\n";
    cout << "           TIC TAC TOE\n";
    cout << "============================================\n";

    char again = 'y';
    while (again == 'y' || again == 'Y') {

        int modeChoice;
        cout << "\nSelect Game Mode:\n";
        cout << " 1. Two Players (X vs O)\n";
        cout << " 2. Single Player (X) vs Computer (O)\n";
        cout << "Enter choice (1 or 2): ";
        cin >> modeChoice;

        if (cin.fail()) {
            cin.clear();
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            modeChoice = 1;
            cout << "Invalid input, defaulting to Two Player mode.\n";
        }

        bool vsComputer = (modeChoice == 2);
        playOneGame(vsComputer);

        cout << "\nPlay again? (y/n): ";
        cin >> again;
    }

    cout << "\nThanks for playing! Goodbye.\n";
    return 0;
}
