
import quake
from .entitymanager import EntityManager

#--------------------------------------------------------------------------------

entity_manager = None

def entity_spawn(self):
    global entity_manager

    if self.classname == 'worldspawn':
        entity_manager = EntityManager()

    entity_manager.spawn(self)
    return True

def entity_touch(self, other):
    entity_manager.touch(self, other)
    return True

def entity_think(self):
    entity_manager.think(self)
    return True

def entity_blocked(self, other):
    entity_manager.blocked(self, other)
    return True

def start_frame():
    return True

def player_pre_think(self):
    return True

def player_post_think(self):
    return True

def client_kill(self):
    return True

def client_connect(self):
    print('client_connect')
    return True

def put_client_in_server(self):
    global entity_manager
    self.classname = 'player'
    entity_manager.spawn(self)
    print('Changed one more time')
    return True

def set_new_parms():
    return True

def set_change_parms(self):
    return True

#--------------------------------------------------------------------------------

callbacks = {
    'entityspawn':              entity_spawn,
    'entitytouch':              entity_touch,
    'entitythink':              entity_think,
    'entityblocked':            entity_blocked,
    'startframe':               start_frame,
    'playerprethink':           player_pre_think,
    'playerpostthink':          player_post_think,
    'clientkill':               client_kill,
    'clientconnect':            client_connect,
    'putclientinserver':        put_client_in_server,
    'setnewparms':              set_new_parms,
    'setchangeparms':           set_change_parms,
}
