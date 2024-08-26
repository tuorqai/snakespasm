
import quake
from .entity import Entity

class World(Entity):
    def __init__(self, edict):
        super().__init__(edict)
        precache_all()

def precache_all():
    sounds = [
        "demon/dland2.wav",
        "items/armor1.wav",
        "items/damage3.wav",
        "items/itembk2.wav",
        "misc/h2ohit1.wav",
        "misc/outwater.wav",
        "misc/power.wav",
        "misc/r_tele1.wav",
        "misc/r_tele2.wav",
        "misc/r_tele3.wav",
        "misc/r_tele4.wav",
        "misc/r_tele5.wav",
        "misc/talk.wav",
        "misc/water1.wav",
        "misc/water2.wav",
        "player/axhit1.wav",
        "player/axhit2.wav",
        "player/death1.wav",
        "player/death2.wav",
        "player/death3.wav",
        "player/death4.wav",
        "player/death5.wav",
        "player/drown1.wav",
        "player/drown2.wav",
        "player/gasp1.wav",
        "player/gasp2.wav",
        "player/gib.wav",
        "player/h2odeath.wav",
        "player/h2ojump.wav",
        "player/inh2o.wav",
        "player/inlava.wav",
        "player/land.wav",
        "player/land2.wav",
        "player/lburn1.wav",
        "player/lburn2.wav",
        "player/pain1.wav",
        "player/pain2.wav",
        "player/pain3.wav",
        "player/pain4.wav",
        "player/pain5.wav",
        "player/pain6.wav",
        "player/plyrjmp8.wav",
        "player/slimbrn2.wav",
        "player/teledth1.wav",
        "player/tornoff2.wav",
        "player/udeath.wav",
        "weapons/ax1.wav",
        "weapons/bounce.wav",
        "weapons/grenade.wav",
        "weapons/guncock.wav",
        "weapons/lhit.wav",
        "weapons/lock4.wav",
        "weapons/lstart.wav",
        "weapons/pkup.wav",
        "weapons/r_exp3.wav",
        "weapons/ric1.wav",
        "weapons/ric2.wav",
        "weapons/ric3.wav",
        "weapons/rocket1i.wav",
        "weapons/sgun1.wav",
        "weapons/shotgn2.wav",
        "weapons/spike2.wav",
        "weapons/tink1.wav"
    ]

    models = [
        "progs/player.mdl",
        "progs/eyes.mdl",
        "progs/h_player.mdl",
        "progs/gib1.mdl",
        "progs/gib2.mdl",
        "progs/gib3.mdl",
        "progs/s_bubble.spr",
        "progs/s_explod.spr",
        "progs/v_axe.mdl",
        "progs/v_shot.mdl",
        "progs/v_nail.mdl",
        "progs/v_rock.mdl",
        "progs/v_shot2.mdl",
        "progs/v_nail2.mdl",
        "progs/v_rock2.mdl",
        "progs/bolt.mdl",
        "progs/bolt2.mdl",
        "progs/bolt3.mdl",
        "progs/lavaball.mdl",
        "progs/missile.mdl",
        "progs/grenade.mdl",
        "progs/spike.mdl",
        "progs/s_spike.mdl",
        "progs/backpack.mdl",
        "progs/zom_gib.mdl",
        "progs/v_light.mdl"
    ]

    for s in sounds:
        quake.precache_sound(s)

    for m in models:
        quake.precache_model(m)
