//World File

#include <stdio.h>
#include <stdlib.h>
#include <tuple>

#include "world.hpp"
#include "player.hpp"

#include <libxml/parser.h>
#include <libxml/tree.h>

//DEFINE DIRECTIONS
const std::pair<int, int> UP = std::pair<int,int>(-1,0);
const std::pair<int, int> LEFT = std::pair<int,int>(0,-1);
const std::pair<int, int> DOWN = std::pair<int,int>(1,0);
const std::pair<int, int> RIGHT = std::pair<int,int>(0,1);
const std::pair<int, int> DIRECTIONS[] = { UP, LEFT, DOWN, RIGHT };

//Representations for tile types. should maybe be an enum?
const char EMPTY  = ' ';
const char WALL   = '#';
const char PLAYER = '$';

int rand_lim(int limit) {
/* return a random number between 0 and limit inclusive.
 */

    int divisor = RAND_MAX/(limit+1);
    int retval;

    do {
        retval = rand() / divisor;
    } while (retval > limit);

    return retval;
}


//This is fucking aweful. Why can't I just recreate the fucking class instead of this weird/terrible namespace thing with class definitions in the header files.
//this is why nobody likes c++.

// Tile Class Decleration.
Tile::Tile(bool e, bool w) : empty(e), wall(w)
{};

bool Tile::isWall() {
	return this->wall;
};

bool Tile::isEmpty() {
	return this->empty;
};

char Tile::represent()
{
    if ( this->empty && ! this->wall )
        return EMPTY;
    else if ( this->wall )
        return WALL;
    return PLAYER;
};


#if defined(LIBXML_TREE_ENABLED) && defined(LIBXML_OUTPUT_ENABLED)
// World Class func declerations
World::World(int w, int h): width(w), height(h), array(new TilePtr[width*height]), wall(true, true), empty(true, false), occupied(false, false)
{
    for (int i=0; i < (this->width*this->height); i++)
    {
        array[i] = &wall;
    }
};
        
TilePtr ** World::getMatrix()
{
    TilePtr **matrix = new TilePtr*[height];
    for (int y=0; y < height; y++)
    {
        matrix[y] = new TilePtr[width];
        for (int x=0; x < width; x++)
        {
            //fprintf(stdout, "X[%i] Y[%i] POS[%i]\n", x, y, (y*width)+x);
            matrix[y][x] = array[(y*width)+x];
        }
    }
    return matrix;
};

int World::removeMatrix(TilePtr ** matrix)
{ //This doesn't alter the world. just free's memory.
    for (int row=0; row < this->height; row++)
    {
        delete [] matrix[row];
    }
    delete [] matrix;
    return true;
};

void * World::getXML()
{
    xmlDocPtr doc = NULL;
    xmlNodePtr root_node = NULL, node = NULL, contents = NULL;

    LIBXML_TEST_VERSION;
    doc = xmlNewDoc(BAD_CAST "1.0");

    //create our world.
    root_node = xmlNewNode(NULL, BAD_CAST "world");    
    xmlNewProp(root_node, BAD_CAST "width", BAD_CAST std::to_string(this->width).c_str() );
    xmlNewProp(root_node, BAD_CAST "height", BAD_CAST std::to_string(this->height).c_str() );
    xmlDocSetRootElement(doc, root_node);

    //Player Details.
    node = xmlNewChild(root_node, NULL, BAD_CAST "players", NULL);
    xmlNewProp(node, BAD_CAST "count", BAD_CAST std::to_string(0).c_str() );

    //Room Details
    node = xmlNewChild(root_node, NULL, BAD_CAST "rooms", NULL);
    xmlNewProp(node, BAD_CAST "count", BAD_CAST std::to_string(0).c_str() );

    //Contents
    contents = xmlNewChild(root_node, NULL, BAD_CAST "contents", NULL);
    TilePtr ** matrix = this->getMatrix();
    for (int h=0; h < this->getHeight(); h++)
    {
        node = xmlNewChild(contents, NULL, BAD_CAST "row", NULL);
        for (int w=0; w < this->getWidth(); w++)
        {
            std::string item_type;
            switch (matrix[w][h]->represent())
            {
                case WALL:
                    xmlNewChild(node, NULL, BAD_CAST "wall", NULL);
                    break;
                case EMPTY:
                    xmlNewChild(node, NULL, BAD_CAST "basetile", NULL);
                    break;
           } 
        };
    } 
    
    xmlBufferPtr buff = xmlBufferCreate();
    xmlNodeDump(buff, doc, NULL, 4, 1);
    printf("%s\n", buff->content);
    return NULL;
}

std::string World::getXMLAsString()
{

    return NULL;
}

