#!/usr/bin/env python3
"""Host-side regression contract for M.I.L.O's tiny assembly router."""

ACTIONS = {
    "help": "help", "commands": "help", "about": "about",
    "memory": "memory", "ram": "memory", "version": "version",
    "dir": "directory", "directory": "directory", "files": "directory",
    "list": "directory", "browse": "directory", "disk": "disk",
    "space": "disk", "storage": "disk", "traits": "traits",
    "patterns": "traits", "learning": "traits", "read": "read",
    "open": "read", "type": "read", "edit": "edit", "find": "find",
    "search": "find", "locate": "find", "show": "show", "tell": "tell",
    "clear": "clear", "status": "status", "health": "status",
    "clock": "clock", "date": "clock", "apps": "apps",
    "applications": "apps", "sound": "sound", "audio": "sound",
    "beep": "beep",
}

FILLERS = {
    "milo", "m.i.l.o", "please", "can", "could", "would", "hey", "hi",
    "hello", "will", "you", "me", "i", "s", "want", "need", "to",
    "the", "a", "an", "my", "our", "what", "is", "are", "how", "much",
    "do", "does", "we", "have", "system", "for",
}

GUARDED = {
    "delete": "guard-delete", "del": "guard-delete",
    "remove": "guard-delete", "erase": "guard-delete",
    "rename": "guard-rename", "ren": "guard-rename",
    "write": "guard-write", "create": "guard-write",
    "copy": "guard-copy", "duplicate": "guard-copy",
}

SOCIAL = {
    "hello": 1, "hey": 1, "hi": 1,
    "thank": 2, "thanks": 2,
    "bye": 4, "goodbye": 4,
    "who": 8, "name": 8,
    "how": 16, "what": 32, "can": 64, "do": 128,
}


def bounded_match(token, word):
    if token == word:
        return True
    if len(word) < 4 or abs(len(token) - len(word)) > 1:
        return False
    if len(token) == len(word):
        mismatches = [i for i, pair in enumerate(zip(token, word))
                      if pair[0] != pair[1]]
        if len(mismatches) == 1:
            return True
        return (len(mismatches) == 2 and mismatches[1] == mismatches[0] + 1
                and token[mismatches[0]] == word[mismatches[1]]
                and token[mismatches[1]] == word[mismatches[0]])
    longer, shorter = (token, word) if len(token) > len(word) else (word, token)
    for skipped in range(len(longer)):
        if longer[:skipped] + longer[skipped + 1:] == shorter:
            return True
    return False


def normalize(text):
    out = []
    for index, char in enumerate(text.lower()):
        if char.isalnum() or char in "-_":
            out.append(char)
        elif char == "." and index + 1 < len(text) and text[index + 1].isalnum():
            out.append(char)
        elif out and out[-1] != " ":
            out.append(" ")
    return "".join(out).strip()


def match_action(token):
    matches = {intent for word, intent in ACTIONS.items()
               if bounded_match(token, word)}
    if len(matches) > 1:
        return "ambiguous"
    return next(iter(matches), "conversation")


def match_guarded(token):
    matches = {intent for word, intent in GUARDED.items()
               if bounded_match(token, word)}
    if len(matches) > 1:
        return "guard-ambiguous"
    return next(iter(matches), "conversation")


def classify(text):
    tokens = normalize(text).split()
    while tokens and tokens[0] in FILLERS:
        tokens.pop(0)
    if not tokens:
        return "conversation"
    intent = match_action(tokens[0])
    if intent == "conversation":
        return match_guarded(tokens[0])
    if intent not in ("show", "tell"):
        return intent
    tokens.pop(0)
    while tokens and tokens[0] in FILLERS:
        tokens.pop(0)
    if not tokens:
        return "conversation"
    if intent == "show" and tokens[0] in ("file", "document"):
        return "read"
    nested = match_action(tokens[0])
    if nested != "conversation":
        return nested
    guarded = match_guarded(tokens[0])
    if guarded != "conversation":
        return guarded
    return "read" if intent == "show" else "conversation"


def classify_social(text):
    flags = 0
    for token in normalize(text).split():
        for word, flag in SOCIAL.items():
            if bounded_match(token, word):
                flags |= flag
    if flags & 2:
        return "thanks"
    if flags & 4:
        return "farewell"
    if flags & 8:
        return "identity"
    if flags & 16:
        return "wellbeing"
    if flags & 32 and flags & (64 | 128):
        return "capability"
    if flags & 1:
        return "greeting"
    return "generic"


def variants(word):
    yield word
    for index in range(len(word)):
        yield word[:index] + word[index + 1:]
        for replacement in "aeiourstln":
            yield word[:index] + replacement + word[index + 1:]
        if index + 1 < len(word):
            yield word[:index] + word[index + 1] + word[index] + word[index + 2:]
    for index in range(len(word) + 1):
        for inserted in "aeiourstln":
            yield word[:index] + inserted + word[index:]


def main():
    checks = {
        "reed README.TXT": "read",
        "seach README.TXT for tiny": "find",
        "find machinery in README.TXT": "find",
        "find README.TXT machinery": "find",
        "lsit files": "directory",
        "please show me the files": "directory",
        "M.I.L.O, could you open NOTES.TXT?": "read",
        "what's the version?": "version",
        "please show system health": "status",
        "what is the date?": "clock",
        "show applications": "apps",
        "audo": "sound",
        "how are you?": "conversation",
        "delet NOTES.TXT": "guard-delete",
        "tell me delet NOTES.TXT": "guard-delete",
        "remvoe NOTES.TXT": "guard-delete",
        "renam NOTES.TXT OLD.TXT": "guard-rename",
        "writ NOTES.TXT text": "guard-write",
        "copi NOTES.TXT OLD.TXT": "guard-copy",
    }
    for text, expected in checks.items():
        actual = classify(text)
        assert actual == expected, (text, expected, actual)

    assert normalize("Read README.TXT.") == "read readme.txt"
    assert bounded_match("opne", "open")
    assert bounded_match("serch", "search")
    assert not bounded_match("rm", "ram")
    assert match_action("hell") == "ambiguous"

    social_checks = {
        "hello M.I.L.O": "greeting",
        "thnaks for that": "thanks",
        "goodbye": "farewell",
        "who are you?": "identity",
        "how are you?": "wellbeing",
        "what can you do?": "capability",
        "ordinary local words": "generic",
    }
    for text, expected in social_checks.items():
        actual = classify_social(text)
        assert actual == expected, (text, expected, actual)

    # Refuse future keyword tables that make one typo resolve to two intents.
    candidates = set()
    for word in ACTIONS:
        if len(word) >= 4:
            candidates.update(variants(word))
    for candidate in candidates:
        intents = {intent for word, intent in ACTIONS.items()
                   if bounded_match(candidate, word)}
        if len(intents) > 1:
            assert match_action(candidate) == "ambiguous"

    guarded_candidates = set()
    for word in GUARDED:
        if len(word) >= 4:
            guarded_candidates.update(variants(word))
    for candidate in guarded_candidates:
        intents = {intent for word, intent in GUARDED.items()
                   if bounded_match(candidate, word)}
        assert len(intents) <= 1, (candidate, sorted(intents))

    print("M.I.L.O routing contract: %d operational + %d social phrases, "
          "%d typo candidates OK" %
          (len(checks), len(social_checks),
           len(candidates) + len(guarded_candidates)))


if __name__ == "__main__":
    main()
