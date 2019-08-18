<?php
    $soil = array(280, 388, 460);
    $pot = array(12, 32, 64);
    $uname = strtolower(php_uname('s'));

    if(isset($_GET["os"]))
    {
        $uname = strtolower(php_uname('s'));
        if (strpos($uname, "darwin") !== false) {
            echo "mac";
        }else if (strpos($uname, "win") !== false) {
            echo "windows";
        }else{
            echo "linux";
        }
    }
    else if(isset($_GET["com"]))
    {
        if (strpos($uname, "darwin") !== false) {
            $command = "ls /dev/cu.*";
        }else if (strpos($uname, "win") !== false) {
            $command = "powershell.exe -ExecutionPolicy Bypass -Command \"Write-Host $([System.IO.Ports.SerialPort]::GetPortNames())\"";
        }else{
            $command = "ls /dev/ttyUSB*";
        }

        exec($command, $output, $return);
        $output = str_replace(" ", "\n", $output); //Windows Fix

        foreach ($output as $line) {
            if (strpos($line, "Bluetooth") == false) {
                echo "$line\n";
            }
        }
    }
    else if(isset($_GET["flash"]))
    {
        $hex = getcwd(). "/firmware/";

        if($_GET["solar"] == 1)
            $hex .= "solar/";

        $hex .= "main-" .$soil[$_GET["soil"]]. "-" .$pot[$_GET["pot"]]. ".hex";

        if (strpos($uname, "darwin") !== false) {
            $command = "../bootloader -d " .$_GET["flash"]. " -b 9600 -p " .$hex;
        }else if (strpos($uname, "win") !== false) {
            $command = "../fboot.exe -c " .$_GET["flash"]. " -b 9600 -p " .$hex;
        }else{
            $command = "../bootloader -d " .$_GET["flash"]. " -b 9600 -p " .$hex;
        }

        $output = shell_exec($command);

        echo $output;
    }
?>