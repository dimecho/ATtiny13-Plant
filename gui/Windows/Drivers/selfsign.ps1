$cert = New-SelfSignedCertificate -Subject "ATtiny13 Plant" -Type CodeSigningCert -NotAfter (Get-Date).AddMonths(60) -CertStoreLocation cert:\LocalMachine\My
$CertPassword = ConvertTo-SecureString -String "P@ss0wrd" -Force -AsPlainText
Export-PfxCertificate -Cert $cert -FilePath $PSScriptRoot\usbtiny.pfx -Password $CertPassword

..\selfsign\inf2cat.exe /driver:$PSScriptRoot /os:10_X64 /verbose

..\selfsign\signtool.exe sign /f $PSScriptRoot\usbtiny.pfx /p P@ss0wrd /t http://timestamp.verisign.com/scripts/timstamp.dll /v $PSScriptRoot\usbtiny.cat

..\selfsign\signtool.exe verify /v /pa $PSScriptRoot\usbtiny.cat

$cert = Get-ChildItem cert:\LocalMachine\My | where { $_.Subject -eq "CN=ATtiny13 Plant" }
Export-Certificate -Cert $cert -FilePath $PSScriptRoot\usbtiny.cer

Import-Certificate -FilePath $PSScriptRoot\usbtiny.cer -CertStoreLocation cert:\LocalMachine\Root
Import-Certificate -FilePath $PSScriptRoot\usbtiny.cer -CertStoreLocation cert:\LocalMachine\TrustedPublisher