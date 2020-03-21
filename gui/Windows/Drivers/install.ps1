function Elevate() {
    # Get the ID and security principal of the current user account
    $myWindowsID=[System.Security.Principal.WindowsIdentity]::GetCurrent()
    $myWindowsPrincipal = New-Object System.Security.Principal.WindowsPrincipal($myWindowsID)

    # Check to see if we are currently running "as Administrator"
    if (!$myWindowsPrincipal.IsInRole([System.Security.Principal.WindowsBuiltInRole]::Administrator)){
        Start-Process powershell -ArgumentList "-ExecutionPolicy Bypass -File ""$PSCommandPath""" -verb runas
        exit
    }
}

$cert = Get-ChildItem cert:\LocalMachine\Root | where { $_.Subject -eq "CN=ATtiny13 Plant" }

if ($cert.length -eq 0)
{
    Elevate
    Import-Certificate -FilePath $PSScriptRoot\usbtiny.cer -CertStoreLocation cert:\LocalMachine\Root
    Import-Certificate -FilePath $PSScriptRoot\usbtiny.cer -CertStoreLocation cert:\LocalMachine\TrustedPublisher
}

pnputil -a $PSScriptRoot\usbtiny.inf
InfDefaultInstall $PSScriptRoot\usbtiny.inf