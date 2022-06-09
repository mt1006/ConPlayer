:: Packs files to zips and generates checksums

@echo off

set EXEC_NAME=cp.exe

md packed
del conpl_win_x86_32.zip
del conpl_win_x86_64.zip 

cd Release
copy %EXEC_NAME% conpl.exe
7z a -tzip conpl_win_x86_32.zip conpl.exe -ir!*.dll ..\LICENSE.txt
del Release\conpl.exe
cd ..
move Release\conpl_win_x86_32.zip packed

cd x64\Release
copy %EXEC_NAME% conpl.exe
7z a -tzip conpl_win_x86_64.zip conpl.exe -ir!*.dll ..\..\LICENSE.txt
del x64\Release\conpl.exe
cd ..\..
move x64\Release\conpl_win_x86_64.zip packed

ch -g "packed\conpl_win_x86_32.zip" "packed\conpl_win_x86_64.zip" >  packed\conpl_win_sha256.txt