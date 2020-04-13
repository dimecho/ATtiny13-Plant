del ATtiny-Plant.exe
C:\Windows\Microsoft.NET\Framework\v4.0.30319\csc.exe -optimize+ -nostdlib ATtiny-Plant.cs /win32icon:icon.ico ^
/resource:Web.zip,APP.Web.zip ^
/resource:Drivers.zip,APP.Drivers.zip ^
/resource:AVR\avrdude.conf,AVR.avrdude.conf ^
/resource:AVR\avrdude.exe,AVR.avrdude.exe ^
/reference:System.dll ^
/reference:System.IO.Compression.dll ^
/reference:System.IO.Compression.FileSystem.dll ^
/reference:System.Security.Cryptography.X509Certificates.dll ^
/reference:mscorlib.dll ^
/resource:php-7.4.3-Win32-vc15-x64.zip,PHP.php-7.4.3-Win32-vc15-x64.zip ^
/resource:VC_redist.x64.exe,VC.VC_redist.x64.exe
pause