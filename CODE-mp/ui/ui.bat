@set include=
@set savedpath=%path%
@set path=%path%;..\..\..\bin

del /q vm
if not exist vm\nul mkdir vm
cd vm

set cc=..\..\..\bin\lcc -DQ3_VM -S -Wf-target=bytecode -Wf-g -I..\..\cgame -I..\..\game -I..\..\ui %1

%cc% ../ui_main.c
@if errorlevel 1 goto pause
%cc% ../../game/bg_misc.c
@if errorlevel 1 goto pause
%cc% ../../game/bg_weapons.c
@if errorlevel 1 goto pause
%cc% ../../game/bg_lib.c
@if errorlevel 1 goto pause
%cc% ../../game/q_math.c
@if errorlevel 1 goto pause
%cc% ../../game/q_shared.c
@if errorlevel 1 goto pause
%cc% ../ui_atoms.c
@if errorlevel 1 goto pause
%cc% ../ui_force.c
@if errorlevel 1 goto pause
%cc% ../ui_shared.c
@if errorlevel 1 goto pause
%cc% ../ui_gameinfo.c
@if errorlevel 1 goto pause
%cc% ../ui_util.c
@if errorlevel 1 goto pause

..\..\..\bin\q3asm -f ../ui
@if errorlevel 1 goto pause

mkdir "..\..\base\vm"
copy *.qvm "..\..\base\vm"
goto quit

:pause
pause

:quit
cd ..
del ".\vm" /f /s /q
rmdir ".\vm"

:quit
cd ..