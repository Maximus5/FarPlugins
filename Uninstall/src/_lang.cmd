@echo off
cd /d "%~dp0"
tools\lng.generator.exe -nc -i lang.ini -ol Lang.templ
