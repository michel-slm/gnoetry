
import string

# All non-point intervals are treated as open intervals
def _overlap(a0, b0, a1, b1):
    if a0 == b0 and a1 == b1:
        return a0 == a1
    elif a0 == b0:
        return a1 < a0 < b1
    elif a1 == b1:
        return a0 < a1 < b0
    else:
        return max(a0, a1) < min(b0, b1)

class Overlay:

    def __init__(self, our_form):
        self.__form = our_form
        self.__items = []


    def __test(self, start_pos, end_pos):
        if not (start_pos <= end_pos \
                and 0 <= start_pos <= len(self.__form) \
                and 0 <= end_pos <= len(self.__form)):
            raise "invalid interval %d:%d" % (start_pos, end_pos)


    def get_all(self):
        return tuple(self.__items)


    def get_touching(self, start_pos, end_pos):
        self.__test(start_pos, end_pos)
        items = []
        for sp, ep, item in self.__items:
            if _overlap(sp, ep, start_pos, end_pos):
                items.append((sp, ep, item))
        return items


    def is_empty(self, start_pos, end_pos):
        self.__test(start_pos, end_pos)
        for sp, ep, items in self.__items:
            if _overlap(sp, ep, start_pos, end_pos):
                return 0
        return 1


    def attach(self, start_pos, end_pos, item=None):
        self.__test(start_pos, end_pos)
        if item is None:
            item = "%d:%d" % (start_pos, end_pos)
        if not self.is_empty(start_pos, end_pos):
            raise "can't attach to populated region %d:%d" % (start_pos,
                                                              end_pos)
        self.__items.append((start_pos, end_pos, item))

        def our_cmp(x, y):
            a0, b0 = x[:2]
            a1, b1 = y[:2]
            if a0 == a1 and b0 == b1:
                return 0
            elif b0 <= a1:
                return -1
            elif b1 <= a0:
                return +1
            assert 0
        self.__items.sort(our_cmp)


    def delete(self, start_pos, end_pos):
        self.__test(start_pos, end_pos)
        def is_disjoint(x):
            return not _overlap(x[0], x[1], start_pos, end_pos)
        self.__items = filter(is_disjoint, self.__items)


    def to_string(self):

        lines = []
        this_line = []
        prev_over = None
        
        for i in range(len(self.__form)):
            syl = self.__form[i]

            over = self.get_touching(i, i+1)
            if over:
                over = over[0]
            else:
                over = None

            if over:
                if prev_over != over:
                    this_line.append(over[2])
                    prev_over = over
            else:
                this_line.append(syl.to_string())
                prev_over = None

            if over is None or over[1] == i+1:
                zero = self.get_touching(i+1, i+1)
                if zero:
                    this_line.append(zero[0][2])

            if syl.is_line_terminal():
                lines.append(string.join(this_line, " "))
                this_line = []
            if syl.is_stanza_terminal():
                lines.append("")

        return string.join(lines, "\n")
    

    def spew(self):
        for sp, ep, item in self.__items:
            print "%3d %3d %s" % (sp, ep, item)
        

            
