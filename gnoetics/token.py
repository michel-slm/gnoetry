import string

def token_list_to_str(L):

    str_list = []

    for i in range(len(L)):
        token = L[i]
        prev_token = i > 0 and L[i-1]

        if prev_token:
            if not (prev_token.has_right_glue() or token.has_left_glue()):
                str_list.append(" ")

        str_list.append(token.get_word())
            
    return string.join(str_list, "")
