#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>

#include <string>

const int SCREEN_WIDTH = 1024;
const int SCREEN_HEIGHT = 768;

bool init();
bool loadMedia();
void close();


SDL_Window * gWindow = NULL;
SDL_Surface * gScreenSurface = NULL;
SDL_Surface * gHelloWorld = NULL;

int main()
{
    if ( ! init() )
        return -1;
    if (! loadMedia() )
    {
        //
    } else {
        bool quit = false;
        SDL_Event e;
        while (! quit )
        {
            while ( SDL_PollEvent( &e ) != 0 )
            {
                if ( e.type == SDL_QUIT )
                {
                    quit = true;
                }
                else if ( e.type == SDL_KEYDOWN )
                {
                    switch ( e.key.keysym.sym ){
                        case SDLK_ESCAPE:
                            quit = true;
                            break;
                    };
                }
            };
            SDL_BlitSurface( gHelloWorld, NULL, gScreenSurface, NULL );
            SDL_UpdateWindowSurface( gWindow );
        };
        //SDL_Delay( 3000 );
    }

    close();
    return 0;
};


bool init()
{
    bool success = true;

    if ( SDL_Init( SDL_INIT_VIDEO ) < 0 )
    {
        fprintf( stderr, "SDL failed to initialize. SDL_ERROR: %s\n", SDL_GetError() );
        success = false;
    }
    else
    {
        gWindow = SDL_CreateWindow( "Callums World", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN );
        if ( gWindow == NULL )
        {
            fprintf( stderr, "Failed to Create Window. SDL_ERROR: %s\n", SDL_GetError() );
            success = false;
        }
        else 
        {
            gScreenSurface = SDL_GetWindowSurface( gWindow );
        };
    };
    return success;
};

SDL_Surface * loadSurface( std::string path )
{
    SDL_Surface * optomizedSurface = NULL;
    SDL_Surface * loadedSurface = SDL_LoadBMP( path.c_str() );
    if ( loadedSurface == NULL )
    {
        fprintf( stderr, "Failed load (%s) BMP. SDL_ERROR: %s\n", path.c_str(), SDL_GetError() );
        return NULL;
    }
    optomizedSurface = SDL_ConvertSurface( loadedSurface, gScreenSurface->format, (unsigned int) NULL );
    if ( optomizedSurface == NULL )
    {
        fprintf( stderr, "Failed to optomize (%s) BMP. SDL_ERROR: %s\n", path.c_str(), SDL_GetError() );
    };
    SDL_FreeSurface( loadedSurface );

    return optomizedSurface;
};

bool loadMedia()
{
    bool success = true;
    
    gHelloWorld = SDL_LoadBMP( "/home/callum/Downloads/ogl-OpenGL-tutorial_0015_33/misc05_picking/screenshot.bmp" );
    if ( gHelloWorld == NULL )
    {
        fprintf( stderr, "Failed to Load image. SDL_ERROR: %s\n", SDL_GetError() );
        success = false;
    };

    return success;
};

void close()
{
    SDL_FreeSurface( gHelloWorld );
    gHelloWorld = NULL;
    SDL_DestroyWindow( gWindow );
    gWindow = NULL;
    SDL_Quit();
};
