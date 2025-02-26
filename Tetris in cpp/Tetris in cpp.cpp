#include <iostream>
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <memory>
#include <random>
#include <chrono>
#include <thread>


int rng(int min, int max) {
    static std::mt19937 gen(std::random_device{}());
    return std::uniform_int_distribution<int>{min, max}(gen);
}

class Object {
public:
    std::shared_ptr<sf::Texture> spriteTexture;
    std::shared_ptr<sf::Sprite> sprite;
    std::string filePath;

    Object(std::string imgfile, float startXpos, float startYpos, int originX = 0, int originY = 0, float scaleX = 1, float scaleY = 1) : filePath(imgfile) {
        spriteTexture = std::make_shared<sf::Texture>();

        if (!spriteTexture->loadFromFile(imgfile)) {
            std::cerr << "N�o foi poss�vel carregar a imagem: " << imgfile << std::endl;
        }

        sprite = std::make_shared<sf::Sprite>(*spriteTexture);

        sprite->setPosition({ startXpos, startYpos });


        if (originX && originY) {
            sprite->setOrigin(sf::Vector2f(static_cast<float>(originX), static_cast<float>(originY)));
        }

        sprite->setScale({ scaleX, scaleY });
    }
};

class Tile : public Object {
public:
    int xGridPos;
    int yGridPos;

    Tile(int xGrid, int yGrid)
        : Object("Sprites/empty_tile.png", xGrid * 16.0f, yGrid * 16.0f, 8, 8,2,2) { 
        xGridPos = xGrid; yGridPos = yGrid;
    }
};

class Texture {
public:
    std::shared_ptr<sf::Texture> texture;

    Texture(std::string imgfile) {
        texture = std::make_shared<sf::Texture>();

        if (!texture->loadFromFile(imgfile)) {
            std::cerr << "N�o foi poss�vel carregar a imagem: " << imgfile << std::endl;
        }
    }
};

class Sound {
public:
    sf::SoundBuffer Buffer;
    std::unique_ptr<sf::Sound> sound;
    Sound(std::string soundfile, int soundVolume = 100) {

        if (!Buffer.loadFromFile(soundfile)) {
            std::cerr << "N�o foi possivel carregar som para o arquivo: " << soundfile << std::endl;
        }

        sound = std::make_unique<sf::Sound>(Buffer);
        sound->setVolume(soundVolume);
    }
};

std::string empty_tile_path = "Sprites/empty_tile.png";
std::string tile1_path = "Sprites/tile1.png";
Texture empty_tile(empty_tile_path);
Texture tile1(tile1_path);
Texture wall("Sprites/wall.png");

Sound MoveSound("SoundEffects/move.wav");
Sound PlaceSound("SoundEffects/place.wav");
Sound RotateSound("SoundEffects/rotate.wav");
Sound ClearLineSound("SoundEffects/single.wav");
Sound TetrisClearSound("SoundEffects/tetris.wav");

std::vector<std::pair<int, int>> LShape = {
    {0, 0}, {0, 1}, {0, 2}, {1, 2}
};

std::vector<std::pair<int, int>> JShape = {
    {0, 0}, {0, 1}, {0, 2}, {-1, 2}
};

std::vector<std::pair<int, int>> OShape = {
    {0, 0}, {0, 1}, {1, 0}, {1, 1}
};

std::vector<std::pair<int, int>> IShape = {
    {0, 0}, {1, 0}, {2, 0}, {3, 0}
};

std::vector<std::pair<int, int>> TShape = {
    {0, 0}, {1, 0}, {2, 0}, {1, 1}
};

std::vector<std::pair<int, int>> ZShape = {
    {0, 1}, {1, 1}, {1, 0}, {2, 0}
};

std::vector<std::pair<int, int>> SShape = {
    {0, 0}, {1, 0}, {1, 1}, {2, 1}
};

class Piece {
public:
    int posX, posY;
    std::vector<std::pair<int, int>> pieceShape;
    std::vector<std::vector<Tile>>& tileMap;

    Piece(std::vector<std::vector<Tile>>& map, int Xpos, int Ypos, std::vector<std::pair<int, int>> shape)
        : tileMap(map), posX(Xpos), posY(Ypos), pieceShape(shape) {
        placeOnTileMap();
    }

    void clearOnTileMap() {
        for (const auto& [pieceX, pieceY] : pieceShape) {
            int x = posX + pieceX;
            int y = posY + pieceY;

            if (y >= 0 && y < tileMap.size() && x >= 0 && x < tileMap[0].size()) {
                tileMap[y][x].sprite->setTexture(*empty_tile.texture); 
                tileMap[y][x].filePath = empty_tile_path;
            }
        }
    }

    void placeOnTileMap() {
        for (const auto& [pieceX, pieceY] : pieceShape) {
            int x = posX + pieceX;
            int y = posY + pieceY;

            if (y >= 0 && y < tileMap.size() && x >= 0 && x < tileMap[0].size()) {
                tileMap[y][x].sprite->setTexture(*tile1.texture);
                tileMap[y][x].filePath = tile1_path;
            }
        }
    }

