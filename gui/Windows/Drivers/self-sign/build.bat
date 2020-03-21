set IlcPath=C:\corert\bin\Windows_NT.x64.Release
dotnet publish -c release
move bin\release\netcoreapp5.0\win-x64\publish\self-sign.exe ..\
pause