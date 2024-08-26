
class Entity:
    def __init__(self, edict):
        self.edict = edict
        print(f'{self.edict.classname} spawned: {self.edict}')

    def touch(self, other):
        pass

    def think(self):
        pass

    def blocked(self, other):
        pass
