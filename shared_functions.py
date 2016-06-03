#! /usr/bin/python

#
# Contains functions shared between the server & client.
#
NULL = '\x00'


def move_player(direction, player):
    y, x = player.getPosition()
    if direction == 'w':
        y = y - 1
    elif direction == 's':
        y = y + 1
    elif direction == 'a':
        x = x - 1
    elif direction == 'd':
        x = x + 1
    elif direction != 'quit':
        error = 'Not a valid command'
    #Raises a MoveException on error.
    player.move(y, x)
    return True


def cleanup(m):
    o = [i.strip(NULL) if isinstance(i,basestring) else i for i in m]
    if isinstance(m,tuple):
        return tuple(o)
    return o


def comparision_Generator(lmin, lmax):
    if not lmax:
        if isinstance(lmin, basestring):
            lmax = lmin
        elif isinstance(lmin, (int, float)):
            lmax = float('inf')
    return lambda x: x is not None and x >= lmin and x <= lmax or False

def equals_Generator(lmin):
    return comparision_Generator(lmin, lmin)

def less_than_Generator(lmax):
    return comparision_Generator(float('-inf'), lmax-1)

def less_than_or_equal_to_Generator(lmax):
    return comparision_Generator(float('-inf'), lmax)

def greater_than_Generator(lmin):
    return comparision_Generator(lmin + 1, float('inf'))

def greater_than_or_equal_to_Generator(lmin):
    return comparision_Generator(lmin, float('inf'))
