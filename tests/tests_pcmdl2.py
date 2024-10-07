from w32_lex import *

# Requires Windows and special STDARGVxxx.DLL to compare results

from ctypes import *
from ctypes import windll, wintypes

CommandLineToArgvW = windll.shell32.CommandLineToArgvW
CommandLineToArgvW.argtypes = [wintypes.LPCWSTR, POINTER(c_int)]
CommandLineToArgvW.restype = POINTER(wintypes.LPWSTR)

LocalFree = windll.kernel32.LocalFree
LocalFree.argtypes = [wintypes.HLOCAL]
LocalFree.restype = wintypes.HLOCAL

# does NOT add dummy foo.exe here, but split full command line

def ctypes_split(s):
    argc = c_int()
    argv = CommandLineToArgvW(s, byref(argc))
    result = [argv[i] for i in range(0, argc.value)]
    LocalFree(argv)
    return result

stdargv = CDLL('.\\STDARGV98.dll') # built from slightly modified CRT sources
#~ stdargv = CDLL('.\\STDARGV2008.dll')
#~ stdargv = CDLL('.\\STDARGV2015.dll')

def parse_cmdline(s):
    numargs = c_int(0)
    numchars = c_int(0)
    cmdline = create_string_buffer(s.encode())
    # void parse_cmdline(char *cmdstart, char **argv, char *args, int *numargs, int *numchars);
    stdargv.parse_cmdline(cmdline, c_void_p(0), c_char_p(0), byref(numargs), byref(numchars))

    argv = (c_char_p * numargs.value)()
    args = create_string_buffer(numchars.value)
    stdargv.parse_cmdline(cmdline, argv, args, byref(numargs), byref(numchars))

    # build a result list similar to ctypes_split
    r = []
    for i in range(0, numargs.value-1): # omit last NULL (None)
        r += [argv[i].decode()] # returns str, not bytes
    return r

examples = [
    (r'\"a     "\"b   \"c" \\\\\\"', ['\\a     \\b   \\c \\\\\\\\\\\\']),
    (r'a.exe a "b c" d', []),
    (r'"a.exe" a "b c" d', []),
    (r'"a.exe" a "b c" d', []),
    (r'"c:\a\b\c\a.exe" a "b c" d', []),
    (r'"c:\a\b c\a.exe" a "b c" d', []),
    (r'\"c:\a\b c\a.exe\" a "b c" d', ['\\c:\\a\\b c\\a.exe\\', 'a', 'b c', 'd']),
    (r'"c:\a\b c\a.exe a "b c" d', ['c:\\a\\b c\\a.exe a b', 'c d']),
]

n=0
m = 0
for ex in examples:
    a, b, c = split(ex[0], 3), parse_cmdline(ex[0]), ctypes_split(ex[0])
    if a != b:
        print ('case=<%s>: split!=parse_cmdline: %s != %s' %(ex[0],a,b))
        n+=1
    if b != c:
        print ('case=<%s>: parse_cmdline!=ctypes_split: %s != %s' %(ex[0],b,c))
        m+=1
if n:
    print('%d/%d tests failed (split!=parse_cmdline)' % (n,len(examples)))
if m:
    print('%d/%d tests failed (parse_cmdline!=ctypes_split)' % (m,len(examples)))
if not m and not n:
    print('All %d tests passed!'%len(examples))
