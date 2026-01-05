// main.cpp
// Simple Snake game using SFML
// Build with: cmake .. && make   (or use g++ + pkg-config)

#include <SFML/Graphics.hpp>
#include <vector>
#include <deque>
#include <random>
#include <string>
#include <chrono>

using Vec2i = sf::Vector2i;

struct GameConfig {
    int cellSize = 20;
    int cols = 32; // grid width
    int rows = 24; // grid height
    float moveInterval = 0.12f; // seconds per move (smaller => faster)
};

class Snake {
public:
    std::deque<Vec2i> body; // front is head
    Vec2i dir{1, 0};
    bool growNext = false;

    Snake(Vec2i start, int initialLength = 4) {
        for (int i = 0; i < initialLength; ++i) {
            body.push_back({start.x - i, start.y});
        }
    }

    Vec2i head() const { return body.front(); }

    void setDirection(const Vec2i& d) {
        // prevent reversing
        if (body.size() > 1 && d == Vec2i(-dir.x, -dir.y)) return;
        dir = d;
    }

    void move() {
        Vec2i newHead = head() + dir;
        body.push_front(newHead);
        if (!growNext) body.pop_back();
        growNext = false;
    }

    void grow() { growNext = true; }

    bool collidesWithSelf() const {
        Vec2i h = head();
        for (size_t i = 1; i < body.size(); ++i)
            if (body[i] == h) return true;
        return false;
    }

    bool occupies(const Vec2i &p) const {
        for (auto &s: body) if (s == p) return true;
        return false;
    }
};

struct RNG {
    std::mt19937 gen;
    RNG() : gen((unsigned)std::chrono::high_resolution_clock::now().time_since_epoch().count()) {}
    int nextInt(int a, int b) { std::uniform_int_distribution<int> d(a, b); return d(gen); }
};