bool World::fromXML(std::string xml, bool hard_reset)
{

    return false;
}


int World::addTile(Tile * tile, std::pair<int, int> pos)
{
    // Should always be X, Y.
    // When referenced in the matrix, it's Y, X.
    array[(pos.second*this->width)+pos.first] = tile;
    return true;
};

Tile * World::getTile(std::pair<int,int> position)
{
    return this->array[(position.second*this->width)+position.first];
};

bool World::movePlayer(Player * player, std::pair<int,int> position)
{
    bool success = false;
    Tile * cur_tile = array[(position.second*this->width)+position.first];
    if (cur_tile->isEmpty() && !cur_tile->isWall())
    {
        std::pair<int,int> cur_pos = player->getPosition();
        array[(cur_pos.second*width)+cur_pos.first] =&empty;
        array[(position.second*width)+position.first] =&occupied;
        success = true;
    }
    return success;
}

Tile * World::getEmpty()
{
       return &empty;
};

Tile * World::getWall()
{
    return &wall;
};

Tile * World::getOccupied()
{
    return &occupied;
}

int World::getWidth()
{
    return width;
};

int World::getHeight()
{
    return height;
};
#endif

//Player Class Func Declerations.

Player::Player(int new_number, std::pair<int,int> position) : number(new_number), x(position.first), y(position.second)
{};

bool Player::setPosition(std::pair<int,int> new_position)
{
	x = new_position.first;
	y = new_position.second;
	return true;
};

std::pair<int,int> Player::getPosition()
{
	return std::make_pair(x, y);
}

bool Player::move(std::pair<int,int> position)
{
	bool success = myWorld->movePlayer(this, position);
	if (success) {
		this->setPosition(position);
	}
	return success;
}


// ughhh. whatever. Fuck all the things.
class Room
{
    private:
        World * myWorld;
        int width;
        int height;
        int x;
        int y;
        std::pair<int,int> * walls;
        std::pair<int,int>  corners[4];
        int ** vectors; //not actually a vector.
        std::pair<int,int> center;

    public:
        Room(World * world, int w, int h, int new_x, int new_y) : myWorld(world), width(w), height(h), x(new_x), y(new_y), walls(new std::pair<int, int>[(width*height)])
        {
        };

        bool build(bool build)
        { //FIXED: inverted the dryrun value (now build). it makes more sense for room.build(true) to actually build, rather than room.build(false) be the build.
            int i = 0;
            for (int col=x; col < (x+width+1); col++)
            {
                for (int row=y; row < (y+height+1); row++)
                {
                    walls[i] = std::pair<int, int>(col, row);
                    ++i;
                }
            }
            if (build)
            {
                // Alter the world.
                for (int i=0; i < ((width+1)*(height+1)); i++)
                {
                    //fprintf(stdout, "Carving position %i\n", ((walls[i].first+1)*(walls[i].second+1)-(myWorld->getHeight()+myWorld->getWidth())));
                    myWorld->addTile(myWorld->getEmpty(),walls[i]); 
                }
            }
            corners[0] = std::pair<int,int>(x,y); // Top Left
            corners[1] = std::pair<int,int>(x+width, y); // Top Right
            corners[2] = std::pair<int,int>(x,y+height); // bottom Left
            corners[3] = std::pair<int,int>(x+width, y+height); // bottom right

            center = std::pair<int, int>( (int) ((corners[0].first + corners[3].first)/2), (int) ((corners[0].second + corners[3].second)/2) );

            return true;
        };

        bool dryrun()
        {
            return this->build(false);
        }

        std::pair<int, int> * getWalls()
        {
            return walls;
        }

        std::pair<int, int> getPosition()
        {
            return std::pair<int, int>(x, y);
        }

        int getWidth()
        {
            return width;
        }
        
        int getHeight()
        {
            return height;
        };
};


// Function that builds rooms. 
bool buildRooms(World * world, int max_width, int max_height, int max_rooms, int min_width, int min_height)
{
    if ( ! world )
    {
        return false;
    }

    srand(time(NULL)); // Seed the random.

    int success_count = 0;
    while ( success_count < max_rooms )
    {
        int width  = rand() % (max_width - min_width  ) + min_width ;
        int height = rand() % (max_height - min_height) + min_height;
        
        int x = rand() % (world->getWidth()-(width+1));
        int y = rand() % (world->getHeight() - (height+1));
        bool can_build = true;
        for (int col=x; col < (x+width+1); ++col)
        {
            for (int row=y; row < (y+height+1); ++row)
            {
                Tile * tile = world->getTile(std::pair<int,int>(col,row));
                if ( tile == NULL || !tile->isEmpty() || ! tile->isWall() ) {
                    //something already occupies this space (a player or another room).
                    can_build = false;
                    break;
                }
            }
            if (! can_build )
            {
                break;
            }
        }
        if ( can_build )
        {
            Room room = Room(world, width, height, x, y);
            room.build(true);
            success_count += 1;
        }
    }

    return true;
}

