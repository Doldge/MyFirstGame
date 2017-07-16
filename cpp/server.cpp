#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include "serializer.h"
#include "world.hpp"
#include "player.hpp"

// various function declerations
void * connection_handler(void *);

World * new_world()
{
    /* This doesn't work. Not sure why. As best I can tell the room building
     * code doesn't do what I expect it to. It seems to replace abitrary blocks, but not
     * the entire room. Not sure if I'm screwing up my object references or if it's an issue
     * with the carving code.
     * Something does appear to replacing *some* of the blocks though, just not a rooms worth.
     */
    World * world = new World(80, 30);
    bool worlds_built = buildRooms( world, 17, 11, 3, 6, 5);
    if ( ! worlds_built )
    {
        fprintf( stderr, "Failed to build Rooms.\n" );
    };
    return world;
    CorridorBuilder * cb = new CorridorBuilder( world );
    if ( ! (cb->generate()) )
    {
        fprintf( stderr, "failed to generate corridors.\n" );
    };
    return world;
};

// globals / constants
const int max_clients = 30;
int client_socket[max_clients];

struct thread_input {
    int socket;
    int position;
};

World * myWorld = new_world();

//int main(int argc, char * argv[] )
int main()
{
    int sock_fd, new_sock_fd, port_no, cli_len;
    struct sockaddr_in serv_addr, cli_addr;
    int max_fd, opts = 1;

    fd_set read_fds;

    for (int i = 0; i< max_clients; i++)
    {
        client_socket[i] = 0;
    }
     
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    if ( sock_fd < 0 )
    {
        fprintf( stderr, "Error Opening Socket.\n" );
        return -1;
    }
    
    if ( setsockopt(sock_fd, SOL_SOCKET, SO_REUSEADDR, (char *) &opts, sizeof(opts)) < 0 )
    {
        fprintf( stderr, "Error setting socket options.\n");
    };

    bzero( (char *) &serv_addr, sizeof(serv_addr) );
    port_no = 6699;

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(port_no);

    if ( bind(sock_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0 )
    {
        //bind failed.
        fprintf( stderr, "Error binding to address/port.\n" );
        return -2;
    }
    
    listen( sock_fd, 10 ); // socket, backlog
   
    cli_len = sizeof(cli_addr); 

	fprintf( stdout, "The Server is now running.\n");
	fprintf( stdout, "Address: %X\n", serv_addr.sin_addr.s_addr);
	fprintf( stdout, "Port: %i\n", port_no);
    while ( true )
    {
        FD_ZERO(&read_fds); 
        FD_SET(sock_fd, &read_fds);
        max_fd = sock_fd;
        
        for ( int i=0; i < max_clients; i++ )
        {
            int sd = client_socket[i];
            if ( sd > 0 )
            {
                FD_SET(sd, &read_fds);
            };
            if ( sd > max_fd )
            {
                max_fd = sd;
            }
        }

        select( max_fd+1, &read_fds, NULL, NULL, NULL );

        if ( FD_ISSET(sock_fd, &read_fds) )
        {
            new_sock_fd = accept( sock_fd, (struct sockaddr *) &cli_addr, (socklen_t *) &cli_len );
            
            if ( new_sock_fd  < 0 )
            {
                fprintf( stderr, "client connection failed.\n" );
                continue; 
            }
            if ( setsockopt(new_sock_fd, IPPROTO_TCP, TCP_NODELAY, (char *) &opts, sizeof(opts)) < 0 )
            {
                fprintf( stderr, "Error setting socket options.\n");
            };
            fprintf( stdout, "Client Connected!\n" );
            for (int i=0; i < max_clients; i++)
            {
                if ( client_socket[i] == 0 )
                {
                    client_socket[i] = new_sock_fd;
                    fprintf( stdout, "Client is %d\n", i );
                    break;
                };
            }
        }
        for (int i =0; i < max_clients; i++)
        {
            int sock = client_socket[i];
            if ( ! FD_ISSET(sock, &read_fds) )
            {
                continue;
            }
            fprintf( stdout, "Activity on %d\n", i );
            pthread_t con_thread;
            struct thread_input args; 
            args.socket = sock;
            args.position = i;

            if ( pthread_create(&con_thread, NULL, connection_handler, (void *) &args) < 0 )
            {
                fprintf( stderr, "failed to create thread.\n" );
            };
        }
    };

    return 0;
};


void * connection_handler(void * args)
{
    struct thread_input * input = (struct thread_input *) args;
    int read_size, address_length;
    char buffer[256];
    struct sockaddr_in address;
    unsigned long length = 0, version = 0, command = 0;

    while ( (read_size = recv(input->socket, buffer, 256, MSG_DONTWAIT)) > 0 )
    {
        fprintf( stdout, "Input length: %d\n Contents:", read_size );
        for (int i = 0; i < read_size; i++)
        {
            fprintf( stdout, "%02X", buffer[i]);
        }
        fprintf( stdout, "\n" );
        if ( ! length && ! version && ! command )
        {
            memcpy( &length, &buffer[0], sizeof(long));
            memcpy( &version, &buffer[4], sizeof(long));
            memcpy( &command, &buffer[8], sizeof(long));
            length = ntohl(length);
            version = ntohl(version);
            command = ntohl(command);
        };
        fprintf( stdout, "length - version - command\t%ld - %ld - %ld\n", length, version, command );
        fprintf( stdout, "%ld Remaining\n", (length-read_size) );

    };
    if ( version == 0x100 )
    {
        switch ( command )
        {
            case 0x1:  // MOVE
                break;
            case 0x2:  // PLAYER CONNECT
                //FIXME
                // we're supposed to return the world first, and then the player connection OK response.
                int res;
                unsigned char message[256], * ptr; 
                ptr = serialize_int( message, sizeof(int) * 4 );
                fprintf( stdout, "Message is: %s\tlen:%ld\tfull-length:%ld\n", message, ptr - message, strlen((char *)message) );
                fprintf( stdout, "m:%c %c %c %c\n", message[0], message[1], message[2], message[3] );
                ptr = serialize_int( ptr, version );
                ptr = serialize_int( ptr, command );
                ptr = serialize_int( ptr, input->position );
                ptr[1] = '\0';
                ptr++;
                fprintf( stdout, "Message is: %s\tlen:%ld\n", message, ptr-message );
                res = write( input->socket, &message, ptr - message );
                fprintf( stdout, "Sent %d bytes\n", res );
                for (int i = 0; i < 12; i++)
                {
                    fprintf( stdout, "%02X", message[i]);
                }
                fprintf( stdout, "\n");
                for (int i = 0; i < 12; i++)
                {
                    fprintf( stdout, "%02X", buffer[i]);
                }
                fprintf( stdout, "\n");
                TilePtr ** matrix = myWorld->getMatrix(); 
                for (int h=0; h < myWorld->getHeight(); h++)
                {
                    for (int w=0; w < myWorld->getWidth(); w++)
                    {
                        fprintf( stdout, "%c", matrix[h][w]->represent() );
                    };
                    fprintf( stdout, "\n" );
                };
                
        };
    };

    if ( read_size == 0 )
    { //disconnect
        getpeername( input->socket, (struct sockaddr *) &address, (socklen_t *) &address_length );
        fprintf( stdout, "Host Disconnected\tIP: %s\nPort %d\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port) );
        close( input->socket );
        client_socket[input->position] = 0;
    }

    //https://developers.google.com/protocol-buffers/
    //http://www.binarytides.com/server-client-example-c-sockets-linux/
    //http://stackoverflow.com/questions/1577161/passing-a-structure-through-sockets-in-c
    //http://www.binarytides.com/multiple-socket-connections-fdset-select-linux/
    return 0;
}
