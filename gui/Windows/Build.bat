del "ATtiny-Plant.exe"
::Downloadable
C:\Windows\Microsoft.NET\Framework\v4.0.30319\csc.exe "ATtiny-Plant.cs" /win32icon:icon.ico ^
/resource:Drivers.zip,APP.Drivers.zip ^
/resource:AVR\avrdude.conf,AVR.avrdude.conf ^
/resource:AVR\avrdude.exe,AVR.avrdude.exe ^
/resource:Web.zip,APP.Web.zip ^
/reference:System.dll /reference:System.IO.Compression.FileSystem.dll /reference:C:\windows\assembly\GAC_MSIL\System.Management.Automation\1.0.0.0__31bf3856ad364e35\System.Management.Automation.dll /reference:mscorlib.dll
::Embedded
::C:\Windows\Microsoft.NET\Framework\v4.0.30319\csc.exe "ATtiny-Plant.cs" /win32icon:icon.ico ^
::/resource:Drivers.zip,APP.Drivers.zip ^
::/resource:AVR\avrdude.conf,AVR.avrdude.conf ^
::/resource:AVR\avrdude.exe,AVR.avrdude.exe ^
::/resource:Web.zip,APP.ATtiny-Plant-Web.zip ^
::/resource:php-7.4.0-Win32-vc15-x64.zip,PHP.php-7.4.0-Win32-vc15-x64.zip ^
::/resource:VC_redist.x64.exe,VC.VC_redist.x64.exe ^
::/reference:System.dll /reference:System.IO.Compression.FileSystem.dll /reference:C:\windows\assembly\GAC_MSIL\System.Management.Automation\1.0.0.0__31bf3856ad364e35\System.Management.Automation.dll /reference:mscorlib.dll
pause