<?php
    session_start();
    set_time_limit(12);
    error_reporting(E_ALL);

    echo Request(0);

	function Request($timeout)
	{
	    $uname = strtolower(php_uname('s'));

        if (strpos($uname, "darwin") !== false) {
            $command = "\"" .getcwd(). "/avrdude\" -C \"" .getcwd(). "/avrdude.conf\"";
            //$tmp_dir = "/tmp";
            $tmp_dir = sys_get_temp_dir();
        }else if (strpos($uname, "win") !== false) {
            $command = "avrdude.exe";
            $tmp_dir = sys_get_temp_dir();
        }else{
            $command = "avrdude";
            $tmp_dir = "/tmp";
        }

	    if(count($_FILES)) {
	    	
	        $file = $_FILES['file']['tmp_name'];
	        $chip = "t13";
	        $fuses = "";

	        if($_SESSION["chip"] == "ATtiny13"){
	        	$fuses = " -U hfuse:w:0xFB:m -U lfuse:w:0x6A:m"; //CPU @ 1.2Mhz
	            //$fuses = " -U hfuse:w:0xFF:m -U lfuse:w:0x7B:m"; //CPU @ 128Khz
	        }else if($_SESSION["chip"] == "ATtiny45"){
	        	$fuses = " -U hfuse:w:0xDD:m -U lfuse:w:0x62:m";
	        }else if($_SESSION["chip"] == "ATtiny85"){
	        	$fuses = " -U hfuse:w:0xDD:m -U lfuse:w:0x62:m";
	        }

	        if($_SESSION["chip"] !== "")
	        	$chip = strtolower($_SESSION["chip"]);

	        $command .= " -c " . $_SESSION["usb"] . " -p " .$chip. $fuses . " -U flash:w:" . $file . ":i";
	        /*
	        if (strpos($file, ".hex") !== false) {
	            $command .= ":i";
	        }else{
	            $command .= ":r";
	        }
	        */
	        $output = Run($command);

	        if (strpos($output, "flash verified") !== false) {
	            header("Refresh:4; url=index.html");
	            return "Firmware Updated!";
	        }else{
	            $html = "<pre>";
	            $html .= $command. "\n";
	            $html .= $output;
	            $html .=  "</pre>";
	            return $html;
	        }
	    }
	    else if(isset($_GET["network"]))
	    {
	    	if($_GET["network"] == "ip") {
				$f = fopen(getcwd(). "/ip.txt", "w") or die();
				fclose($f);
				if (strpos($uname, "darwin") !== false) {
					$output = shell_exec("ifconfig | grep 'inet ' | grep -v '127.0.0.1' | cut -f2 -d' '| awk 'NR==1{print $1}' 2>&1");
					return $output;
				}
	    	}else if($_GET["network"] == "localhost") {
	    		@unlink(getcwd(). "/ip.txt");
	    	}
	    }
	    else if(isset($_GET["log"]))
	    {
	        $log_file = getcwd(). "/log.txt";

	        if (file_exists($log_file)) {
	            return file_get_contents($log_file);
	        }
	    }
	    else if(isset($_GET["connect"]))
	    {
	        return Connect("usbtiny",0);
	    }
	    else if(isset($_GET["reset"]))
	    {
	        $command .= " -c " . $_SESSION["usb"] . " -p t13 -Ulfuse:v:0x00:m";
	        $output = Run($command);
		    return $command. "\n" .$output;
		}
	    else if(isset($_GET["fuse"]))
	    {
	    	$command .= " -c " . $_SESSION["usb"] . " -p " .strtolower($_SESSION["chip"]);
	    	if(isset($_GET["h"])) {
	    		$command .= " -U hfuse:w:" .$_GET["h"]. ":m";
	    	}
	    	if(isset($_GET["l"])) {
	    		$command .= " -U lfuse:w:" .$_GET["l"]. ":m";
	    	}
	    	$output = Run($command);
		    return $command. "\n" .$output;
	    }
	    else if(isset($_GET["eeprom"]))
	    {
	        $eeprom_file = "/attiny.eeprom";

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

	            $command .= " -c " . $_SESSION["usb"] . " -p " .strtolower($_SESSION["chip"]). " -V -U eeprom:w:" .$tmp_dir . $eeprom_file.":r";
	            $output = Run($command);
	        }else if($_GET["eeprom"] == "flash") {
	            $command .= " -c " . $_SESSION["usb"] . " -p " .strtolower($_SESSION["chip"]). " -V -U flash:w:" .getcwd(). "/firmware/" .strtolower($_GET["firmware"]). ".hex:i";
	        }else{
	            $command .= " -c " . $_SESSION["usb"] . " -p " .strtolower($_SESSION["chip"]). " -U eeprom:r:" .$tmp_dir . $eeprom_file.":r";
	        }
	        
	        if(!file_exists($tmp_dir . $eeprom_file) || $_GET["eeprom"] == "read")
	        	$output = Run($command);
	        
	        if (file_exists($tmp_dir . $eeprom_file)) {

	            $fsize = filesize($tmp_dir . $eeprom_file);
	            $f = fopen($tmp_dir . $eeprom_file,'rb+');

	            if($f)
	            {
	                $binary = fread($f, $fsize);
	                $unpacked = unpack('C*', $binary);

	                $output = $tmp_dir . $eeprom_file . "\n";
	 
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
				            		//print_r($lo_hi);
									$output .= "Bitwise: " .($lo_hi[0] | $lo_hi[1] << 8) . "\n";
									
									fwrite($f, pack('c', $lo_hi[0]));
									fwrite($f, pack('c', $lo_hi[1]));
				            	}else{
									fwrite($f, pack('c', intval($value_array[$x])));
									fwrite($f, pack('c', 255));
				            	}
				            }
				        }else{
							fseek($f, intval($_GET["offset"]), SEEK_SET);
							fwrite($f, pack('c', intval($_GET["value"])));
							//fwrite($f, pack('c', 255));
				        }

	                    rewind($f);
	                    $binary = fread($f, $fsize);
	                    $unpacked = unpack('C*', $binary);
	                    foreach($unpacked as $value) {
	                        $output .= $value. "\n";
	                    }

	                    //sleep(1);
	                    //$command = str_replace("-U eeprom:r:", "-v -U eeprom:w:", $command);
	                    //$command = str_replace("-U eeprom:r:", "-B5 -v -U eeprom:w:", $command);
	                    $command = str_replace("-U eeprom:r:", $_SESSION["bitrate"]. "-U eeprom:w:", $command);
	                    $output .= Run($command);

	                }else{
	                    foreach($unpacked as $value) {
	                        $output .= $value. "\n";
	                    }
	                }
	                fclose($f);
	                unlink($tmp_dir . $eeprom_file);
	            }
	        }

	        return $output . "\n" . $command;
	    }
	}

	function Run($command)
    {
    	$output = "";
    	$timeout = 2;
    	$retry = array("rc=-1", "timed out", "output error", "libusb: debug", "initialization failed", "Broken pipe");

    	while ($timeout > 0) {
    		$output = shell_exec($command. " 2>&1");
    		$run = true;
    		foreach($retry as $item) {
    			if (strpos($output, $item) !== false) {
    				$run = false;
    				break;
    			}
			}
    		if ($run == true) {
    			return $output;
    		}
    		sleep(4);
    		$timeout--;
    	}

    	return $output;
    }

    function Connect($programmer,$timeout)
    {
        $uname = strtolower(php_uname('s'));

        if (strpos($uname, "darwin") !== false) {
            $command = "\"" .getcwd(). "/avrdude\" -C \"" .getcwd(). "/avrdude.conf\" -c " . $programmer . " -p t13 -n";
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
        $_SESSION["bitrate"] = "";
        $_SESSION["chip"] = "";

        if (strpos($output, "0x1e9007") !== false) {
            $_SESSION["chip"] = "ATtiny13";
        }else if (strpos($output, "0x1e9206") !== false) {
            $_SESSION["chip"] = "ATtiny45";
        }else if (strpos($output, "0x1e930b") !== false) {
            $_SESSION["chip"] = "ATtiny85";
        }else if (strpos($output, "initialization failed") !== false) {
            if(strlen($output) > 0 ) {
                return "fix";
            }
        }else{
            if($timeout < 4) {
            	if(strpos(strtolower($output), "error:") !== false) {
            		$_SESSION["bitrate"] = "-B250 ";
            		if($timeout > 2) {
		                if($programmer == "usbtiny") { //try another programmer
		                	$programmer = "usbasp";
		                }else{
		                	$programmer = "usbtiny";
		                }
	            	}
	                sleep(1);
	                $timeout++;
	                $output = Connect($programmer,$timeout);
				}
            }else{
                return "sck";
            }
            return $output;
        }
        $output = $_SESSION["chip"] . "\n" . $_SESSION["usb"];
        
        if($programmer == "usbasp" && ($_SESSION["chip"] == "ATtiny45" || $_SESSION["chip"] == "ATtiny85")) {
        	$output .= "\nUSBTiny";
        }
        return $output;
    }
?>