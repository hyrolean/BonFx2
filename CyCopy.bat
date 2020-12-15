echo off
REM -*- NOTICE -*-
REM Before compiling BonFx2 open source contents, you must install SuiteUSB 3.4.
REM After installing SuiteUSB 3.4, run this batch script to fulfill lack of CyApi contents.
REM You can get SuiteUSB 3.4 installer from this URL.
REM URL: http://www.cypress.com/?rID=34870  (ID:/getDgOc thanx!)
echo on

SET CyAPIDir=C:\Cypress\Cypress Suite USB 3.4.7\CyAPI

mkdir .\unified\libcyapi
mkdir .\unified\libcyapi\x64
copy "%CyAPIDir%\lib\x86\CyApi.lib" .\unified\libcyapi\.
copy "%CyAPIDir%\lib\x64\CyApi.lib" .\unified\libcyapi\x64\.
copy "%CyAPIDir%\inc\CyAPI.h" .\unified\.

pause
