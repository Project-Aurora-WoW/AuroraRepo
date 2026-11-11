"""Build the Aurora patch MPQ through StormLib's low-level file API.

The convenience functions (SFileAddFile/SFileAddFileEx) collapse several
names into one block in this DLL build, so this script creates the archive
and then writes each file through SFileCreateFile + SFileWriteFile +
SFileFinishFile, which keeps independent block entries.
"""

import ctypes
import os
from ctypes import wintypes

STORM = r"C:\Users\Orel\Downloads\WDBX.Editor\x64\StormLib.dll"
REPO = r"C:\Users\Orel\Documents\MultiTool\HomeChats\Chat-3\AuroraRepo"
DBC_SRC = os.path.join(REPO, "dbc")
OUT_MPQ = os.path.join(REPO, "mpq", "patch-Z.MPQ")

FILES = ["Achievement.dbc", "Achievement_Criteria.dbc", "CharTitles.dbc"]

MPQ_CREATE_ARCHIVE_V2 = 0x00000100
MPQ_FILE_COMPRESS = 0x00000200
MPQ_FILE_EXISTS = 0x80000000
MPQ_COMPRESSION_ZLIB = 0x02


def main():
    os.makedirs(os.path.dirname(OUT_MPQ), exist_ok=True)
    if os.path.exists(OUT_MPQ):
        os.remove(OUT_MPQ)

    storm = ctypes.WinDLL(STORM)

    storm.SFileCreateArchive.argtypes = [
        wintypes.LPCWSTR, wintypes.DWORD, wintypes.DWORD,
        ctypes.POINTER(wintypes.HANDLE)
    ]
    storm.SFileCreateArchive.restype = wintypes.BOOL

    storm.SFileCreateFile.argtypes = [
        wintypes.HANDLE, wintypes.LPCWSTR, wintypes.DWORD,
        wintypes.DWORD, wintypes.DWORD, wintypes.DWORD,
        ctypes.POINTER(wintypes.HANDLE)
    ]
    storm.SFileCreateFile.restype = wintypes.BOOL

    storm.SFileWriteFile.argtypes = [
        wintypes.HANDLE, ctypes.c_char_p, wintypes.DWORD, wintypes.DWORD
    ]
    storm.SFileWriteFile.restype = wintypes.BOOL

    storm.SFileFinishFile.argtypes = [wintypes.HANDLE]
    storm.SFileFinishFile.restype = wintypes.BOOL

    storm.SFileCloseArchive.argtypes = [wintypes.HANDLE]
    storm.SFileCloseArchive.restype = wintypes.BOOL

    h = wintypes.HANDLE()
    if not storm.SFileCreateArchive(OUT_MPQ, MPQ_CREATE_ARCHIVE_V2,
                                    len(FILES) + 8, ctypes.byref(h)):
        print("error: SFileCreateArchive failed")
        return 1

    try:
        for name in FILES:
            path = os.path.join(DBC_SRC, name)
            raw = open(path, "rb").read()
            archived = "DBFilesClient\\" + name

            fh = wintypes.HANDLE()
            ok = storm.SFileCreateFile(
                h, archived, 0, len(raw), 0,
                MPQ_FILE_EXISTS | MPQ_FILE_COMPRESS, ctypes.byref(fh))
            if not ok:
                print("error: SFileCreateFile failed for", name)
                return 1

            ok = storm.SFileWriteFile(fh, raw, len(raw), MPQ_COMPRESSION_ZLIB)
            if not ok:
                print("error: SFileWriteFile failed for", name)
                return 1

            if not storm.SFileFinishFile(fh):
                print("error: SFileFinishFile failed for", name)
                return 1

            print("wrote", archived, len(raw), "bytes")
    finally:
        storm.SFileCloseArchive(h)

    print("ok:", OUT_MPQ, os.path.getsize(OUT_MPQ), "bytes")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
