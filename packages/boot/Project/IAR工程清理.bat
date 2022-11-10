@echo off

del /s /a *.lst *.pbi *.cout *.pbd *.browse *.wsdt *.map *.dni *.dep *.ewt *.dbgdt *.tmp 2>nul
for /r . %%d in (.) do rd /s /q "%%d\Debug" 2>nul
for /r . %%d in (.) do rd /s /q "%%d\Release" 2>nul
for /r . %%d in (.) do rd /s /q "%%d\settings" 2>nul

exit