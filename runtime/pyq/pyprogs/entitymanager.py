
from .entity import Entity
from .player import Player
from .world import World

class EntityManager:
    typedict = {
        'path_corner': Entity,
        'test_teleport': Entity,
        'test_fodder': Entity,
        'monster_boss': Entity,
        'event_lightning': Entity,
        'func_button': Entity,
        'info_intermission': Entity,
        'trigger_changelevel': Entity,
        'info_player_start': Entity,
        'info_player_start2': Entity,
        'info_player_deathmatch': Entity,
        'info_player_coop': Entity,
        'monster_demon1': Entity,
        'monster_dog': Entity,
        'func_door': Entity,
        'func_door_secret': Entity,
        'monster_enforcer': Entity,
        'monster_fish': Entity,
        'monster_hell_knight': Entity,
        'noclass': Entity,
        'item_health': Entity,
        'item_armor1': Entity,
        'item_armor2': Entity,
        'item_armorInv': Entity,
        'weapon_supershotgun': Entity,
        'weapon_nailgun': Entity,
        'weapon_supernailgun': Entity,
        'weapon_grenadelauncher': Entity,
        'weapon_rocketlauncher': Entity,
        'weapon_lightning': Entity,
        'item_shells': Entity,
        'item_spikes': Entity,
        'item_rockets': Entity,
        'item_cells': Entity,
        'item_weapon': Entity,
        'item_key1': Entity,
        'item_key2': Entity,
        'item_sigil': Entity,
        'item_artifact_invulnerability': Entity,
        'item_artifact_envirosuit': Entity,
        'item_artifact_invisibility': Entity,
        'item_artifact_super_damage': Entity,
        'trigger_jctest': Entity,
        'monster_knight': Entity,
        'info_null': Entity,
        'info_notnull': Entity,
        'light': Entity,
        'light_fluoro': Entity,
        'light_fluorospark': Entity,
        'light_globe': Entity,
        'light_torch_small_walltorch': Entity,
        'light_flame_large_yellow': Entity,
        'light_flame_small_yellow': Entity,
        'light_flame_small_white': Entity,
        'misc_fireball': Entity,
        'misc_explobox': Entity,
        'misc_explobox2': Entity,
        'trap_spikeshooter': Entity,
        'trap_shooter': Entity,
        'air_bubbles': Entity,
        'viewthing': Entity,
        'func_wall': Entity,
        'func_illusionary': Entity,
        'func_episodegate': Entity,
        'func_bossgate': Entity,
        'ambient_suck_wind': Entity,
        'ambient_drone': Entity,
        'ambient_flouro_buzz': Entity,
        'ambient_drip': Entity,
        'ambient_comp_hum': Entity,
        'ambient_thunder': Entity,
        'ambient_light_buzz': Entity,
        'ambient_swamp1': Entity,
        'ambient_swamp2': Entity,
        'misc_noisemaker': Entity,
        'monster_ogre': Entity,
        'monster_oldone': Entity,
        'func_plat': Entity,
        'func_train': Entity,
        'misc_teleporttrain': Entity,
        'monster_shalrath': Entity,
        'monster_shambler': Entity,
        'monster_army': Entity,
        'monster_tarbaby': Entity,
        'trigger_multiple': Entity,
        'trigger_once': Entity,
        'trigger_relay': Entity,
        'trigger_secret': Entity,
        'trigger_counter': Entity,
        'info_teleport_destination': Entity,
        'trigger_teleport': Entity,
        'trigger_setskill': Entity,
        'trigger_onlyregistered': Entity,
        'trigger_hurt': Entity,
        'trigger_push': Entity,
        'trigger_monsterjump': Entity,
        'monster_wizard': Entity,
        'worldspawn': World,
        'monster_zombie': Entity,
        'player': Player,
    }

    def __init__(self):
        self.instances = {}

    def spawn(self, edict):
        if not edict.classname in EntityManager.typedict:
            raise ValueError('Unknown classname')
        self.instances[edict] = EntityManager.typedict[edict.classname](edict)

    def touch(self, e1, e2):
        self.instances[e1].touch(self.instances[e2])

    def think(self, e):
        self.instances[e].think()

    def blocked(self, e1, e2):
        self.instances[e1].blocked(self.instances[e2])
