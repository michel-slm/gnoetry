import string
import form

CHUNK_FREE  = "free"   # ("free", i0, i1)
CHUNK_WORD  = "word"   # ("word", i0, i1, token)
CHUNK_ZERO  = "zero"   # ("zero", token)
CHUNK_BREAK = "break"  # ("break",)


class SyllableBasedPoem:

    def __init__(self, form):
        self.__form = form
        self.__chunks = [(CHUNK_BREAK,)]

        i0 = 0
        for i in range(len(form)):
            syl = form[i]
            if syl.is_word_terminal() \
               or syl.is_sentence_terminal() \
               or syl.is_line_terminal() \
               or syl.is_stanza_terminal():
                self.__chunks.append((CHUNK_FREE, i0, i))
                if syl.is_sentence_terminal():
                    self.__chunks.append((CHUNK_BREAK,))
                i0 = i+1

    def get_form(self):
        return self.__form

    def to_string(self):
        lines = []
        this_line = []

        for chunk in self.__chunks:

            eol = 0
            eostza = 0

            if chunk[0] == CHUNK_FREE:
                i0, i1 = chunk[1:]
                for syl in self.__form[i0:i1+1]:
                    this_line.append(syl.to_string())
                syl = self.__form[i1]
                eol = syl.is_line_terminal()
                eostza = syl.is_stanza_terminal()
            elif chunk[0] == CHUNK_WORD:
                i0, i1, token = chunk[1:]
                s = "%s(%d)(%s)" % (token.get_word(),
                                    token.get_syllables(),
                                    token.get_meter())
                this_line.append(s)
                syl = self.__form[i1]
                eol = syl.is_line_terminal()
                eostza = syl.is_stanza_terminal()
            elif chunk[0] == CHUNK_ZERO:
                token = chunk[-1]
                this_line.append(token.get_word())

            if eol:
                lines.append(string.join(this_line, " "))
                this_line = []
            if eostza:
                lines.append("")

        if this_line:
            lines.append(string.join(this_line, " "))

        return string.join(lines, "\n")

    def spew_chunks(self):
        for i in range(len(self.__chunks)):
            print "%2d" % i, self.__chunks[i]

    def __find_chunk_index(self, i):
        for j in range(len(self.__chunks)):
            chunk = self.__chunks[j]
            if chunk[0] == CHUNK_FREE or chunk[0] == CHUNK_WORD:
                i0, i1 = chunk[1:3]
                if i0 <= i and i <= i1:
                    return j
        return -1

    def _get_chunk(self, i):
        return self.__chunks[i]

    def _find_free_chunk(self):
        for j in range(len(self.__chunks)):
            chunk = self.__chunks[j]
            if chunk[0] == CHUNK_FREE:
                return j
        return -1

    def _study_free_chunk(self, j):
        free = self.__chunks[j]
        assert free[0] == CHUNK_FREE
        f0, f1 = free[1:]
        pred_tokens = []
        succ_tokens = []

        pred_brk = 0
        succ_brk = 0
        pred_zero = 0
        succ_zero = 0

        meter = []
        for i in range(f0, f1+1):
            meter.append( self.__form[i].get_meter())
        meter = string.join(meter, "")

        i = j-1
        while i >= 0:
            chunk = self.__chunks[i]
            if chunk[0] == CHUNK_FREE:
                break
            if chunk[0] == CHUNK_BREAK:
                pred_brk = 1
                break
            token = chunk[-1]
            pred_tokens.insert(0, token)
            pred_zero = (token.get_syllables() == 0)
            i -= 1

        i = j+1
        while i < len(self.__chunks):
            chunk = self.__chunks[i]
            if chunk[0] == CHUNK_FREE:
                break
            if chunk[0] == CHUNK_BREAK:
                succ_brk = 1
                break
            token = chunk[-1]
            if not succ_tokens:
                succ_zero = (token.get_syllables() == 0)
            succ_tokens.append(token)
            i += 1

        info = {
            "predecessors": pred_tokens,
            "successors":   succ_tokens,
            "pred_break":   pred_brk,
            "succ_break":   succ_brk,
            "pred_zero":    pred_zero,
            "succ_zero":    succ_zero,
            "syllables":    f1 - f0 + 1,
            "meter":        meter,
            }

        return info
        

    def bind(self, chunk_index, token=None, right_bind=0,
             insert_break=0):
        j = chunk_index
        assert 0 <= j < len(self.__chunks)

        chunk = self.__chunks[j]
        assert chunk[0] == CHUNK_FREE

        if token is None and insert_break:
            if right_bind:
                self.__chunks.insert(j+1, (CHUNK_BREAK,))
            else:
                self.__chunks.insert(j, (CHUNK_BREAK,))
            return
        assert not insert_break

        i0, i1 = chunk[1:]

        n = token.get_syllables()
        assert n <= i1-i0+1

        if right_bind:
            f0 = i0
            f1 = i1-n
            w0 = f1+1
            w1 = i1
        else:
            w0 = i0
            w1 = w0+n-1
            f0 = w1+1
            f1 = i1

        if n == 0:
            new_chunk = (CHUNK_ZERO, token)
        else:
            new_chunk = (CHUNK_WORD, w0, w1, token)

        if f0 <= f1:
            new_free = (CHUNK_FREE, f0, f1)
        else:
            new_free = None

        new_section = [new_chunk]
        if new_free:
            if right_bind:
                new_section.insert(0, new_free)
            else:
                new_section.append(new_free)

        self.__chunks = self.__chunks[:j] + new_section + self.__chunks[j+1:]



        

