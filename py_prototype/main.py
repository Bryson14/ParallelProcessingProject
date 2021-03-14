import sys
import pathlib as p
from typing import List
import numpy as np

USAGE = "USAGE:\n\tpython main.py badFileName [--r](replace) || [-s int](suggestions)"
dict_file = 'popular.txt'
dict_set = set()
replace = False
meta_chars = list(range(1, 32))
space_punctuation_numbers = list(range(32, 65)) + list(range(91, 97)) + list(range(123, 127))
special_char = meta_chars + space_punctuation_numbers


def levenshtein(source, target):
    """
    Vectorized version of min edit distance of strings
    https://en.wikibooks.org/wiki/Algorithm_Implementation/Strings/Levenshtein_distance#Python
    """
    if len(source) < len(target):
        return levenshtein(target, source)

    # So now we have len(source) >= len(target).
    if len(target) == 0:
        return len(source)

    # We call tuple() to force strings to be used as sequences
    # ('c', 'a', 't', 's') - numpy uses them as values by default.
    source = np.array(tuple(source))
    target = np.array(tuple(target))

    previous_row = np.arange(target.size + 1)
    for s in source:
        # Insertion (target grows longer than source):
        current_row = previous_row + 1

        # Substitution or matching:
        current_row[1:] = np.minimum(
            current_row[1:],
            np.add(previous_row[:-1], target != s))

        # Deletion (target grows shorter than source):
        current_row[1:] = np.minimum(
            current_row[1:],
            current_row[0:-1] + 1)

        previous_row = current_row

    return previous_row[-1]


# This would be a separate process
def min_edit_distance(word: str, idx: int, return_num_words: int) -> List[str]:
    if return_num_words == 1:
        best_score = 100
        best = ""
        for d_word in dict_set:
            score = levenshtein(word, d_word)
            if score < best_score:
                best = d_word
                best_score = score
        return [best]

    else:
        words = {}
        cutoff = 100
        for d_word in dict_set:
            score = levenshtein(word, d_word)
            if len(words) < return_num_words:
                words[d_word] = score
                if len(words) == return_num_words:
                    cutoff = max([words[w] for w in words])

            elif len(words) == return_num_words and score < cutoff:
                # replace
                for w in words:
                    if words[w] > score:
                        words.pop(w)
                        words[d_word] = score
                        break

        return [w for w in words]


# This would be a separate process
def check_word(word: str, idx: int, return_num_words: int) -> List[str]:
    if not word:
        return [""]
    if word.lower() in dict_set:
        return [word]
    else:
        return min_edit_distance(word, idx, return_num_words)


# master process
def main(args):
    if len(args) == 1:
        print(USAGE)
        exit(1)
    incorrect_file = args[1]
    if not p.Path.is_file(p.Path(incorrect_file)):
        print(USAGE)
        exit(1)

    if len(args) == 4 and args[2] == '-s':
        suggestions = int(args[3])
    else:
        suggestions = 1

    if len(args) == 3 and args[2] == '--r':
        global replace
        replace = True
    dict_f = open(dict_file, 'r')
    lines = dict_f.readlines()
    dict_f.close()
    for line in lines:
        dict_set.add(line.strip())

    f_name, f_type = incorrect_file.split('.')
    corrected_f = open(f"{f_name}(corrected).md", "w+")

    with open(incorrect_file, 'r') as f:
        i = 0
        buffer = ""
        while f.readable():
            char = f.read(1)
            if len(char) == 0:  # EOL
                break
            if len(char) > 0 and ord(char) in special_char:  # new line or space
                r_words = check_word(buffer, i, suggestions)  # new process
                if r_words[0] == buffer:
                    corrected_f.write(buffer + char)  # no problem
                elif replace:
                    corrected_f.write(r_words[0] + char)  # problem replaced
                else:
                    corrected_f.write(buffer + "(" + "|".join(r_words) + ")" + char)  # suggested solutions
                buffer = ""
            else:
                buffer += char

    corrected_f.close()


if '__main__' == __name__:
    main(sys.argv)