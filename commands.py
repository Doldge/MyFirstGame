#! /usr/bin/python

# Contains Network Protocol Info.
#
#

CURRENT_VERSION = 0x100

# # # # # # # # #
#TODO: make the below an ENUM, as you can't be moving AND disconnecting.
MOVE = 0x1
PLAYER_CONNECT = 0x2
PING = 0x3
PLAYER_WAIT = 0x4
WORLD = 0x5
PLAYER_DISCONNECT = 0x6
client = {
    0x100: {  # Messages
        # M-Nmber   M-Decode
        MOVE: '!III{0}s'.format,  # this is a move command
        PLAYER_CONNECT: '!III',  # client is requesting which player it is.
        PING: '!III',  # FIXME either remove this, or add a ping mech in the client
        WORLD: '!III',
    }
}

server = {
    0x100: {
        MOVE: '!IIII{0}s'.format,  # this a move command.
        PLAYER_CONNECT: '!IIII',  # tell the client which player it is.
        PING: '!III',
        PLAYER_WAIT: '!III',
        WORLD: '!III{0}s'.format,  # this returns the xml world.
        PLAYER_DISCONNECT: '!IIII'
    }
}

