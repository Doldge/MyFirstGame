#include <stdlib.h>
#include <tuple>

class World;

class Player {
    private:
        int number;
        int x;
        int y;
        World * myWorld;
    public:
        Player(int n, std::pair<int, int> pos );
        bool setPosition(std::pair<int, int> position);
        std::pair<int, int> getPosition();
        bool move(std::pair<int, int> position);
};
