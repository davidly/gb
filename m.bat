del gb.exe
del gb.pdb
cl /nologo gb.cxx /I.\ /Ox /Qpar /O2 /Oi /Ob2 /EHac /Zi /Gy /DNDEBUG /DUNICODE /D_AMD64_ /link ntdll.lib /OPT:REF


