$cert = Get-ChildItem cert:\CurrentUser\My | where { $_.Subject -eq "CN=ATtiny13 Plant" }

if ($cert.length -eq 0)
{
    $cert = New-SelfSignedCertificate -Subject "ATtiny13 Plant" -Type CodeSigningCert -NotAfter (Get-Date).AddMonths(60) -CertStoreLocation cert:\CurrentUser\My
}

Export-Certificate -Cert $cert -FilePath $PSScriptRoot\usbtiny.cer

#inf2cat.exe /driver:$PSScriptRoot /os:10_X64 /verbose

Set-AuthenticodeSignature $PSScriptRoot\usbtiny.cat -Certificate $cert -TimeStampServer "http://timestamp.comodoca.com"
Get-AuthenticodeSignature $PSScriptRoot\usbtiny.cat -Verbose | fl

#Import-Certificate -FilePath $PSScriptRoot\usbtiny.cer -CertStoreLocation cert:\LocalMachine\Root
#Import-Certificate -FilePath $PSScriptRoot\usbtiny.cer -CertStoreLocation cert:\LocalMachine\TrustedPublisher