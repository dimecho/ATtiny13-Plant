<?php
    $soil = array(280, 388, 460);
    $pot = array(12, 32, 64);
    $uname = strtolower(php_uname('s'));

	if(isset($_GET["connect"]))
    {
    	if (strpos($uname, "darwin") !== false) {
    		$command = "/usr/local/bin/avrdude -c usbasp -p ATtiny13 -B5";
    	}else if (strpos($uname, "win") !== false) {
            $command = "avrdude.exe -c usbasp -p ATtiny13 -B5";
        }else{
            $command = "avrdude -c usbasp -p ATtiny13 -B5";
        }

        $output = shell_exec($command. " 2>&1");

        if (strpos($output, "0x1e9007") !== false) {
            echo "ATtiny13";
        }else if (strpos($output, "0x1e9206") !== false) {
        	echo "ATtiny45";
        }else if (strpos($output, "0x1e930b") !== false) {
            echo "ATtiny85";
        }else if (strpos($output, "could not find USB device") !== false) {
            if (strpos($uname, "darwin") !== false) {
                echo "";
        	}else if (strpos($uname, "win") !== false) {
        		$command = "powershell.exe -ExecutionPolicy Bypass -Command \"Get-WmiObject Win32_PNPEntity | Where { \$_.HardwareID -like \\\"*VID_16C0*PID_05DC*\\\" }\"";
        		$output = shell_exec($command. " 2>&1");

        		if(strlen($output) > 0 )
        		{
        			echo "fix";
        		}
        		//echo $output;
        	}
        }else{
        	echo "sck";
        }
    }
    else if(isset($_GET["driver"]))
    {
    	if (strpos($uname, "win") !== false) {
	    	$command = "powershell.exe -ExecutionPolicy Bypass -Command \"Get-WmiObject Win32_PNPEntity | Where { \$_.HardwareID -like \\\"*VID_16C0*PID_05DC*\\\"} | Select -ExpandProperty HardwareID\"";
			$output = shell_exec($command. " 2>&1");

			$id = explode("\n", $output);
			$idtag = str_replace("&", ".", $id[0]);
			
	    	$command = "powershell.exe -ExecutionPolicy Bypass Start-Process \"" .sys_get_temp_dir(). "\install-filter.exe\" -ArgumentList \"install\", \"--device=" .$idtag. "\" -Verb runAs -Wait";
	    	//echo $command;
	        shell_exec($command);

	        echo "ok";
    	}
    }
    else if(isset($_GET["flash"]))
    {
        $hex = getcwd(). "/firmware/". $_GET["flash"] ."/";
        $fuses = "-Uhfuse:w:0xFF:m -Ulfuse:w:0x6A:m";

        //if($_GET["solar"] == 1)
        //    $hex .= "solar/";

        $hex .= "main-" .$soil[$_GET["soil"]]. "-" .$pot[$_GET["pot"]]. ".hex";

        if (strpos($uname, "darwin") !== false) {
            $command = "/usr/local/bin/avrdude -p " .$_GET["flash"]. " " .$fuses. " -U flash:w:" .$hex. ":i";
        }else if (strpos($uname, "win") !== false) {
             $command = "avrdude.exe -p " .$_GET["flash"]. " " .$fuses. " -U flash:w:" .$hex. ":i";
        }else{
            $command = "avrdude -p " .$_GET["flash"]. " " .$fuses. " -U flash:w:" .$hex. ":i";
        }

        $output = shell_exec($command);

        echo $output;
    }
?>