int main() {
    GameConfig cfg;
    // Adjust resolution for retina / scaling if desired
    int windowW = cfg.cellSize * cfg.cols;
    int windowH = cfg.cellSize * cfg.rows;
    sf::RenderWindow window(sf::VideoMode(sf::Vector2u(windowW, windowH)), "SFML Snake");
    window.setFramerateLimit(120);

    // Prepare grid rectangles
    sf::RectangleShape cellShape(sf::Vector2f((float)cfg.cellSize - 1.0f, (float)cfg.cellSize - 1.0f));
    cellShape.setOutlineThickness(0);

    // Start snake centered
    Vec2i startPos(cfg.cols / 2, cfg.rows / 2);
    Snake snake(startPos, 5);

    RNG rng;
    Vec2i food;
    auto placeFood = [&](void) {
        while (true) {
            int x = rng.nextInt(0, cfg.cols - 1);
            int y = rng.nextInt(0, cfg.rows - 1);
            if (!snake.occupies({x, y})) { food = {x, y}; break; }
        }
    };
    placeFood();

    int score = 0;
    bool paused = false;
    bool gameOver = false;

    sf::Font font;
    // Use SFML default fallback if no font; try to load an OS font for nicer text.
    if (!font.openFromFile("/System/Library/Fonts/Supplemental/Arial.ttf")) {
        // fallback: try a generic one (may fail on some setups)
        (void)font.openFromFile("/Library/Fonts/Arial.ttf");
    }

    sf::Text scoreText(font);
    scoreText.setCharacterSize(18);
    scoreText.setPosition(sf::Vector2f(8.f, 4.f));

    sf::Text infoText(font);
    infoText.setCharacterSize(20);
    infoText.setPosition(sf::Vector2f(8.f, (float)(windowH - 28)));

    sf::Clock moveClock;
    float acc = 0.f;

    // Main loop
    while (window.isOpen()) {
        // --- Events ---
        while (const auto event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) window.close();
            if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->code == sf::Keyboard::Key::Escape) window.close();
                if (keyPressed->code == sf::Keyboard::Key::P) paused = !paused;
                if (keyPressed->code == sf::Keyboard::Key::R) {
                    // restart
                    snake = Snake(startPos, 5);
                    placeFood();
                    score = 0;
                    paused = false;
                    gameOver = false;
                    moveClock.restart();
                }
                if (!gameOver) {
                    if (keyPressed->code == sf::Keyboard::Key::Up || keyPressed->code == sf::Keyboard::Key::W) snake.setDirection({0, -1});
                    if (keyPressed->code == sf::Keyboard::Key::Down || keyPressed->code == sf::Keyboard::Key::S) snake.setDirection({0, 1});
                    if (keyPressed->code == sf::Keyboard::Key::Left || keyPressed->code == sf::Keyboard::Key::A) snake.setDirection({-1, 0});
                    if (keyPressed->code == sf::Keyboard::Key::Right || keyPressed->code == sf::Keyboard::Key::D) snake.setDirection({1, 0});
                }
            }
        }

        // --- Update ---
        if (!paused && !gameOver) {
            float dt = moveClock.restart().asSeconds();
            acc += dt;
            // handle movement at fixed interval
            while (acc >= cfg.moveInterval) {
                acc -= cfg.moveInterval;
                snake.move();

                // boundary collision
                Vec2i h = snake.head();
                if (h.x < 0 || h.x >= cfg.cols || h.y < 0 || h.y >= cfg.rows) {
                    gameOver = true;
                    break;
                }
                // self collision
                if (snake.collidesWithSelf()) {
                    gameOver = true;
                    break;
                }

                // food?
                if (snake.head() == food) {
                    snake.grow();
                    score += 10;
                    placeFood();
                    // optional speed up slightly every X points
                    if (score % 50 == 0 && cfg.moveInterval > 0.04f) cfg.moveInterval *= 0.92f;
                }
            }
        } else {
            // if paused or gameOver, still reset the moveClock to avoid jump when unpausing
            moveClock.restart();
        }

        // --- Render ---
        window.clear(sf::Color(30, 30, 30));

        // draw grid background (optional faint checker)
        for (int x = 0; x < cfg.cols; ++x) {
            for (int y = 0; y < cfg.rows; ++y) {
                cellShape.setPosition(sf::Vector2f((float)x * cfg.cellSize, (float)y * cfg.cellSize));
                if ((x + y) % 2 == 0) cellShape.setFillColor(sf::Color(38, 38, 38));
                else cellShape.setFillColor(sf::Color(34, 34, 34));
                window.draw(cellShape);
            }
        }

        // draw food
        sf::RectangleShape foodShape(sf::Vector2f((float)cfg.cellSize - 2.f, (float)cfg.cellSize - 2.f));
        foodShape.setPosition(sf::Vector2f((float)food.x * cfg.cellSize + 1.f, (float)food.y * cfg.cellSize + 1.f));
        foodShape.setFillColor(sf::Color(200, 40, 40));
        window.draw(foodShape);

        // draw snake
        sf::RectangleShape segShape(sf::Vector2f((float)cfg.cellSize - 2.f, (float)cfg.cellSize - 2.f));
        bool headDrawn = false;
        for (size_t i = 0; i < snake.body.size(); ++i) {
            Vec2i s = snake.body[i];
            segShape.setPosition(sf::Vector2f((float)s.x * cfg.cellSize + 1.f, (float)s.y * cfg.cellSize + 1.f));
            if (i == 0) {
                segShape.setFillColor(sf::Color(120, 220, 120)); // head
                window.draw(segShape);
                headDrawn = true;
            } else {
                segShape.setFillColor(sf::Color(80, 180, 80));
                window.draw(segShape);
            }
        }

        // text
        scoreText.setString("Score: " + std::to_string(score));
        window.draw(scoreText);

        if (paused) {
            infoText.setString("[P] Resume  [R] Restart  [Esc] Quit  (Paused)");
            window.draw(infoText);
        } else if (gameOver) {
            sf::Text goText(font);
            goText.setCharacterSize(36);
            goText.setString("Game Over");
            goText.setPosition(sf::Vector2f(windowW / 2.f - goText.getGlobalBounds().size.x / 2.f, windowH / 2.f - 40.f));
            window.draw(goText);

            infoText.setString("[R] Restart  [Esc] Quit");
            window.draw(infoText);
        } else {
            infoText.setString("[Arrows / WASD] Move  [P] Pause  [R] Restart  [Esc] Quit");
            window.draw(infoText);
        }

        window.display();
    }

    return 0;
}