CorridorBuilder::CorridorBuilder(World * w) : world(w)
{};
CorridorBuilder::CorridorBuilder()
{};

bool CorridorBuilder::generate() {
    if ( ! world ) {
        return false;
    }
    bool res;
    res = this->createMaze(std::pair<int,int>(1,1));
    if ( res )
        res = this->cleanUp();
    return res;
}

bool CorridorBuilder::roomsOverlap(Room * room1, Room * room2)
{
    if ( (room1->getPosition().first + room1->getWidth() + 1) < room2->getPosition().first )
        return false;
    else if ( room1->getPosition().first > (room2->getPosition().first + room2->getWidth() + 1) )
        return false;
    else if ( (room1->getPosition().second + room1->getHeight() + 1) < room2->getPosition().second )
        return false;
    else if ( room1->getPosition().second > (room2->getPosition().second + room2->getHeight() +1) )
        return false;

    return false;
}

bool CorridorBuilder::createMaze(std::pair<int, int> start_pos)
{
    int free_cell_count = 0;
    int windingPercent = 15;
    std::pair<int,int> * cells = new std::pair<int,int>[world->getWidth() * world->getHeight()]();
    int last_dir = -1;
    cells[free_cell_count] = start_pos;
    srand(time(NULL)); // Seed the random.

    while (free_cell_count >= 0)
    {
        std::pair<int,int> cell = cells[free_cell_count];
        int * unmade_cells = new int[4]();
        bool no_cells = true;
        for (int direction =0; direction <= 3; direction++)
        {
            unmade_cells[direction] = -1;
            if ( this->canCarve(cell, DIRECTIONS[direction]) )
            {
                unmade_cells[direction] = true;
                no_cells = false;
            }
        }
        int direction = -1;
        if ( ! no_cells )
        {
            int change = rand() % 100;
            if ( (last_dir >= 0 && unmade_cells[last_dir]) && (change > windingPercent))
            {
                direction = last_dir;
            } else {
                int d = -1;
                while ( d == -1 )
                {
                    direction = rand_lim(3);
                    d = unmade_cells[direction];
                };
            }

            this->carve(cell, DIRECTIONS[direction]);
            this->carve(cell, std::pair<int,int>(DIRECTIONS[direction].first * 2, DIRECTIONS[direction].second * 2));
            free_cell_count += 1;
            cells[free_cell_count] = std::pair<int,int>(cell.first + DIRECTIONS[direction].first * 2, cell.second + DIRECTIONS[direction].second * 2);
            last_dir = direction;
        } else {
            free_cell_count -= 1;
            last_dir = -1;
        }
    }

    return true;
}

bool CorridorBuilder::canCarve(std::pair<int,int> cell, std::pair<int,int> direction)
{
    cell.first = cell.first + direction.first*2;
    cell.second = cell.second + direction.second*2; 
    if ( cell.first >= world->getWidth() || cell.second >= world->getHeight() || cell.first <= 0 || cell.second <= 0 )
        return false;
    return world->getTile(cell)->isWall();
};

std::pair<int, int> CorridorBuilder::carve(std::pair<int,int> cell, std::pair<int,int> direction)
{
    cell.first = cell.first + direction.first;
    cell.second = cell.second + direction.second;
    world->addTile( world->getEmpty(), cell);
    return cell;
};

bool CorridorBuilder::cleanUp() {
    bool done = false;
    while (! done )
    {
        done = true;
        for (int x=1; x<world->getWidth(); x++)
        {
            for (int y=1; y<world->getHeight(); y++)
            {
                if ( this->world->getTile(std::pair<int,int>(x,y))->isWall() )
                    continue;

                int open_sides = 0;
                for (int i=0; i <= 3; i++)
                {
                    std::pair<int,int> d = DIRECTIONS[i];
                    if ( ((x+d.first) < 0) || ((y+d.second) < 0) || ((x+d.first) >= this->world->getWidth()) || ((y+d.second) >= this->world->getHeight()) )
                        continue;
                    if (! this->world->getTile(std::pair<int,int>(x+d.first, y+d.second))->isWall() )
                    {
                        open_sides += 1;
                    }
                    
                }
                if (open_sides == 1)
                {
                    done = false;
                    this->world->addTile(this->world->getWall(), std::pair<int,int>(x,y));
                }
            }
        }
    }
    return done;
}
