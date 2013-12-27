@echo off

REM arguments:

    REM event type:
    set etype=%1
    REM configuration name:
    set cfgname=%2

REM ### Event handling #

goto :Event_%etype%


REM ### Pre-Build

    :Event_PRE
        if "%cfgname%" == "Release" (
            revision.vbs
        )
    goto end

REM ### Post-Build

    :Event_POST
    goto end

REM ### Cancel-Build

    :Event_CANCEL
    goto end


:end