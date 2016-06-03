#! /usr/bin/python

import struct
import socket
import os
import logging
import select
import threading
import Queue
import sys

# Custom Imports
import world
from commands import *
from shared_functions import *

logging.basicConfig(
    level=logging.DEBUG,
    filename='/tmp/g_client.log'
)
logger = logging.getLogger()
logger.setLevel(logging.DEBUG)



#Socket Info
HOST='127.0.0.1'
PORT=6699
WALL = '|'
ROOF = '-'

myworld = world.World()
CURRENT_PLAYER = None

def move_wrapper(direction, player):
    global error
    try:
        move_player(direction,player)
    except world.MoveException as e:
        error = e.message
        return False
    return True


def draw():
    global is_paused
    os.system('clear')
    print(
        "Connected To {} on Port {}".format(HOST,PORT)
    )
    if CURRENT_PLAYER:
        print(
            "You Are: {}\n\n".format(CURRENT_PLAYER)
        )
    print("-"*(myworld.width+2))
    for line in myworld.getMatrix():
        print('|'+''.join([str(x) for x in line])+'|')
    print("-"*(myworld.width+2))
    info()
    if is_paused:
        print('''
        Something went wrong and the game has been paused.
        Press any key to continue.''')
        raw_input('')
    if waiting:
        print('''
        Waiting for more players to connect.''')


def pause():
    raw_input('')

def lpad(s,l=32):
    if len(s) < l:
        return ' '*(l-len(s))+s
    return s


def rpad(s,l=32):
    if len(s) < l:
        return s+' '*(l-len(s))
    return s

def info():
    global error
    tab = ' '*12
    endtab = '|\n'
    print(
        tab + "-" * 32 + '\n' +
        tab + rpad('| Controls:') + endtab +
        tab + rpad('| w:  Move Up') + endtab +
        tab + rpad('| a:  Move Left') + endtab +
        tab + rpad('| s:  Move Down') + endtab +
        tab + rpad('| d:  Move Right') + endtab +
        tab + rpad('|') + endtab +
        tab + rpad('| quit:  Exits the game') + endtab +
        tab + "-" * 32 + '\n'
    )

    if error:
        print(
            "\n" +
            tab +'ERROR: '+error
            +"\n"
        )
    error = None


def setPlayer(player):
    global CURRENT_PLAYER
    if not CURRENT_PLAYER:
        CURRENT_PLAYER = myworld.players[player]




def close():
    global con
    if con is not None:
        try:
            con.shutdown(socket.SHUT_RDWR)
        except:
            pass
        con.close()


def process_response(message=[]):
    global myworld,waiting, redraw
    if not message:
        return
    data = ''.join([ c.decode('hex') for c in message ])
    length, version, command = None, None, None
    try:
        length = int(''.join(message[:4]),base=16)
        version = int(''.join(message[4:8]),base=16)
        command = int(''.join(message[8:12]),base=16)
    except:
        logger.error('could not retrieve version and command')
        return
    s = server[version].get(command)
    if command == MOVE:
        logger.debug(message)
        m = struct.unpack(s(len(message[16:])),data)
        m = cleanup(m)
        move_wrapper(m[4],myworld.players[m[3]])
    elif command == PLAYER_CONNECT:
        # FIXME FIXME FIXME
        m = struct.unpack(s, data)
        m = cleanup(m)
        setPlayer(m[3])
    elif command == PLAYER_DISCONNECT:
        m = struct.unpack(s, data)
        logger.debug( m )
        myworld.removePlayer(m[3])
    elif command == PLAYER_WAIT:
        #There's no data, we just need to wait
        redraw = True
        waiting = True
    elif command == WORLD:
        m = struct.unpack(s(len(message[12:])), data)
        #logger.debug(m)
        myworld = myworld.createFromXML(m[3])
        if len(myworld.players) >= 2:
            waiting = False
        redraw = True


def send(message):
    totalsent = 0
    while ( totalsent < len(message) ):
        sent = 0
        try:
            sent = con.send(message[totalsent:])
        except Exception as e:
            error = str(e)
            is_paused = True
            connect()
            draw()
            break;
        totalsent = totalsent + sent
    return totalsent


def recv():
    data = ''.join([ '00'.decode('hex') ]*4)
    length = None
    try:
        length = con.recv(4)
        length = ''.join([x.encode('hex') for x in length])
        length = int(length,base=16)-4
    except:
        pass
    try:
        while length:
            page = con.recv(length)
            data = data + page if data else page
            length -= len(page)
        return [ c.encode('hex') for c in data ]
    except:
        con.settimeout(None)
        return None


def connect():
    global con, PORT, HOST
    close()
    con = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    while PORT > 6690:
        try:
            con.connect( (HOST,PORT) )
            con.setblocking(0)
            return con
        except socket.error as e:
            if (e.errno == 98 or e.errno == 111):
                PORT -= 1
            else:
                raise


def networking():
    global con, network_ops, redraw
    try:
        connect()
    except socket.error as e:
        logger.exception('Connection Failed')
    except Exception as e:
        logger.exception('shitty programming')
    try:
        while not is_done:
            if not network_ops.empty():
                cmd = network_ops.get()
                send(cmd)
                network_ops.task_done()
            if select.select([con], [], [], 1) == ([], [], []):
                pass
            else:
                a = recv()
                if a:
                    process_response(a)
                    redraw = True
        con.close()
    except Exception as e:
        logger.exception('more bad programming')


def input():
    try:
        global redraw, is_done
        inp = None
        handled = []
        while not is_done:
            i,o,e = select.select([sys.stdin],[],[])
            for f in i:
                if f == sys.stdin:
                    inp = sys.stdin.readline()
            if inp:
                inp = inp.strip()
                logger.debug('INPUT:{}'.format(inp))
            if inp == 'quit':
                is_done = True
            elif inp:
                success = move_wrapper(inp,CURRENT_PLAYER)
                message = struct.pack(
                    client[CURRENT_VERSION][MOVE](len(inp)),
                    12+len(inp),
                    CURRENT_VERSION, MOVE, inp
                )
                if success:
                    network_ops.put(message)
                redraw = True
                inp = None
    except:
        logger.exception('something went wrong')


def __main__():
    global is_done, error, is_paused, redraw
    m = None
    #Set which player we are.
    m = struct.pack(
        client[CURRENT_VERSION][PLAYER_CONNECT], 12, CURRENT_VERSION,
        PLAYER_CONNECT
    )
    network_ops.put(m)
    while not is_done:
        if redraw:
            draw()
            redraw = False


if __name__ == '__main__':
    waiting = False
    redraw = False
    is_done = False
    error = None
    is_paused = False
    con = None
    network_ops = Queue.Queue()
    ns = threading.Thread( target=networking )
    ns.daemon = True
    ns.start()
    inp = threading.Thread( target=input )
    inp.daemon = True
    inp.start()
    __main__();
