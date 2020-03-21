<?php
    session_start();

    $uname = strtolower(php_uname('s'));

    if(count($_FILES)) {
        
        avrConnect("usbtiny",0);

        $file = $_FILES['file']['tmp_name'];
        $fuses = "";

        if (strpos($uname, "darwin") !== false) {
            $command = "/usr/local/bin/avrdude";
        }else if (strpos($uname, "win") !== false) {
            $command = "avrdude.exe";
        }else{
            $command = "avrdude";
        }

        if($_SESSION["chip"] == "ATtiny13"){
        	$fuses = " -U hfuse:w:0xFF:m -U lfuse:w:0x6A:m"; //CPU @ 1.2Mhz
            //$fuses = " -U hfuse:w:0xFF:m -U lfuse:w:0x7B:m"; //CPU @ 128Khz
        }else if($_SESSION["chip"] == "ATtiny45"){
        	$fuses = " -U hfuse:w:0xDF:m -U lfuse:w:0x62:m";
        }else if($_SESSION["chip"] == "ATtiny85"){
        	$fuses = " -U hfuse:w:0xDF:m -U lfuse:w:0x62:m";
        }

        $command .= " -c " . $_SESSION["usb"] . " -p " . $_SESSION["chip"] . $fuses . " -U flash:w:" . $file . ":i";
        /*
        if (strpos($file, ".hex") !== false) {
            $command .= ":i";
        }else{
            $command .= ":r";
        }
        */
        $output = shell_exec($command. " 2>&1");

        if (strpos($output, "flash verified") !== false) {
            header("Refresh:4; url=index.html");
            echo "Firmware Updated!";
        }else{
            echo "<pre>";
            echo $command;
            echo $output;
            echo "</pre>";
        }
    }
    else if(isset($_GET["connect"]))
    {
        echo avrConnect("usbtiny",0);
    }
    else if(isset($_GET["reset"]))
    {
        if (strpos($uname, "darwin") !== false) {
            $command = "/usr/local/bin/avrdude";
        }else if (strpos($uname, "win") !== false) {
            $command = "avrdude.exe";
        }else{
            $command = "avrdude";
        }

        $command .= " -c " . $_SESSION["usb"] . " -p " .$_SESSION["chip"]. " -Ulfuse:v:0x00:m";

        $output = shell_exec($command. " 2>&1");
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
    else if(isset($_GET["eeprom"]))
    {
        if (strpos($uname, "darwin") !== false) {
            $command = "/usr/local/bin/avrdude";
            //$tmp_dir = "/tmp";
            $tmp_dir = sys_get_temp_dir();
        }else if (strpos($uname, "win") !== false) {
            $command = "avrdude.exe";
            $tmp_dir = sys_get_temp_dir();
        }else{
            $command = "avrdude";
            $tmp_dir = "/tmp";
        }

        $eeprom_file = "/attiny-read.eeprom";

        if($_GET["eeprom"] == "erase") {

            header("Refresh:3; url=index.html");

            $esize = 64;
            if($_SESSION["chip"] == "ATtiny45"){
                $esize = 256;
            }else if($_SESSION["chip"] == "ATtiny85"){
                $esize = 512;
            }

        	$f = fopen($tmp_dir . $eeprom_file, 'wb');
			for ($i=0; $i<$esize; $i++) {
			    fwrite($f, pack("C*", 0xFF));
			}
			fclose($f);

            $command .= " -c " . $_SESSION["usb"] . " -p " .$_SESSION["chip"]. " -U eeprom:w:" . $tmp_dir . $eeprom_file .":r";
        }else if($_GET["eeprom"] == "flash") {
            $command .= " -c " . $_SESSION["usb"] . " -p " .$_SESSION["chip"]. " -U flash:w:" . dirname(__FILE__) . "/firmware/" . strtolower($_SESSION["chip"]) . ".hex:i";
        }else{
            $command .= " -c " . $_SESSION["usb"] . " -p " .$_SESSION["chip"]. " -U eeprom:r:" . $tmp_dir . $eeprom_file .":r";
        }
        
        $output = shell_exec($command. " 2>&1");

        if (file_exists($tmp_dir . $eeprom_file)) {

            $fsize = filesize($tmp_dir . $eeprom_file);
            $f = fopen($tmp_dir . $eeprom_file,'rb+');

            if($f)
            {
                $binary = fread($f, $fsize);
                $unpacked = unpack('C*', $binary);
 
                if($_GET["eeprom"] == "write") {

	        		if (strpos($_GET["offset"],",") !== false) //Multi-value support
	        		{
	            		$offset_array = explode(",",$_GET["offset"]);
		          		$value_array = explode(",",$_GET["value"]);

			            for ($x = 0; $x < count($offset_array); $x++)
			            {
			            	//echo "> " .$offset_array[$x]. " " .$value_array[$x]. "\n";

			            	fseek($f, intval($offset_array[$x]), SEEK_SET);

			            	if(intval($value_array[$x]) > 255) { //too big for uint8, split

			            		$lo_hi = [(intval($value_array[$x]) & 0xFF), (intval($value_array[$x]) >> 8)]; //0xAAFF = { 0xFF, 0xAA }
			            		print_r($lo_hi);
								echo "Bitwise: " .($lo_hi[0] | $lo_hi[1] << 8) . "\n";
								
								fwrite($f, pack('c', $lo_hi[0]));
								fwrite($f, pack('c', $lo_hi[1]));
			            	}else{
								fwrite($f, pack('c', intval($value_array[$x])));
			            	}
			            }
			        }else{
						fseek($f, intval($_GET["offset"]), SEEK_SET);
						fwrite($f, pack('c', $_GET["value"]));
			        }

                    rewind($f);
                    $binary = fread($f, $fsize);
                    $unpacked = unpack('C*', $binary);
                    foreach($unpacked as $value) {
                        echo $value. "\n";
                    }
                    fclose($f);
                    
                    $command = str_replace("eeprom:r:", "eeprom:w:", $command);

					echo $tmp_dir . $eeprom_file . "\n";
                    //echo $command . "\n";

                    $output = shell_exec($command. " 2>&1");

                    unlink($tmp_dir . $eeprom_file);
                    exit;
                }else{
                    foreach($unpacked as $value) {
                        echo $value. "\n";
                    }
                }

                fclose($f);
                unlink($tmp_dir . $eeprom_file);
            }else{
                echo $command;
            }
        } else {
            echo $output;
        }
    }

    function avrConnect($programmer,$timeout)
    {
        $uname = strtolower(php_uname('s'));

        if (strpos($uname, "darwin") !== false) {
            $command = "/usr/local/bin/avrdude -c " . $programmer . " -p t13 -n";
        }else if (strpos($uname, "win") !== false) {
            $command = "avrdude.exe -c " . $programmer . " -p t13 -n";
        }else{
            $command = "avrdude -c " . $programmer . " -p t13 -n";
        }

        $output = shell_exec($command. " 2>&1");

        session_unset();
        session_destroy();
        session_start();

        $_SESSION["usb"] = $programmer;
        $_SESSION["chip"] = "";

        if (strpos($output, "0x1e9007") !== false) {
            $_SESSION["chip"] = "ATtiny13";
        }else if (strpos($output, "0x1e9206") !== false) {
            $_SESSION["chip"] = "ATtiny45";
        }else if (strpos($output, "0x1e930b") !== false) {
            $_SESSION["chip"] = "ATtiny85";
        }else if (strpos($output, "could not find USB device") !== false) {

            if (strpos($uname, "darwin") !== false) {
                return "";
            }else if (strpos($uname, "win") !== false) {
                $command = "powershell.exe -ExecutionPolicy Bypass -Command \"Get-WmiObject Win32_PNPEntity | Where { \$_.HardwareID -like \\\"*VID_16C0*PID_05DC*\\\" }\"";
                $output = shell_exec($command . " 2>&1");

                if(strlen($output) > 0 )
                {
                    return "fix";
                }
                //echo $output;
            }
        }else{
            if($timeout < 4 && strpos($output, "error:") == false) {
                sleep(1);
                $output = avrConnect("usbasp",$timeout++);
            }else{
                return "sck";
            }
            return $output;
        }

        return $_SESSION["chip"];
    }
?>