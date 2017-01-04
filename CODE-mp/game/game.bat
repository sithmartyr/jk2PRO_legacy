@set include=
@set savedpath=%path%
@set path=%path%;..\..\..\bin

del /q vm
if not exist vm\nul mkdir vm
cd vm
set cc=..\..\..\bin\lcc -A -DQ3_VM -DMISSIONPACK -S -Wf-target=bytecode -Wf-g -I..\..\cgame -I..\..\game -I..\..\ui %1

%cc%  ../g_main.c
@if errorlevel 1 goto pause

%cc%  ../g_syscalls.c
@if errorlevel 1 goto pause

%cc%  ../bg_misc.c
@if errorlevel 1 goto pause
%cc%  ../bg_lib.c
@if errorlevel 1 goto pause
%cc%  ../bg_pmove.c
@if errorlevel 1 goto pause
%cc%  ../bg_saber.c
@if errorlevel 1 goto pause
%cc%  ../bg_slidemove.c
@if errorlevel 1 goto pause
%cc%  ../bg_panimate.c
@if errorlevel 1 goto pause
%cc%  ../bg_weapons.c
@if errorlevel 1 goto pause
%cc%  ../q_math.c
@if errorlevel 1 goto pause
%cc%  ../q_shared.c
@if errorlevel 1 goto pause

%cc%  ../ai_main.c
@if errorlevel 1 goto pause
%cc%  ../ai_util.c
@if errorlevel 1 goto pause
%cc%  ../ai_wpnav.c
@if errorlevel 1 goto pause

%cc%  ../g_account.c
@if errorlevel 1 goto pause

%cc%  ../g_active.c
@if errorlevel 1 goto pause

%cc%  ../g_arenas.c
@if errorlevel 1 goto pause
%cc%  ../g_bot.c
@if errorlevel 1 goto pause
%cc%  ../g_client.c
@if errorlevel 1 goto pause
%cc%  ../g_cmds.c
@if errorlevel 1 goto pause
%cc%  ../g_combat.c
@if errorlevel 1 goto pause
%cc%  ../g_items.c
@if errorlevel 1 goto pause
%cc%  ../g_log.c
@if errorlevel 1 goto pause
%cc%  ../g_mem.c
@if errorlevel 1 goto pause
%cc%  ../g_misc.c
@if errorlevel 1 goto pause
%cc%  ../g_missile.c
@if errorlevel 1 goto pause
%cc%  ../g_mover.c
@if errorlevel 1 goto pause
%cc%  ../g_object.c
@if errorlevel 1 goto pause
%cc%  ../g_saga.c
@if errorlevel 1 goto pause
%cc%  ../g_session.c
@if errorlevel 1 goto pause
%cc%  ../g_spawn.c
@if errorlevel 1 goto pause
%cc%  ../g_svcmds.c
@if errorlevel 1 goto pause
%cc%  ../g_target.c
@if errorlevel 1 goto pause
%cc%  ../g_team.c
@if errorlevel 1 goto pause
%cc%  ../g_trigger.c
@if errorlevel 1 goto pause
%cc%  ../g_utils.c
@if errorlevel 1 goto pause
%cc%  ../g_weapon.c
@if errorlevel 1 goto pause
%cc%  ../w_force.c
@if errorlevel 1 goto pause
%cc%  ../w_saber.c
@if errorlevel 1 goto pause

q3asm -f ../game
@if errorlevel 1 goto pause

mkdir "..\..\base\vm"
copy *.qvm "..\..\base\vm"
goto quit

:pause
pause

:quit
cd ..