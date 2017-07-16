#! /usr/bin/python

import socket
import SocketServer
import threading
import sys
import os
import logging
import struct
import time

# Custom Imports
import world
from commands import *
from shared_functions import *

HOST = ''
PORT = 6699
CURRENT_VERSION = 0x100
logging.basicConfig(
    level = logging.DEBUG
)
logger = logging.getLogger()


class ThreadedTCPRequestHandler(SocketServer.BaseRequestHandler):

    def setup(self):
        self.request.setblocking(0)

    def handle(self):
        self.server.clients.append(self.request)
        try:
            while True:
                data = ''
                try:
                    data = length = self.request.recv(4)
                except socket.error as e:
                    if e.errno == 11:
                        continue
                    raise
                length = ''.join([ c.encode('hex') for c in length ])
                # Deduct the 4 bytes we've just grabbed that contain the
                # length.
                length = int(length, base=16) - 4
                while length:
                    page = self.request.recv(length)
                    data = data + page if data else page
                    length -= len(page)
                logger.debug(data.encode('hex'))
                self.process_request(data)
        except Exception as e:
            logger.exception('Exception occurred in handle')
        finally:
            #Disconnected.
            self.server.disconnect(self.request)


    def process_request(self, message):
        if not message:
            return
        block = [ c.encode('hex') for c in message ]
        length, version, command = None, None, None
        try:
            length = int(''.join(block[:4]), base=16)
            version = int(''.join(block[4:8]),base=16)
            command = int(''.join(block[8:12]),base=16)
        except:
            #content doesn't match our requests. dump it.
            return
        logger.debug( (version, command) )
        decoder = client[version].get(command)
        if command == MOVE:
            # Player moved.
            m = struct.unpack(decoder(len(block[12:])),message)
            m = cleanup(m)
            try:
                move_player(
                    m[3], self.server.world.getPlayer(
                        self.server.clients.index(self.request)
                    )
                )
            except world.MoveException as m:
                logger.exception('invalid movement for player {}'.format(
                    self.server.world.getPlayer(
                        self.server.clients.index(self.request)
                    ).name
                ))
            m = struct.pack(
                server[version][MOVE](len(m[3])), 16+len(m[3]), version,
                MOVE, self.server.clients.index(self.request), m[3]
            )
            self.server.broadcast(self.request,m)
        elif command == PLAYER_CONNECT:
            # They want to know which player they are.
            w = self.server.world.getXMLasString()
            #print( w )
            m = struct.pack(server[version][WORLD](len(w)), 12+len(w), version,
            WORLD, w)
            #world.draw(self.server.world)
            self.sendall(self.request, m)
            m = None
            if len(self.server.clients) < 2:
                # Wait for more players.
                m = struct.pack(
                    server[version][PLAYER_WAIT], 12, version, PLAYER_WAIT
                )
                self.sendall(self.request, m)
            else:
                if not self.server.started:
                    # Start the game.
                    for sock in self.server.clients:
                        p = world.Player(self.server.clients.index(sock), '&',
                            name='Player {}'.format(self.server.clients.index(sock))
                        )
                        self.server.world.addPlayer(p)
                    self.server.started = True
                else:
                    p = world.Player(
                        self.server.clients.index(self.request), '&',
                        name='Player {}'.format(self.server.clients.index(self.request))
                    )
                    self.server.world.addPlayer(p)
                w = self.server.world.getXMLasString()
                m = struct.pack(server[version][WORLD](len(w)), 12+len(w), version,
                WORLD, w)
                self.server.broadcast(None, m)
                for sock in self.server.clients:
                    m = struct.pack(
                        server[version].get(command), 16, version, command,
                        self.server.clients.index(sock)
                    )
                    self.sendall(sock, m)
        else:
            logger.debug(block)

    def send(self, sock, data):
        logger.debug(data.encode('hex'))
        return sock.send(data)

    def sendall(self, sock, data):
        logger.debug(data.encode('hex'))
        return sock.sendall(data)


class GameServer(SocketServer.ThreadingTCPServer):
    def __init__(self, server_address, handler_class):
        #super(GameServer,self).__init__(server_address,handler_class)
        SocketServer.ThreadingTCPServer.__init__(self, server_address, handler_class)
        self.clients = []
        self.world = None
        self.newWorld()
        self.started = False

    def newWorld(self):
        self.world = world.World()
        world.buildRooms(self.world, 17, 11, 3)
        c = world.CorridorBuilder(self.world)
        c.generate()

    def startPing(self):
        self.pingThread = threading.Thread(target=self.pinger)
        self.pingThread.daemon = True
        self.pingThread.start()

    def broadcast(self, omitsock,message):
        for sock in self.clients:
            if sock != omitsock:
                try:
                    self.send(sock, message)
                except socket.error as e:
                    if e.errno == 9 or e.errno == 32:
                        try:
                            sock.shutdown()
                            sock.close()
                        except:
                            pass
                        finally:
                            self.disconnect(sock)
                    else:
                        raise

    def pinger(self):
        p = struct.pack(server[CURRENT_VERSION][PING], 12, CURRENT_VERSION, PING)
        while not finished:
            try:
                self.broadcast(None, p)
            except:
                logger.exception('in ping thread')
            time.sleep(3)

    def disconnect(self, sock):
        logger.debug('player disconnected')
        i = self.clients.index(sock)
        m = struct.pack(
            server[CURRENT_VERSION][PLAYER_DISCONNECT], 16,
            CURRENT_VERSION, PLAYER_DISCONNECT, i
        )
        self.broadcast(sock, m)
        p = self.world.players[i]
        y, x = p.getPosition()
        self.world.getMatrix()[y][x] = world.BaseTile()
        del(self.world.players[i])
        self.clients.remove(sock)
        #world.draw(self.world)

def __main__():
    primary_server = GameServer(
        (HOST, PORT), ThreadedTCPRequestHandler
    )
    primary_server.allow_reuse_address = True
    ip, port = primary_server.server_address
    logger.info('Serving on {}:{}  in PID: {}'.format(ip,port,os.getpid()))
    primary_server.serve_forever()


if __name__ == '__main__':
    finished = False
    try:
        while not finished:
            try:
                __main__()
            except socket.error as e:
                if e.errno == 98:
                    PORT -= 1
                else:
                    raise
    except KeyboardInterrupt as k:
        finished = True
        logger.info('caught ctrl + c')
        sys.exit()
