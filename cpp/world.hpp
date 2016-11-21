#include <stdlib.h>
#include <tuple>


class Player;
class Room;
class Tile {
    private:
        bool empty;
        bool wall;
    public:
        Tile(bool e, bool w);
        bool isWall();
        bool isEmpty();
        char represent();
};
typedef Tile * TilePtr ;

class World {
    private:
        int width;
        int height;
        TilePtr * array;
        Tile wall;
        Tile empty;
        Tile occupied;
        int * players;
        bool fromXML(std::string, bool);
    public:
        World( int w, int h );
        TilePtr ** getMatrix();
        int removeMatrix(TilePtr ** matrix);
        int addTile(Tile * tile, std::pair<int,int> position);
        Tile * getTile(std::pair<int,int> position);
        bool movePlayer( Player * player, std::pair<int, int> position);
        Tile * getEmpty();
        Tile * getWall();
        Tile * getOccupied();
        int getWidth();
        int getHeight();
        void * getXML();
        std::string getXMLAsString();
        bool createFromXML(std::string);
        bool updateFromXML(std::string);
};

bool buildRooms( World *, int, int, int, int, int );

class CorridorBuilder {
    private:
        World * world;
    public:
        CorridorBuilder( World * );
        CorridorBuilder();
        bool generate();
        bool createMaze(std::pair<int,int>);
        bool cleanUp();
        bool roomsOverlap(Room *, Room *);
        bool canCarve(std::pair<int,int>, std::pair<int,int>);
        std::pair<int,int> carve(std::pair<int,int>, std::pair<int,int>);
};
