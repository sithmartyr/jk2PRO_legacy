@set include=
@set savedpath=%path%
@set path=%path%;..\..\..\bin

del /q vm
if not exist vm\nul mkdir vm
cd vm
set cc=..\..\..\bin\lcc -DQ3_VM -DMISSIONPACK -DCGAME -S -Wf-target=bytecode -Wf-g -I..\..\cgame -I..\..\game -I..\..\ui %1

%cc% ../../game/bg_misc.c
@if errorlevel 1 goto pause
%cc% ../../game/bg_weapons.c
@if errorlevel 1 goto pause
%cc% ../../game/bg_panimate.c
@if errorlevel 1 goto pause
%cc% ../../game/bg_pmove.c
@if errorlevel 1 goto pause
%cc% ../../game/bg_slidemove.c
@if errorlevel 1 goto pause
%cc% ../../game/bg_lib.c
@if errorlevel 1 goto pause
%cc% ../../game/bg_saber.c
@if errorlevel 1 goto pause
%cc% ../../game/q_math.c
@if errorlevel 1 goto pause
%cc% ../../game/q_shared.c
@if errorlevel 1 goto pause
%cc% ../cg_consolecmds.c
@if errorlevel 1 goto pause
%cc% ../cg_draw.c
@if errorlevel 1 goto pause
%cc% ../cg_drawtools.c
@if errorlevel 1 goto pause
%cc% ../cg_effects.c
@if errorlevel 1 goto pause
%cc% ../cg_ents.c
@if errorlevel 1 goto pause
%cc% ../cg_event.c
@if errorlevel 1 goto pause
%cc% ../cg_info.c
@if errorlevel 1 goto pause
%cc% ../cg_light.c
@if errorlevel 1 goto pause
%cc% ../cg_localents.c
@if errorlevel 1 goto pause
%cc% ../cg_main.c
@if errorlevel 1 goto pause
%cc% ../cg_marks.c
@if errorlevel 1 goto pause
%cc% ../cg_players.c
@if errorlevel 1 goto pause
%cc% ../cg_playerstate.c
@if errorlevel 1 goto pause
%cc% ../cg_predict.c
@if errorlevel 1 goto pause
%cc% ../cg_saga.c
@if errorlevel 1 goto pause
%cc% ../cg_scoreboard.c
@if errorlevel 1 goto pause
%cc% ../cg_servercmds.c
@if errorlevel 1 goto pause
%cc% ../cg_snapshot.c
@if errorlevel 1 goto pause
%cc% ../cg_turret.c
@if errorlevel 1 goto pause
%cc% ../cg_view.c
@if errorlevel 1 goto pause
%cc% ../cg_weaponinit.c
@if errorlevel 1 goto pause
%cc% ../cg_weapons.c
@if errorlevel 1 goto pause
%cc% ../fx_blaster.c
@if errorlevel 1 goto pause
%cc% ../fx_bowcaster.c
@if errorlevel 1 goto pause
%cc% ../fx_bryarpistol.c
@if errorlevel 1 goto pause
%cc% ../fx_demp2.c
@if errorlevel 1 goto pause
%cc% ../fx_disruptor.c
@if errorlevel 1 goto pause
%cc% ../fx_flechette.c
@if errorlevel 1 goto pause
%cc% ../fx_heavyrepeater.c
@if errorlevel 1 goto pause
%cc% ../fx_rocketlauncher.c
@if errorlevel 1 goto pause
%cc% ../fx_force.c
@if errorlevel 1 goto pause
%cc% ../../ui/ui_shared.c
@if errorlevel 1 goto pause
%cc% ../cg_newDraw.c
@if errorlevel 1 goto pause

..\..\..\bin\q3asm -f ../cgame
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