    void move(int pieceX, int pieceY) {
        clearOnTileMap();
        posX += pieceX;
        posY += pieceY;
        placeOnTileMap(); 
    }

    bool checkIfIsPartOfPiece(int newX,int newY) {
        for (const auto& [x, y] : pieceShape) {
            if (posX + x == newX && posY + y == newY) {
                return true;
            }
        }

        return false;
    }

    bool checkIfMovable(int x,int y) {
        if (x < 2 || x >= tileMap[0].size() - 1 || y < 0 || y >= tileMap.size()) {
            return false;
        }

        if (tileMap[y][x].filePath != empty_tile_path) {
            bool isPartOfPiece = checkIfIsPartOfPiece(x, y);
            if (!isPartOfPiece) {
                return false;
            }
        }

        return true;
    }

    bool canMoveDown() {
        for (const auto& [dx, dy] : pieceShape) {
            int x = posX + dx;
            int y = posY + dy;
            int newY = y + 1; 

            bool checkMove = checkIfMovable(x, newY);
            if (!checkMove) { return false; }
        }

        return true;
    }

    bool canMoveSideways(int direction) {  
        for (const auto& [pieceX, pieceY] : pieceShape) {
            // o pieceX � a posi��o relativa de cada tile dentro do shape
            // ent�o o currentX � a posi��o da peca (tilemap) somada a posi��o do tile
            // assim dando a posi��o do tile da peca no tile map

            int currentX = posX + pieceX;
            int currentY = posY + pieceY;
            int newX = currentX + direction;

            bool checkMove = checkIfMovable(newX, currentY);
            if (!checkMove) { return false; }
        }

        return true;
    }

    void rotate() {
        std::vector<std::pair<int, int>> originalShape = pieceShape;
        
        int minX = INT_MAX, minY = INT_MAX, maxX = INT_MIN, maxY = INT_MIN;
        for (const auto& [x, y] : pieceShape) {
            if (x < minX) minX = x;
            if (y < minY) minY = y;
            if (x > maxX) maxX = x;
            if (y > maxY) maxY = y;
        }
        int originX = (minX + maxX) / 2;
        int originY = (minY + maxY) / 2;

        std::vector<std::pair<int, int>> rotatedShape;
        for (const auto& [x, y] : pieceShape) {
            int newX = originX - (y - originY);
            int newY = originY + (x - originX);
            rotatedShape.push_back({ newX, newY });
        }

        clearOnTileMap();
        pieceShape = rotatedShape;

        if (!isRotationValid()) {
            pieceShape = originalShape;
        }

        placeOnTileMap();
        RotateSound.sound->play();
    }

    bool isRotationValid() {
        for (const auto& [pieceX, pieceY] : pieceShape) {
            int x = posX + pieceX;
            int y = posY + pieceY;

            bool checkRotation = checkIfMovable(x, y);
            if (!checkRotation) { return false; }
        }
        return true;
    }
    
    std::vector<std::pair<int, int>> getTiles() {
        std::vector<std::pair<int, int>> tiles;

        for (const auto& [pieceX, pieceY] : pieceShape) {
            int currentX = posX + pieceX;
            int currentY = posY + pieceY;

            tiles.push_back({ currentX, currentY });
        }

        return tiles;
    };
};

void renderTiles(sf::RenderWindow& window, std::vector<std::vector<Tile>>& tileMap) {
    // Loop pelos tiles depois de abrir a janela
    for (const auto& row : tileMap) {
        for (auto& tile : row) {
            window.draw(*tile.sprite);
        }
    }
}

std::vector<int> checkCompletedLines(int startY, int endY, int columns, const std::vector<std::vector<Tile>>& tileMap) {
    std::vector<int> completedLines;

    for (int y = endY; y >= startY; y--) {
        bool lineCompleted = true;

        for (int x = 2; x < columns - 1; x++) {
            if (tileMap[y][x].filePath == empty_tile_path) {
                lineCompleted = false;
                break;
            }
        }

        if (lineCompleted) {
            completedLines.push_back(y);
        }
    }

    return completedLines;
}

void clearLines(const std::vector<int>& lines, int columns, std::vector<std::vector<Tile>>& tileMap, sf::RenderWindow& window) {
    if (lines.empty()) return;

    if (lines.size() == 4) {
        TetrisClearSound.sound->play();
    }
    else {
        ClearLineSound.sound->play();
    }

    for (int x = 2; x < columns - 1; x++) {
        for (int lineIndex : lines) {
            tileMap[lineIndex][x].filePath = empty_tile_path;
            tileMap[lineIndex][x].sprite->setTexture(*empty_tile.texture);
        }

        window.clear();
        renderTiles(window, tileMap);
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        window.display();
    }
}

