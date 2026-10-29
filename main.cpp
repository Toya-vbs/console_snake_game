#include <algorithm>
#include <chrono>
#include <conio.h>
#include <iostream>
#include <random>
#include <string>
#include <vector>
#include <windows.h>

namespace {

constexpr int kWidth = 40;
constexpr int kHeight = 20;
constexpr int kFrameTimeMs = 120;

struct Point {
    int x;
    int y;

    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
};

/**
 * @brief four direction
 */
enum class Direction { Up, Down, Left, Right };

class SnakeGame {
public:
    SnakeGame()
        : randomEngine_(std::random_device{}()) {
        restart();
    }

    void run() {
        setupConsole();

        while (!quit_) {
            const auto frameStart = std::chrono::steady_clock::now();
            handleInput();
            if (!gameOver_) {
                update();
            }
            draw();

            const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::steady_clock::now() - frameStart);
            if (elapsed.count() < kFrameTimeMs) {
                Sleep(kFrameTimeMs - static_cast<DWORD>(elapsed.count()));
            }
        }

        restoreConsole();
    }

private:
    std::vector<Point> snake_;
    Point food_{};
    Direction direction_ = Direction::Right;
    Direction nextDirection_ = Direction::Right;
    int score_ = 0;
    bool gameOver_ = false;
    bool quit_ = false;
    std::mt19937 randomEngine_;
    HANDLE outputHandle_ = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO oldCursorInfo_{};

    void restart() {
        snake_ = {{kWidth / 2, kHeight / 2},
                  {kWidth / 2 - 1, kHeight / 2},
                  {kWidth / 2 - 2, kHeight / 2}};
        direction_ = Direction::Right;
        nextDirection_ = Direction::Right;
        score_ = 0;
        gameOver_ = false;
        placeFood();
    }

    void setupConsole() {
        GetConsoleCursorInfo(outputHandle_, &oldCursorInfo_);
        CONSOLE_CURSOR_INFO cursorInfo = oldCursorInfo_;
        cursorInfo.bVisible = FALSE;
        SetConsoleCursorInfo(outputHandle_, &cursorInfo);
        std::cout << "\x1b[2J";
    }

    void restoreConsole() const {
        SetConsoleCursorInfo(outputHandle_, &oldCursorInfo_);
        COORD position{0, static_cast<SHORT>(kHeight + 4)};
        SetConsoleCursorPosition(outputHandle_, position);
        std::cout << "\nSee you again~\n";
    }

    static bool isOpposite(Direction first, Direction second) {
        return (first == Direction::Up && second == Direction::Down) ||
               (first == Direction::Down && second == Direction::Up) ||
               (first == Direction::Left && second == Direction::Right) ||
               (first == Direction::Right && second == Direction::Left);
    }

    void changeDirection(Direction requestedDirection) {
        if (!isOpposite(direction_, requestedDirection)) {
            nextDirection_ = requestedDirection;
        }
    }

    void handleInput() {
        while (_kbhit()) {
            const int key = _getch();
            if (key == 0 || key == 224) {
                const int arrow = _getch();
                if (arrow == 72) changeDirection(Direction::Up);
                if (arrow == 80) changeDirection(Direction::Down);
                if (arrow == 75) changeDirection(Direction::Left);
                if (arrow == 77) changeDirection(Direction::Right);
                continue;
            }

            switch (key) {
                case 'w': case 'W': changeDirection(Direction::Up); break;
                case 's': case 'S': changeDirection(Direction::Down); break;
                case 'a': case 'A': changeDirection(Direction::Left); break;
                case 'd': case 'D': changeDirection(Direction::Right); break;
                case 'r': case 'R': if (gameOver_) restart(); break;
                case 'q': case 'Q': quit_ = true; break;
                default: break;
            }
        }
    }

    void update() {
        direction_ = nextDirection_;
        Point head = snake_.front();
        if (direction_ == Direction::Up) --head.y;
        if (direction_ == Direction::Down) ++head.y;
        if (direction_ == Direction::Left) --head.x;
        if (direction_ == Direction::Right) ++head.x;

        const bool hitsWall = head.x < 0 || head.x >= kWidth ||
                              head.y < 0 || head.y >= kHeight;
        const bool ateFood = head == food_;
        const std::size_t bodyEnd = snake_.size() - (ateFood ? 0 : 1);
        const bool hitsBody = std::any_of(snake_.begin(), snake_.begin() + bodyEnd,
            [&head](const Point& segment) { return segment == head; });

        if (hitsWall || hitsBody) {
            gameOver_ = true;
            return;
        }

        snake_.insert(snake_.begin(), head);
        if (ateFood) {
            ++score_;
            placeFood();
        } else {
            snake_.pop_back();
        }
    }

    void placeFood() {
        std::uniform_int_distribution<int> xDistribution(0, kWidth - 1);
        std::uniform_int_distribution<int> yDistribution(0, kHeight - 1);

        do {
            food_ = {xDistribution(randomEngine_), yDistribution(randomEngine_)};
        } while (std::find(snake_.begin(), snake_.end(), food_) != snake_.end());
    }

    void draw() const {
        std::vector<std::string> board(kHeight, std::string(kWidth, ' '));
        board[food_.y][food_.x] = '*';
        for (std::size_t i = 0; i < snake_.size(); ++i) {
            board[snake_[i].y][snake_[i].x] = (i == 0 ? '@' : 'o');
        }

        std::string screen = "SNAKE  score: " + std::to_string(score_) +
            " use arrow keys/WASD to move   Q to quit\n";
        screen += '+' + std::string(kWidth, '-') + "+\n";
        for (const auto& row : board) {
            screen += '|' + row + "|\n";
        }
        screen += '+' + std::string(kWidth, '-') + "+\n";
        if (gameOver_) {
            screen += "Game over! Press R to play again, or press Q to quit.\n";
        }

        COORD topLeft{0, 0};
        SetConsoleCursorPosition(outputHandle_, topLeft);
        std::cout << screen << std::flush;
    }
};

}  // namespace

int main() {
    SnakeGame game;
    game.run();
    return 0;
}
