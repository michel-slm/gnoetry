# This is -*- Python -*-

class PoemListener:

    def __init__(self, p=None):
        self.__poem = None
        self.__changed_id = 0
        self.__block = True
        self.set_poem(p)
        self.__block = False


    def get_poem(self):
        return self.__poem


    def set_poem(self, p):
        if p == self.__poem:
            return
        if self.__changed_id:
            self.__poem.disconnect(self.__changed_id)
            self.__changed_id = 0
        if p:
            self.__changed_id = p.connect("changed",
                                          lambda x, y: y.poem_changed(x),
                                          self)
        self.__poem = p
        if not self.__block:
            self.poem_changed(self.__poem)


    def poem_changed(self, p):
        pass
        
