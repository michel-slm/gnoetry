# This is -*- Python -*-

class PoemListener:

    def __init__(self, p=None):
        self.__poem = None
        self.__changed_id = 0
        self.__changed_flag_id = 0
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

        if self.__changed_flag_id:
            self.__poem.disconnect(self.__changed_flag_id)
            self.__changed_flag_id = 0

        def changed_cb(x, y):
            y.poem_changed(x)

        def changed_flag_cb(x, i, y):
            y.poem_changed_flag(x, i)
            
        if p:
            self.__changed_id = p.connect("changed",
                                          changed_cb,
                                          self)
            self.__changed_flag_id = p.connect("changed_flag",
                                               changed_flag_cb,
                                               self)
        self.__poem = p
        if not self.__block:
            self.poem_changed(self.__poem)


    def poem_changed(self, p):
        pass


    def poem_changed_flag(self, p, i):
        pass
        
