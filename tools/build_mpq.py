"""Build a WoW 3.3.5a patch MPQ (v1) for the Aurora DBC files.

Written directly in Python because the StormLib build shipped with
WDBX.Editor collapses every added name into a single hash entry, so the
archive cannot be produced through it.

Layout written:
    [32-byte header]
    [file data]
    [hash table]   (encrypted with key "HASH")
    [block table]  (encrypted with key "BLOCK")
"""

import os
import struct
import zlib

REPO = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
DBC_SRC = os.path.join(REPO, "dbc")
OUT_MPQ = os.path.join(REPO, "mpq", "patch-Z.MPQ")

FILES = ["Achievement.dbc", "Achievement_Criteria.dbc", "CharTitles.dbc"]

MPQ_FILE_COMPRESS = 0x00000200
MPQ_FILE_EXISTS = 0x80000000
MPQ_COMPRESSION_ZLIB = 0x02

HASH_TABLE_SIZE = 16
SECTOR_SIZE = 512


# --------------------------------------------------------------- hash helpers
def _build_crypt_table():
    seed = 0x00100001
    table = [0] * 0x500
    for index1 in range(0x100):
        index2 = index1
        for _ in range(5):
            seed = (seed * 125 + 3) % 0x2AAAAB
            temp1 = (seed & 0xFFFF) << 0x10
            seed = (seed * 125 + 3) % 0x2AAAAB
            temp2 = seed & 0xFFFF
            table[index2] = temp1 | temp2
            index2 += 0x100
    return table


CRYPT = _build_crypt_table()


def hash_string(name, hash_type):
    seed1 = 0x7FED7FED
    seed2 = 0xEEEEEEEE
    for ch in name.upper().replace("/", "\\"):
        c = ord(ch)
        seed1 = CRYPT[(hash_type << 8) + c] ^ ((seed1 + seed2) & 0xFFFFFFFF)
        seed1 &= 0xFFFFFFFF
        seed2 = (c + seed1 + seed2 + (seed2 << 5) + 3) & 0xFFFFFFFF
    return seed1


def encrypt_table(data_bytes, key_text):
    key = hash_string(key_text, 3)
    words = list(struct.unpack("<%dI" % (len(data_bytes) // 4), data_bytes))
    seed1, seed2 = key & 0xFFFFFFFF, 0xEEEEEEEE
    out = []
    for word in words:
        seed2 = (seed2 + CRYPT[0x400 + (seed1 & 0xFF)]) & 0xFFFFFFFF
        ch = (word ^ (seed1 + seed2)) & 0xFFFFFFFF
        seed1 = (((~seed1 << 0x15) + 0x11111111) | (seed1 >> 0x0B)) & 0xFFFFFFFF
        seed2 = (word + seed2 + (seed2 << 5) + 3) & 0xFFFFFFFF
        out.append(ch)
    return struct.pack("<%dI" % len(out), *out)


# --------------------------------------------------------------------- builder
def main():
    os.makedirs(os.path.dirname(OUT_MPQ), exist_ok=True)

    header_size = 32
    data = bytearray()
    block_entries = []

    for name in FILES:
        src = os.path.join(DBC_SRC, name)
        if not os.path.isfile(src):
            print("error: missing source", src)
            return 1

        raw = open(src, "rb").read()
        packed = zlib.compress(raw, 9)

        flags = MPQ_FILE_EXISTS | MPQ_FILE_COMPRESS
        if len(packed) >= len(raw):
            packed = raw
            flags = MPQ_FILE_EXISTS

        offset = header_size + len(data)
        data += packed
        block_entries.append((offset, len(packed), len(raw), flags, name))
        print("packed %-26s %7d -> %7d bytes" % (name, len(raw), len(packed)))

    hash_table_pos = header_size + len(data)
    block_table_pos = hash_table_pos + HASH_TABLE_SIZE * 16

    # ---- hash table: one entry per file, indexed by (hashA & mask)
    hash_entries = [None] * HASH_TABLE_SIZE
    for index, (_, _, _, _, name) in enumerate(block_entries):
        archived = "DBFilesClient\\" + name
        slot = hash_string(archived, 0) & (HASH_TABLE_SIZE - 1)
        while hash_entries[slot] is not None:
            slot = (slot + 1) & (HASH_TABLE_SIZE - 1)

        hash_entries[slot] = struct.pack(
            "<IIHHI",
            hash_string(archived, 0),
            hash_string(archived, 1),
            0,  # locale neutral
            0,  # platform neutral
            index,
        )

    hash_raw = bytearray()
    for entry in hash_entries:
        hash_raw += entry if entry else struct.pack("<IIHHI", 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFF, 0xFFFF, 0xFFFFFFFF)

    block_raw = bytearray()
    for offset, csize, fsize, flags, _ in block_entries:
        block_raw += struct.pack("<IIII", offset, csize, fsize, flags)

    with open(OUT_MPQ, "wb") as fh:
        fh.write(
            struct.pack(
                "<4sIIHHIIII",
                b"MPQ\x1a",
                header_size,
                header_size + len(data) + len(hash_raw) + len(block_raw),
                0,   # v1
                3,   # sector size shift (512 << 3)
                hash_table_pos,
                block_table_pos,
                HASH_TABLE_SIZE,
                len(block_entries),
            )
        )
        fh.write(data)
        fh.write(encrypt_table(bytes(hash_raw), "(hash table)"))
        fh.write(encrypt_table(bytes(block_raw), "(block table)"))

    print("ok:", OUT_MPQ, os.path.getsize(OUT_MPQ), "bytes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