void makeBlocksFall(const std::vector<int>& clearedLines, int columns, std::vector<std::vector<Tile>>& tileMap) {
    if (clearedLines.empty()) return;

    std::vector<int> dropAmount(tileMap.size(), 0);

    for (int y = 0; y < tileMap.size(); y++) {
        for (int clearedY : clearedLines) {
            if (clearedY > y) {
                dropAmount[y]++;
            }
        }
    }

    for (int y = tileMap.size() - 1; y >= 0; y--) {
        if (std::find(clearedLines.begin(), clearedLines.end(), y) != clearedLines.end() ||
            dropAmount[y] == 0) {
            continue;
        }

        int newY = y + dropAmount[y];

        if (newY < tileMap.size()) {
            for (int x = 2; x < columns - 1; x++) {
                tileMap[newY][x].filePath = tileMap[y][x].filePath;

                if (tileMap[y][x].filePath != empty_tile_path) {
                    tileMap[newY][x].sprite->setTexture(*tile1.texture);
                }
                else {
                    tileMap[newY][x].sprite->setTexture(*empty_tile.texture);
                }

                tileMap[y][x].filePath = empty_tile_path;
                tileMap[y][x].sprite->setTexture(*empty_tile.texture);
            }
        }
    }
}

int main() {
    int normal_tickrate = 20;
    int fast_tickrate = normal_tickrate / 4;
    int tickrate = normal_tickrate;
    int tick = 5;

    const int width = 176;   
    const int height = 336;  

    int colums = 12;    
    int rows = 21;    

    std::vector<std::vector<Tile>> tileMap;

    for (int y = 0; y < rows; y++) {
        std::vector<Tile> row;
        for (int x = 0; x < colums; x++) {
            Tile tile(x, y);
            row.emplace_back(tile);
        }
        tileMap.push_back(row);
    }
    
    // Loop pelos tiles antes de abrir a janela
    for (const auto& row : tileMap) {
        for (auto& tile : row) {
            if (tile.xGridPos == 1 || tile.xGridPos == colums-1 || tile.yGridPos == 0 || tile.yGridPos == rows) {
                tile.sprite->setTexture(*wall.texture);
            }
        }
    }

    std::unique_ptr<Piece> MainPiece = std::make_unique<Piece>(tileMap, 5, 1, TShape);
   
    std::unique_ptr window = std::make_unique<sf::RenderWindow>(sf::VideoMode({ width, height }), "Tetris");

    window->setFramerateLimit(60);

    while (window->isOpen()) {
        // Os tiles das bordas sempre devem ficar vazios
        // Tiles us�veis: x de 1 a 10 e y de 1 a 20

        tick -= 1;

        while (const std::optional event = window->pollEvent()) {

            if (event->is<sf::Event::Closed>()) {
                window->close();
            }

            else if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
                if (keyPressed->scancode == sf::Keyboard::Scancode::Escape) {
                    window->close();
                    return 0;
                }
                else if (keyPressed->scancode == sf::Keyboard::Scancode::Left) {
                    if (MainPiece->canMoveSideways(-1)) {
                        MoveSound.sound->play();
                        MainPiece->move(-1, 0);
                    }
                    
                }
                else if (keyPressed->scancode == sf::Keyboard::Scancode::Right) {
                    if (MainPiece->canMoveSideways(1)) {
                        MoveSound.sound->play();
                        MainPiece->move(1, 0);
                    }

                }
                else if (keyPressed->scancode == sf::Keyboard::Scancode::Up) {
                    MainPiece->rotate();
                }
            }
        }

        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) {
            tickrate = fast_tickrate;
        }
        else {
            tickrate = normal_tickrate;
        }

        if (tick == 0) {
            if (!MainPiece->canMoveDown()) {
                MainPiece->move(0, 0);
                int chosePiece = rng(1, 7);

                std::vector<std::pair<int, int>> tilesOfPiece = MainPiece->getTiles();
                
                std::vector<int> completedLines = checkCompletedLines(1, rows - 1, colums, tileMap);
                if (!completedLines.empty()) {
                    clearLines(completedLines, colums, tileMap, *window);
                    makeBlocksFall(completedLines, colums, tileMap);
                }

                PlaceSound.sound->play();

                switch (chosePiece) {
                case 1:
                    MainPiece.reset(new Piece(tileMap, 5, 1, TShape));
                    break;
                case 2:
                    MainPiece.reset(new Piece(tileMap, 5, 1, IShape));
                    break;
                case 3:
                    MainPiece.reset(new Piece(tileMap, 5, 1, LShape));
                    break;
                case 4:
                    MainPiece.reset(new Piece(tileMap, 5, 1, OShape));
                    break;
                case 5:
                    MainPiece.reset(new Piece(tileMap, 5, 1, ZShape));
                    break;
                case 6:
                    MainPiece.reset(new Piece(tileMap, 5, 1, SShape));
                    break;
                case 7:
                    MainPiece.reset(new Piece(tileMap, 5, 1, JShape));
                    break;
                }
   
            }
            else {
                MainPiece->move(0, 1);
            }
            tick = tickrate;
        }

        window->clear();

        renderTiles(*window, tileMap);
        
        window->display();
    }
    return 0;
}