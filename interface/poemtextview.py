# This is -*- Python -*-

import string
import pango, gtk
import gnoetics


class PoemTextView(gtk.TextView,
                   gnoetics.PoemListener):

    def __init__(self, poem=None):
        gtk.TextView.__init__(self)
        gnoetics.PoemListener.__init__(self, poem)

        self.set_editable(0)
        self.set_cursor_visible(0)

        self.__buffer = gtk.TextBuffer()
        self.poem_changed(self.get_poem())
        self.set_buffer(self.__buffer)


    def __poem_to_nice_str(self, poem):
        lines = []
        current_line = ""

        last_was_break = False

        for x in poem.to_list_with_breaks():
            if x == "end of line":
                lines.append(current_line)
                current_line = ""
            elif x == "end of stanza":
                lines.append(current_line)
                lines.append("")
                current_line = ""
            elif type(x) == gnoetics.Token:
                if x.is_break():
                    last_was_break = True
                else:
                    word = x.get_word()

                    if last_was_break:
                        word = word[0].upper() + word[1:]

                    if word == "i":
                        word = "I"

                    # extra space at beginning of sentence
                    if current_line and last_was_break:
                        current_line += " "
                        
                    if current_line and not x.has_left_glue():
                        current_line += " "

                    current_line += word

                    if not x.is_punctuation():
                        last_was_break = False

            else: # is a unit
                if current_line:
                    current_line += " "
                current_line += "_" * x.get_syllables()

        if current_line:
            lines.append(current_line)

        return string.join(lines, "\n")


    def poem_changed(self, poem):
        txt = ""
        if poem:
            txt = self.__poem_to_nice_str(poem)
        self.__buffer.set_text(txt)
