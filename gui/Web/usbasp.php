<?php
    session_start();

    $uname = strtolower(php_uname('s'));

    if(count($_FILES)) {
        
        avrConnect();

        $file = $_FILES['file']['tmp_name'];

        if (strpos($uname, "darwin") !== false) {
            $command = "/usr/local/bin/avrdude";
        }else if (strpos($uname, "win") !== false) {
            $command = "avrdude.exe";
        }else{
            $command = "avrdude";
        }

        $command .= " -c usbasp -p " .$_SESSION["chip"]. " -U hfuse:w:0xFF:m -U lfuse:w:0x6A:m -U flash:w:" . $file . ":i";
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
            echo $command;
            echo $output;
        }
    }
    else if(isset($_GET["connect"]))
    {
        echo avrConnect();
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

        $command .= " -c usbasp -p " .$_SESSION["chip"]. " -Ulfuse:v:0x00:m";

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

        $command .= " -c usbasp -p " .$_SESSION["chip"]. " -U eeprom:r:" . $tmp_dir . $eeprom_file .":r";

        if($_GET["eeprom"] == "erase") {

        	$f = fopen($tmp_dir . $eeprom_file, 'wb');
			for ($i=0; $i<64; $i++) {
			    fwrite($f, pack("C*", 0xFF));
			}
			fclose($f);

        	$command = str_replace("eeprom:r:", "eeprom:w:", $command);
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

    function avrConnect()
    {
        $uname = strtolower(php_uname('s'));

        if (strpos($uname, "darwin") !== false) {
            $command = "/usr/local/bin/avrdude -c usbasp -p t13 -n";
        }else if (strpos($uname, "win") !== false) {
            $command = "avrdude.exe -c usbasp -p t13 -n";
        }else{
            $command = "avrdude -c usbasp -p t13 -n";
        }

        $output = shell_exec($command. " 2>&1");

        session_unset();
        session_destroy();
        session_start();

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
                $output = shell_exec($command. " 2>&1");

                if(strlen($output) > 0 )
                {
                    return "fix";
                }
                //echo $output;
            }
        }else{
            return"sck";
        }

        return $_SESSION["chip"];
    }
?>