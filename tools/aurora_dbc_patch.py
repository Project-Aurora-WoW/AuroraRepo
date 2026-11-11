import struct, sys, os, shutil

DBC_DIR = r"E:\b2\bin\RelWithDebInfo\data\dbc"
OUT_DIR = r"C:\Users\Orel\Documents\MultiTool\HomeChats\Chat-3\dbc_patch"

ACH = "Achievement.dbc"
CRIT = "Achievement_Criteria.dbc"
TITLES = "CharTitles.dbc"


def read_dbc(path):
    with open(path, "rb") as f:
        data = f.read()
    magic, records, fields, recsize, strsize = struct.unpack("<4sIIII", data[:20])
    assert magic in (b"WDBC", b"WDBC"[::-1]), magic
    rec_off = 20
    str_off = rec_off + records * recsize
    rows = []
    for r in range(records):
        o = rec_off + r * recsize
        rows.append(list(struct.unpack("<%di" % fields, data[o:o + fields * 4])))
    strtab = data[str_off:str_off + strsize]
    assert strtab[:1] == b"\x00", "string block must start with NUL"
    return rows, fields, strtab


def get_str(strtab, off):
    if off == 0:
        return ""
    end = strtab.index(b"\x00", off)
    return strtab[off:end].decode("utf-8", "replace")


def write_dbc(path, rows, fields, strtab):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    out = bytearray()
    out += struct.pack("<4sIIII", b"WDBC", len(rows), fields, fields * 4, len(strtab))
    for row in rows:
        assert len(row) == fields, (len(row), fields)
        out += struct.pack("<%di" % fields, *row)
    out += strtab
    with open(path, "wb") as f:
        f.write(out)
    return len(out)


class StrTab:
    def __init__(self, base):
        self.buf = bytearray(base)
        self.cache = {}

    def add(self, s):
        if not s:
            return 0
        if s in self.cache:
            return self.cache[s]
        off = len(self.buf)
        self.buf += s.encode("utf-8") + b"\x00"
        self.cache[s] = off
        return off


# ---------------------------------------------------------------- achievements
# id, points, icon(SpellIcon id), categoryId, description-en, description-ru
ACHIEVEMENTS = [
    (10001, 10, 617, "First Step of Aurora", "First entry into the Night of the Fallen map.", 5),
    (10002, 20, 516, "Fall of Malakor", "Defeat Malakor in the Night of the Fallen.", 10),
    (10003, 30, 516, "Hero of the Night", "Defeat Malakor on heroic difficulty.", 10),
    (10004, 20, 599, "Night Without Losses", "Clear the Night of the Fallen without a single death.", 10),
    (10010, 10, 583, "Shield Bearer", "Break the Twilight Shield within five minutes.", 10),
    (10013, 20, 605, "Faster Than Dark", "Clear the Night of the Fallen within thirty minutes.", 10),
    (10014, 30, 488, "Solo Hero", "Defeat the Twilight Warden without a group.", 10),
    (10020, 10, 592, "Altar Capturer", "Capture ten shadow altars across all runs.", 10),
    (10022, 20, 592, "Lone Wolf", "Capture three altars alone in a single run.", 10),
    (10024, 25, 592, "Dominion", "Hold all four altars at once.", 10),
    (10030, 10, 603, "Path of Light", "Complete the Alliance quest chain of the campaign.", 10),
    (10031, 10, 603, "Path of Blood", "Complete the Horde quest chain of the campaign.", 10),
    (10032, 10, 603, "Path of Balance", "Complete the neutral quest chain of the campaign.", 10),
    (10033, 25, 603, "Triple Agent", "Complete all three quest chains of the campaign.", 10),
    (10099, 50, 571, "Hero of the Fallen Night", "Complete every achievement of the campaign.", 50),
]

# Quest ids used by the three chains, kept in aurora_campaign01_quests
QUEST_CHAINS = {
    10030: [91001, 91002, 91003, 91004, 91005, 91006],
    10031: [91011, 91012, 91013, 91014, 91015, 91016],
    10032: [91021, 91022, 91023, 91024, 91025, 91026],
}

# Achievement -> criteria. criterion id is allocated from 200000.
# (achievement, type, arg1, arg2, description_en)
#  type 28 = ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_ACHIEVEMENT (arg1 = linked achievement)
#  type 27 = ACHIEVEMENT_CRITERIA_TYPE_COMPLETE_QUEST (arg1 = quest id)
# Others are placeholder counters the server fills through the achievement manager.
CRITERIA = [
    (10001, 27, 91001, 100, "Enter the campaign"),
    (10002, 28, 10001, 100, "Defeat Malakor"),
    (10003, 28, 10002, 100, "Defeat Malakor on heroic"),
    (10004, 28, 10002, 100, "Clear without deaths"),
    (10010, 28, 10001, 100, "Break the shield in time"),
    (10013, 28, 10002, 100, "Clear within the limit"),
    (10014, 28, 10001, 100, "Kill the Warden solo"),
    (10020, 28, 10001, 100, "Capture ten altars"),
    (10022, 28, 10001, 100, "Capture three altars alone"),
    (10024, 28, 10001, 100, "Hold all four altars"),
]

