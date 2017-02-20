@echo off
setlocal
set s_flag=0
set SYS=""
set DEVICE="EVR0"
set FF="PCIe"




:loop
if x%1 equ x goto done
set param=%1
if %param:~0,1% equ - goto checkParam
:paramError
echo Parameter error: %1 

:next
shift /1
goto loop

:checkParam
if "%1" equ "-s" goto S
if "%1" equ "-d" goto D
if "%1" equ "-f" goto F
if "%1" equ "-h" goto H
goto paramError

:S
    shift /1
    set SYS=%1
    set s_flag=1
    goto next

:D
    shift /1
    set DEVICE=%1
    goto next

:F
    shift /1
    set FF=%1
    goto next

:H
    goto help

:done

if %s_flag%==0 GOTO :help
set macro="SYS=%SYS%,DEVICE=%DEVICE%,FF=%FF%"

call caqtdm -macro %macro% G_EVR_main.ui 

goto finish
:help

    echo Usage: %0 [options]
    echo Options:
    echo     -s [system name]     The system/project name
    echo     -d [EVR name]        Event Receiver / timing card name (default: %DEVICE%)
    echo     -f [form factor]     EVR form factor (default: %FF%)
    echo                          Choices: VME, PCIe, VME-300, PCIe-300DC
    echo     -h                   This help

:finish