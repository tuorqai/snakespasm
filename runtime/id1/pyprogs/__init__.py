
# This is a dummy 'pyprogs' package that does nothing.

from quake import *

def entity_spawn(self):
    pass

def after_entity_spawn(self):
    pass

def entity_touch(self, other):
    pass

def after_entity_touch(self, other):
    pass

def entity_think(self):
    pass

def after_entity_think(self):
    pass

def entity_blocked(self, other):
    pass

def after_entity_blocked(self, other):
    pass

def start_frame():
    pass

def after_start_frame():
    pass

def player_pre_think(self):
    pass

def after_player_pre_think(self):
    pass

def player_post_think(self):
    pass

def after_player_post_think(self):
    pass

def client_kill(self):
    pass

def after_client_kill(self):
    pass

def client_connect(self):
    pass

def after_client_connect(self):
    pass

def put_client_in_server(self):
    pass

def after_put_client_in_server(self):
    pass

def set_new_parms():
    pass

def after_set_new_parms():
    pass

def set_change_parms(self):
    pass

def after_set_change_parms(self):
    pass

callbacks = {
    'entityspawn':              entity_spawn,
    'afterentityspawn':         after_entity_spawn,
    'entitytouch':              entity_touch,
    'afterentitytouch':         after_entity_touch,
    'entitythink':              entity_think,
    'afterentitythink':         after_entity_think,
    'entityblocked':            entity_blocked,
    'afterentityblocked':       after_entity_blocked,
    'startframe':               start_frame,
    'afterstartframe':          after_start_frame,
    'playerprethink':           player_pre_think,
    'afterplayerprethink':      after_player_pre_think,
    'playerpostthink':          player_post_think,
    'afterplayerpostthink':     after_player_post_think,
    'clientkill':               client_kill,
    'afterclientkill':          after_client_kill,
    'clientconnect':            client_connect,
    'afterclientconnect':       after_client_connect,
    'putclientinserver':        put_client_in_server,
    'afterputclientinserver':   after_put_client_in_server,
    'setnewparms':              set_new_parms,
    'aftersetnewparms':         after_set_new_parms,
    'setchangeparms':           set_change_parms,
    'aftersetchangeparms':      after_set_change_parms,
}
