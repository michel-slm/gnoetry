
import gobject

class TileModel(gobject.GObject):

    def __init__(self, line_count):
        gobject.GObject.__init__(self)
        assert line_count > 0
        self.__line_count = line_count
        self.__lines = [None] * line_count
        for i in range(line_count):
            self.__lines[i] = []
        self.__selection = {}

    def get_line_count(self):
        return self.__line_count

    def get_line_length(self, line_num):
        if not (0 <= line_num < self.__line_count):
            raise ValueError, "invalid line %d" % line_num
        return len(self.__lines[line_num])

    def get_tile(self, line_num, col_num):
        if not (0 <= line_num < self.__line_count):
            raise ValueError, "invalid line %d" % line_num
        if not (0 <= col_num < len(self.__lines[line_num])):
            raise ValueError, "invalid column %d (line %d len=%d)" \
                  % (col_num, line_num, len(self.__lines[line_num]))
        return self.__lines[line_num][col_num]

    def set_tile(self, line_num, col_num, tile):
        if not (0 <= line_num < self.__line_count):
            raise ValueError, "invalid line %d" % line_num
        if not (0 <= col_num < len(self.__lines[line_num])):
            raise ValueError, "invalid column %d (line %d len=%d)" \
                  % (col_num, line_num, len(self.__lines[line_num]))
        self.__lines[line_num][col_num] = tile
        self.emit("changed_line", line_num)

    def insert_tile(self, line_num, col_num, tile):
        if not (0 <= line_num < self.__line_count):
            raise ValueError, "invalid line %d" % line_num
        if not (0 <= col_num <= len(self.__lines[line_num])):
            raise ValueError, "invalid column %d (line %d len=%d)" \
                  % (col_num, line_num, len(self.__lines[line_num]))
        self.__lines[line_num].insert(col_num, tile)
        self.emit("changed_line", line_num)

    def delete_tile(self, line_num, col_num):
        if not (0 <= line_num < self.__line_count):
            raise ValueError, "invalid line %d" % line_num
        if not (0 <= col_num < len(self.__lines[line_num])):
            raise ValueError, "invalid column %d (line %d len=%d)" \
                  % (col_num, line_num, len(self.__lines[line_num]))
        self.__lines[line_num].pop(col_num)
        self.emit("changed_line", line_num)

    def append_tile(self, line_num, tile):
        if not (0 <= line_num < self.__line_count):
            raise ValueError, "invalid line %d" % line_num
        self.__lines[line_num].append(tile)
        self.emit("changed_line", line_num)

    ###
    ### Manage tile selection
    ###

    def get_selection_flag(self, line_num, col_num):
        return self.__selection.get((line_num, col_num), 0)

    def set_selection_flag(self, line_num, col_num, flag=1):
        if flag:
            flag = 1
        else:
            flag = 0

        x = self.get_selection_flag(line_num, col_num)
        if x != flag:
            self.__selection[(line_num, col_num)] = flag
            self.emit("changed_selection",
                      line_num, col_num, flag)

    def set_selection_flag_on_line(self, line_num, flag):
        N = self.get_line_length(line_num)
        for i in range(N):
            self.set_selection_flag(line_num, i, flag)

    def clear_selection(self):
        if self.__selection:
            self.__selection = {}
            self.emit("changed_selection", -1, -1, 0)
            
        

gobject.type_register(TileModel)

gobject.signal_new("changed_line",
                   TileModel,
                   gobject.SIGNAL_RUN_LAST,
                   gobject.TYPE_NONE,
                   (gobject.TYPE_INT,)) # line number

gobject.signal_new("changed_selection",
                   TileModel,
                   gobject.SIGNAL_RUN_LAST,
                   gobject.TYPE_NONE,
                   (gobject.TYPE_INT, gobject.TYPE_INT,  # line num,  col num
                    gobject.TYPE_INT))                   # new value

    

##############################################################################

class TileModelListener:

    def __init__(self, model=None):
        self.__model = None
        self.__changed_line_id = None
        self.__changed_selection_id = None
        if model:
            self.connect_model(model)

    # FIXME: need to disconnect model on destruct

    def connect_model(self, model):
        if self.__model == model:
            return

        self.disconnect_model()

        if model is None:
            return

        self.__model = model

        def changed_line_handler(m, n, x):
            x.do_changed_line(n)

        def changed_selection_handler(m, i, j, f, x):
            x.do_changed_selection(i, j, f)

        self.__changed_line_id = model.connect("changed_line",
                                               changed_line_handler,
                                               self)
        self.__changed_selection_id = model.connect("changed_selection",
                                                    changed_selection_handler,
                                                    self)


    def disconnect_model(self):
        
        if self.__changed_line_id is not None:
            self.__model.disconnect(self.__changed_line_id)
            self.__changed_line_id = None

        if self.__changed_selection_id is not None:
            self.__model.disconnect(self.__changed_selection_id)
            self.__changed_selection_id = None
            
        self.__model = None

    def do_changed_line(self, line_num):
        pass

    def do_changed_selection(self, line_num, col_num, flag):
        pass
    
 
