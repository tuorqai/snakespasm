
import quake as qu
from .entity import Entity

def select_spawn_point():
    for e in qu.entities():
        if e.classname == 'testplayerspawn':
            return e

    for e in qu.entities():
        if e.classname == 'info_player_start':
            return e

    return qu.entities()[0]

class Player(Entity):
    def __init__(self, edict):
        super().__init__(edict)

        self.edict.health = 160
        self.edict.solid = qu.SOLID_SLIDEBOX
        self.edict.movetype = qu.MOVETYPE_WALK
        self.edict.max_health = 100
        self.edict.flags = qu.FL_CLIENT
        self.edict.deadflag = qu.DEAD_NO

        spot = select_spawn_point()

        self.edict.setorigin(spot.origin + qu.Vector(0, 0, 1))
        self.edict.angles = spot.angles
        self.edict.fixangle = 1
        self.edict.setsize(qu.Vector(-16, -16, -24), qu.Vector(16, 16, 32))
        self.edict.setmodel('progs/player.mdl')

        self.edict.view_ofs = qu.Vector(0, 0, 22)

        self.edict.nextthink = qu.time() + 3.0

    def think(self):
        pass
        #print(f'{self.edict} think')
        #self.edict.nextthink = qu.time() + 0.5
        #self.edict.velocity.z += 270
