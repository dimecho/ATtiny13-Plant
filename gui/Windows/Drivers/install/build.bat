::Tlbimp.exe /out:Microsoft.CertificateServices.Certenroll.Interop.dll C:\WINDOWS\System32\CertEnroll.dll
::Tlbimp.exe /out:Microsoft.CertificateServices.Certenroll.Interop.dll C:\WINDOWS\SysWOW64\CertEnroll.dll
C:\Windows\Microsoft.NET\Framework\v4.0.30319\csc.exe -optimize+ -nostdlib install.cs ^
/reference:System.dll ^
/reference:System.Security.Cryptography.X509Certificates.dll ^
/reference:Microsoft.CertificateServices.Certenroll.Interop.dll ^
/reference:mscorlib.dll
move install.exe ..\
pause