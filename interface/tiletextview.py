
import string
import pango, gtk
import gnoetics, tilemodel

class TileTextView(gtk.TextView,
                   tilemodel.TileModelListener):

    def __init__(self, model):
        gtk.TextView.__init__(self)
        tilemodel.TileModelListener.__init__(self, model)

        self.set_editable(0)
        self.set_cursor_visible(0)

        self.__model = model
        self.__buffer = gtk.TextBuffer()
        self.__tiles_to_buffer()
        self.set_buffer(self.__buffer)


    def __tiles_to_buffer(self):
        lines = []
        for i in range(self.__model.get_line_count()):
            line_str = ""
            prev_right_glue = 0
            for j in range(self.__model.get_line_length(i)):
                t = self.__model.get_tile(i, j)
                txt = "???"
                has_left_glue = 0
                if type(t) == gnoetics.Token:
                    txt = t.get_word()
                    has_left_glue = t.has_left_glue()
                    prev_right_glue = t.has_right_glue()
                elif t == "stanza break":
                    txt = ""
                    prev_right_glue = 0
                else:
                    txt = "__"
                    prev_right_glue = 0

                if prev_right_glue or has_left_glue:
                    line_str += txt
                else:
                    line_str += " " + txt

            lines.append(" " + line_str + " ")
        self.__buffer.set_text(string.join(lines, "\n"))


    def do_changed_line(self, line_num):
        self.__tiles_to_buffer()