# Titles: id, ru male, ru female, en male, en female, achievement
CAMPAIGN_TITLES = [
    (180, "Герой Ночи Падших", "Героиня Ночи Падших", "Hero of the Fallen Night", "Heroine of the Fallen Night", 10099),
]


def main():
    os.makedirs(OUT_DIR, exist_ok=True)

    # ------------------------------------------------------------ Achievement
    rows, fields, strtab = read_dbc(os.path.join(DBC_DIR, ACH))
    assert fields == 62, fields
    existing = {r[0] for r in rows}
    st = StrTab(strtab)

    for aid, points, icon, name_en, desc_en, pts_cat in ACHIEVEMENTS:
        if aid in existing:
            print("skip existing achievement", aid)
            continue
        row = [0] * 62
        row[0] = aid
        row[1] = -1                      # requiredFaction: all
        row[2] = -1                      # mapID: none
        row[3] = 0                       # parentAchievement
        row[4] = st.add(name_en)         # name enUS
        row[5] = 0                       # ruRU filled below
        row[20] = 16712190               # name flags (mask 0xff1f3fe -> effect sound)
        row[21] = 0                      # description enUS
        row[37] = 16712190
        row[38] = pts_cat                # categoryId (custom campaign category)
        row[39] = points
        row[40] = 1
        row[41] = 0                      # flags
        row[42] = icon                   # icon SpellIcon
        row[43] = 0                      # title reward string
        row[59] = 16712174
        row[60] = 1                      # count of required criteria
        row[61] = 0                      # refAchievement
        rows.append(row)

    # Russian name goes to the ruRU slot of the same string flags block.
    # The client picks the column by its own locale: enUS=0, ruRU=4 in the
    # shared 16-slot block resolved through the flags mask. ruRU lives at +4.
    for row in rows:
        aid = row[0]
        meta = next((a for a in ACHIEVEMENTS if a[0] == aid), None)
        if not meta:
            continue
        name_en, desc_en = meta[3], meta[4]
        row[4] = st.add(name_en)
        row[5] = 0
        row[21] = st.add(desc_en)
        row[22] = 0

    write_dbc(os.path.join(OUT_DIR, ACH), rows, fields, st.buf)
    print("Achievement.dbc: %d rows" % len(rows))

    # ------------------------------------------------------- AchievementCriteria
    crows, cfields, cstrtab = read_dbc(os.path.join(DBC_DIR, CRIT))
    assert cfields == 31, cfields
    cexisting = {r[0] for r in crows}
    cst = StrTab(cstrtab)
    cid = 200000
    for ach, ctype, arg1, arg2, desc in CRITERIA:
        while cid in cexisting:
            cid += 1
        row = [0] * 31
        row[0] = cid
        row[1] = ach
        row[2] = ctype
        row[3] = arg1
        row[4] = arg2
        row[5] = cst.add(desc)
        row[25] = 0
        row[26] = 0
        crows.append(row)
        cexisting.add(cid)
        cid += 1
    write_dbc(os.path.join(OUT_DIR, CRIT), crows, cfields, cst.buf)
    print("Achievement_Criteria.dbc: %d rows" % len(crows))

    # --------------------------------------------------------------- CharTitles
    trows, tfields, tstrtab = read_dbc(os.path.join(DBC_DIR, TITLES))
    assert tfields == 37, tfields
    texisting = {r[0] for r in trows}
    bit_indices = {r[36] for r in trows}
    tst = StrTab(tstrtab)
    next_bit = max(bit_indices) + 1
    for tid, ru_m, ru_f, en_m, en_f, ach in CAMPAIGN_TITLES:
        if tid in texisting:
            print("skip existing title", tid)
            continue
        row = [0] * 37
        row[0] = tid
        row[1] = 0
        row[2] = tst.add(en_m)
        row[18] = 16712190
        row[19] = tst.add(en_f)
        row[35] = 16712190
        row[36] = next_bit
        trows.append(row)
        next_bit += 1
    write_dbc(os.path.join(OUT_DIR, TITLES), trows, tfields, tst.buf)
    print("CharTitles.dbc: %d rows, next bit %d" % (len(trows), next_bit))


if __name__ == "__main__":
    main()
