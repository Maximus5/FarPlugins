rem This will reset converted (from old VC9) projects to our defaults for VC12

for %%g in (*.vcxproj) do powershell -noprofile -command "& {%~dp0FixProj.ps1 %%g}"
