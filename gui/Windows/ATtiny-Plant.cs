using System;
using System.IO;
using System.Net;
//using System.Text;
using System.Diagnostics;
using System.Threading.Tasks;
using System.IO.Compression;
using System.Collections.Generic;
using System.Security.Principal;
using System.Security.Cryptography;
using System.Security.Cryptography.X509Certificates;
using System.Management;
using System.Reflection;
using Microsoft.Win32;

[assembly:AssemblyVersion("1.0.0.0")]

internal static class ATtinyPlant
{
	static string path = System.IO.Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location) + "\\";
	static string temp = Environment.GetEnvironmentVariable("TEMP") + "\\";
	static string profile = Environment.GetEnvironmentVariable("USERPROFILE") + "\\";
	static string appdata = Environment.GetFolderPath(Environment.SpecialFolder.CommonApplicationData)+ "\\ATtiny-Plant\\";

	//[STAThread]
    internal static void Main(string[] args)
    {
    	ServicePointManager.ServerCertificateValidationCallback = new System.Net.Security.RemoteCertificateValidationCallback(AcceptAllCertifications);
		ServicePointManager.SecurityProtocol |= SecurityProtocolType.Tls12;

		Console.ForegroundColor = ConsoleColor.Green;

		if(!checkMD5(appdata + @"Web\js\index.js", "2a751fc122c6d961eb3421c4c809b6cb"))
		{
			Directory.CreateDirectory(appdata);

			Console.WriteLine("\n...Extracting Web-Interface");
			extractEmbedResource("APP", "Web.zip", temp);
			extractZIP(temp + "Web.zip", appdata);

			extractEmbedResource("AVR", "avrdude.conf", appdata + @"Web\");
			extractEmbedResource("AVR", "avrdude.exe", appdata + @"Web\");
			//extractEmbedResource("AVR", "libusb0.dll", appdata + @"Web\");
		}

		if(!checkMD5(appdata + @"Drivers\usbtiny.cat", "cd87c2597f3286090aab6bf4b97f0a2d"))
		{
			Console.WriteLine("\n...Extracting Drivers");
			extractEmbedResource("APP", "Drivers.zip", temp);
			extractZIP(temp + "Drivers.zip", appdata);
		}
		
		if (!checkMD5(appdata + @"php\php.exe", "62e25ef43bf6da65017e5499e837efb6"))
		{
			Directory.CreateDirectory(appdata + "php");

			string phpFile = "php-7.4.3-Win32-vc15-x64.zip";
			extractEmbedResource("PHP", phpFile, temp);

			if (File.Exists(temp + phpFile)) {
				Console.WriteLine("\n...Extracting PHP");
				extractZIP(temp + phpFile, appdata + "php");
			}else{
				if (!File.Exists(profile+ @"Downloads\" + phpFile)) {
			   		Console.WriteLine("\n...Downloading PHP");

			   		using (WebClient webClient = new WebClient()) {
				   		try {
				   			webClient.Headers.Add("User-Agent", "Mozilla/4.0 (compatible; MSIE 8.0)");
							webClient.DownloadFile("https://windows.php.net/downloads/releases/archives/" + phpFile, profile + @"Downloads\" + phpFile);
				   		} catch (Exception ex) {
		                    Console.WriteLine(ex.ToString());
		                }
		            }
			   	}
			   	ZipFile.ExtractToDirectory(profile + @"Downloads\" + phpFile, appdata + "php");
			}

		   	Console.WriteLine("\n...Installing PHP");
			File.Move(appdata + @"php\php.ini-production", appdata + @"php\php.ini");
		}

		var vc2019 = Registry.LocalMachine.OpenSubKey(@"SOFTWARE\Classes\Installer\Dependencies\VC,redist.x64,amd64,14.25,bundle\Dependents\{6913e92a-b64e-41c9-a5e6-cef39207fe89}");
		if (vc2019 == null)
		{
			string vcFile = "VC_redist.x64.exe";
			extractEmbedResource("VC",vcFile,temp);

			if (File.Exists(temp + vcFile)) {
				vcFile = temp + vcFile;
			}else{
				if (!File.Exists(profile + @"Downloads\" + vcFile)) {
			   	
					Console.WriteLine("\n...Downloading C++ Redistributable for Visual Studio 2019");

					using (WebClient webClient = new WebClient()) {
				   		try {
				   			webClient.Headers.Add("User-Agent", "Mozilla/4.0 (compatible; MSIE 8.0)");
							webClient.DownloadFile("https://aka.ms/vs/16/release/" + vcFile, profile + @"Downloads\" + vcFile);
				   		} catch (Exception ex) {
		                    Console.WriteLine(ex.ToString());
		                }
		            }
				}
				vcFile = profile + @"Downloads\" + vcFile;
			}

			ProcessStartInfo start = new ProcessStartInfo();
			start.FileName = vcFile;
			using (Process proc = Process.Start(start))
			{
			    proc.WaitForExit();
			}
			//File.Delete(vcFile);
		}
		
		MainAsync(args).GetAwaiter().GetResult();
		//Console.Read();
    }

    private static async Task MainAsync(string[] args)
    {
    	
    	Process php = new Process();
		php.StartInfo.FileName = appdata + @"php\php.exe";
		if (File.Exists(appdata + "Web\\ip.txt")) {
			php.StartInfo.Arguments = "-S 0.0.0.0:8080 -t \"" + appdata + "Web\"";
		}else{
			php.StartInfo.Arguments = "-S 127.0.0.1:8080 -t \"" + appdata + "Web\"";
		}
		php.Start();
		
        var tcs = new TaskCompletionSource<object>();
        var server = new AsyncHttpServer(portNumber: 8080);
        //var task = Task.Run(() => server.Start());
        
        await Console.Out.WriteLineAsync("Listening on port 8080");
		
		checkDriver("VID_1781&PID_0C9F");
        //checkDriver("VID_16C0&PID_05DC");

        await tcs.Task;
        await server.Stop();
    }

    private static void runDriverInstall(ProcessStartInfo start, bool admin)
	{
		if(admin == true) {
			start.Verb = "runas";
			start.UseShellExecute = true;
			start.RedirectStandardOutput = false;
			start.RedirectStandardError = false;
			start.LoadUserProfile = true;
		}

		using (StreamWriter log = new StreamWriter(appdata + "Web\\log.txt"))
        {
	        try {
	           	Process p = Process.Start(start);
				log.WriteLine(p.StandardOutput.ReadToEnd());
				log.WriteLine(p.StandardError.ReadToEnd());
				p.WaitForExit();
	        } catch (Exception err) {
	            log.WriteLine(err.Message);
	            if(admin == false) {
	            	log.Close();
	        		runDriverInstall(start, true);
	            }
	        }
    	}
	}

	private static void checkDriver(string id)
	{
		Process browser = new Process();

		try
        {
            ManagementObjectSearcher searcher = new ManagementObjectSearcher("root\\CIMV2", "SELECT * FROM Win32_PNPEntity WHERE Status LIKE '%Error%' AND DeviceID LIKE '%" + id + "%'"); 

			if(searcher.Get().Count > 0)
			{
				browser.StartInfo.FileName = "http://127.0.0.1:8080/install.html";
				browser.Start();

				Console.WriteLine("...Validating Certificate");

				ProcessStartInfo start = new ProcessStartInfo();
				start.UseShellExecute = false;
			    start.RedirectStandardOutput = true;
			    start.RedirectStandardError = true;
			    
			    X509Certificate2 certificateToValidate = new X509Certificate2(X509Certificate2.CreateFromCertFile(appdata + "Drivers\\usbtiny.cer"));
				bool valid = certificateToValidate.Verify();
				
				//https://github.com/adafruit/Adafruit_Windows_Drivers
				if (!valid && certificateToValidate.Subject.IndexOf("Adafruit") == -1)
				{
					Console.WriteLine("Certificate Not Valid");
					
					start.FileName = "powershell.exe";
					start.Arguments = "-ExecutionPolicy Bypass -Command \".\\" + appdata + "Drivers\\self-sign.ps1\"";
					runDriverInstall(start, false);
				    
					start.FileName = "powershell.exe";
					start.Arguments = "-ExecutionPolicy Bypass -Command \".\\" + appdata + "Drivers\\install.ps1\"";
					runDriverInstall(start, false);
					
				}else{
					Console.WriteLine("...Installing Driver");

					start.FileName = "pnputil";
					start.Arguments = "/add-driver \"" + appdata + "Drivers\\usbtiny.inf\" /install /force";
					runDriverInstall(start, false);
					
					start.FileName = "InfDefaultInstall";
					start.Arguments = "\"" + appdata + "Drivers\\usbtiny.inf\"";
					runDriverInstall(start, false);
				}
				
				return;
			}
		}
        catch (ManagementException e)
        {
            Console.WriteLine("An error occurred while querying for WMI data: " + e.Message);
        }

		browser.StartInfo.FileName = "http://127.0.0.1:8080";	
		browser.Start();
	}

	private static void extractZIP(string zipPath, string extractPath)
	{
		//ZipFile.ExtractToDirectory(zipPath", extractPath);

		using (ZipArchive archive = ZipFile.OpenRead(zipPath))
		{
        	foreach (ZipArchiveEntry entry in archive.Entries)
        	{
        		string destinationPath = Path.GetFullPath(Path.Combine(extractPath, entry.FullName));
        		try {
            		if (destinationPath.EndsWith("\\")) {
            			Directory.CreateDirectory(destinationPath);
                	}else{
            			entry.ExtractToFile(destinationPath, true);
            		}
            		Console.WriteLine(destinationPath);
        		}catch{}
    		}
		}
		File.Delete(zipPath);
	}

	private static bool checkMD5(string filename, string sum)
	{
    	if (File.Exists(filename)) {
    		using (var md5 = MD5.Create())
			{
			    using (var stream = File.OpenRead(filename))
			    {
			    	string hash = BitConverter.ToString(md5.ComputeHash(stream)).Replace("-", "").ToLower();
			        if(hash == sum) {
			        	return true;
			        }
			        Console.WriteLine("File: " + filename);
			        Console.WriteLine("MD5: " + hash + " <-> " + sum);
			    }
			}
    	}

		return false;
	}

 	private static void extractEmbedResource(string name, string resource, string destination)
	{
	    if (File.Exists(destination + resource)) {
	    	File.Delete(destination + resource);
	    }
		Stream stream = Assembly.GetExecutingAssembly().GetManifestResourceStream(name + "." + resource);
		if(stream != null) {
            FileStream fileStream = new FileStream(destination + resource, FileMode.CreateNew);
            for (int i = 0; i < stream.Length; i++)
                fileStream.WriteByte((byte)stream.ReadByte());
            fileStream.Close();
    	}
	}

    private static bool AcceptAllCertifications(object sender, System.Security.Cryptography.X509Certificates.X509Certificate certification, System.Security.Cryptography.X509Certificates.X509Chain chain, System.Net.Security.SslPolicyErrors sslPolicyErrors)
	{
	    return true;
	}

	private static bool IsAdministrator()
	{
	    WindowsIdentity identity = WindowsIdentity.GetCurrent();
	    WindowsPrincipal principal = new WindowsPrincipal(identity);
	    return principal.IsInRole(WindowsBuiltInRole.Administrator);
	}

    private class AsyncHttpServer
    {
        private readonly HttpListener _listener;
        public static string _SESSION_USB = "";
        public static string _SESSION_BITRATE = "";
        public static string _SESSION_CHIP = "";

        public AsyncHttpServer(int portNumber)
        {
            _listener = new HttpListener();
            _listener.Prefixes.Add(string.Format("http://127.0.0.1:{0}/", portNumber));
        }

        public async Task Start()
        {
            _listener.Start();

            while (true)
            {
                HttpListenerContext ctx = await _listener.GetContextAsync();

                string filename = ctx.Request.Url.LocalPath;
                if (filename.EndsWith("/")) filename = "/index.html";
                string sMIME = getContentType(filename);
                filename = filename.Replace("/","\\");

                //Console.WriteLine(ctx.Request.Url.ToString());
                Console.WriteLine(appdata + "Web" + filename);

                ctx.Response.Headers.Add("Content-Type: " + sMIME);

            	if (filename.EndsWith("\\usb.php")) {
            		
            		string query = ctx.Request.Url.Query.Substring(1);
            		string[] items = { query };
            		
            		if(query.Contains("&")) {
            			items = query.Split('&');
            		}
            		using (StreamWriter sw = new StreamWriter(ctx.Response.OutputStream))
	                {
	                	string command = appdata + "Web\\avrdude.exe";

	            		foreach (string item in items) {
	            			//Console.WriteLine(item);
					    	string[] param = item.Split('=');
					    	/*
					    	for (int i = 0; i < param.Length; i+=2) {
					    		Console.WriteLine(param[i] + "=" + param[i+1]);
					    	}
					    	*/
					    	if(param[0] == "runas") {
								if (IsAdministrator() == false) {
									Console.WriteLine("...Restart program and run as Admin");
								    var exeName = System.Diagnostics.Process.GetCurrentProcess().MainModule.FileName;
								    ProcessStartInfo startInfo = new ProcessStartInfo(exeName);
								    startInfo.Verb = "runas";
								    System.Diagnostics.Process.Start(startInfo);
								    Environment.Exit(0);
								}
				    		}else if(param[0] == "connect") {
				    			await sw.WriteAsync(Connect("usbtiny",0));
				    		}else if(param[0] == "log") {
				    			using (StreamReader reader = new StreamReader(appdata + "Web\\log.txt")) {
        							await sw.WriteAsync(reader.ReadToEnd());
						        }
						    }else if(param[0] == "reset") {
						    	string args = "-c " + _SESSION_USB + " -p t13 -Ulfuse:v:0x00:m";
						    	string output = Run(command, args);
							    await sw.WriteAsync(command + args + "\n" + output);
						   	}else if(param[0] == "eeprom") {

						   		string eeprom_file = "attiny.eeprom";
						   		string args = "";

						   		if(param[1] == "erase") {
						   			ctx.Response.Headers.Add("Refresh:3; url=index.html");

						   			args = " -c " + _SESSION_USB + " -p " + _SESSION_CHIP.ToLower() + " -V -U eeprom:w:" + temp + eeprom_file + ":r";
						   		}else if(param[1] == "flash") {
						   			args = " -c " + _SESSION_USB + " -p " + _SESSION_CHIP.ToLower() + " -V -U flash:w:" + appdata + "Web\\firmware\\" + _SESSION_CHIP.ToLower() + ".hex:i";
						   		}else{
						   			args = " -c " + _SESSION_USB + " -p " + _SESSION_CHIP.ToLower() + " -U eeprom:r:" + temp + eeprom_file + ":r";
						   		}
						    	string output = Run(command, args);

						    	if(File.Exists(temp + eeprom_file)) {

						    		using (BinaryReader reader = new BinaryReader(File.Open(temp + eeprom_file, FileMode.Open))) {
							       		
							       		await sw.WriteAsync(temp + eeprom_file + "\n");

								        if(param[1] == "write") {

								        	int length = (int)reader.BaseStream.Length;
							            	byte[] binary = reader.ReadBytes(length);

							            	args = args.Replace("-U eeprom:r:", _SESSION_BITRATE + "-U eeprom:w:");
							            	//output = Run(command, args);

							            	await sw.WriteAsync(command + args + "\n");
							            	await sw.WriteAsync(output);
							        	}else{
							        		while (reader.PeekChar() != -1)
    										{
    											int b = reader.ReadInt16();
    											byte lo = (byte)b;
    											byte hi = (byte)(b >> 8);

							        			await sw.WriteAsync(lo + "\n");
							        			await sw.WriteAsync(hi + "\n");
							        		}
							        	}
							        }
							        //await sw.WriteAsync(output);
						    	}else{
						    		await sw.WriteAsync(output);
						    	}
				    		}else{
				    			await sw.WriteAsync("");
				    		}
				    		await sw.FlushAsync();
					    }
	                }
            	}else{
            		using (BinaryReader reader = new BinaryReader(File.Open(appdata + "Web" + filename, FileMode.Open)))
			        {
			        	int length = (int)reader.BaseStream.Length;
			            byte[] contents = reader.ReadBytes(length);

			            using (var sw = new BinaryWriter(ctx.Response.OutputStream))
		                {
		                    sw.Write(contents);
		                    sw.Flush();
		                }
			        }
            	}
            }
        }

        public string Run(string command, string args)
        {
        	string output = "";
        	int timeout = 3;
        	string[] retry = { "timed out", "output error", "libusb: debug", "initialization failed", "Broken pipe" };
			
			while (timeout > 0)
			{
				Process process = new Process();
				process.StartInfo.UseShellExecute = false;
				process.StartInfo.RedirectStandardOutput = true;
				process.StartInfo.RedirectStandardError = true;
				process.StartInfo.FileName = command;
				process.StartInfo.Arguments = args;
				process.Start();
				var outputList = new List<string>();
				while (process.StandardOutput.Peek() > -1) {
				    outputList.Add(process.StandardOutput.ReadLine());
				}
				while (process.StandardError.Peek() > -1) {
				    outputList.Add(process.StandardError.ReadLine());
				}
				process.WaitForExit(4000);
				output = string.Join("", outputList.ToArray());

				bool run = true;
	    		foreach (string item in retry)
				{
					if(output.IndexOf(item) != -1) {
	    				run = false;
	    				break;
	    			}
				}
	    		if (run == true) {
	    			return output;
	    		}
	    		//sleep(1);
	    		timeout--;
	    	}

        	return output;
        }

        public string Connect(string programmer, int timeout)
        {
        	string output = "";
        	
			Process process = new Process();
			process.StartInfo.UseShellExecute = false;
			process.StartInfo.RedirectStandardOutput = true;
			process.StartInfo.RedirectStandardError = true;
			process.StartInfo.WorkingDirectory = appdata + "Web";
			process.StartInfo.FileName =  appdata + "Web\\avrdude.exe";
			process.StartInfo.Arguments = "-c " + programmer + " -p t13 -n";
			process.Start();
			var outputList = new List<string>();
			while (process.StandardOutput.Peek() > -1) {
			    outputList.Add(process.StandardOutput.ReadLine());
			}
			while (process.StandardError.Peek() > -1) {
			    outputList.Add(process.StandardError.ReadLine());
			}
			process.WaitForExit(4000);
			output = string.Join("", outputList.ToArray());
			
			//Console.WriteLine(output);

			_SESSION_USB = programmer;
			_SESSION_BITRATE = "-B250 ";
			_SESSION_CHIP = "";

			if(output.IndexOf("0x1e9007") != -1) {
				_SESSION_CHIP = "ATtiny13";
			}else if (output.IndexOf("0x1e9206") != -1) {
            	_SESSION_CHIP = "ATtiny45";
        	}else if (output.IndexOf("0x1e930b") != -1) {
            	_SESSION_CHIP = "ATtiny85";
	        }else if (output.ToLower().IndexOf("initialization failed") != -1) {
	            if(output.Length > 0 ) {
	                return "fix";
	            }
	        }else{
	        	if(timeout < 4) {
	        		if (output.ToLower().IndexOf("error:") != -1) {
	        			if(programmer == "usbtiny") { //try another programmer
	                		programmer = "usbasp";
		                }else{
		                	programmer = "usbtiny";
		                }
		                timeout++;
		                output = Connect(programmer,timeout);
	        		}
	        	}else{
	        		return "sck";
	        	}
	        	return output;
	        }

        	return _SESSION_CHIP;
        }

        public string getContentType(string filename)
        {
        	if (filename.EndsWith(".php")) return "text/html";
			else if (filename.EndsWith(".htm")) return "text/html";
			else if (filename.EndsWith(".html")) return "text/html";
			else if (filename.EndsWith(".css")) return "text/css";
			else if (filename.EndsWith(".js")) return "application/javascript";
			else if (filename.EndsWith(".json")) return "application/json";
			else if (filename.EndsWith(".png")) return "image/png";
			else if (filename.EndsWith(".jpg")) return "image/jpeg";
			else if (filename.EndsWith(".jpeg")) return "image/jpeg";
			else if (filename.EndsWith(".ico")) return "image/x-icon";
			else if (filename.EndsWith(".svg")) return "image/svg+xml";
			else if (filename.EndsWith(".ttf")) return "font/ttf";
			else if (filename.EndsWith(".woff")) return "font/woff";
			else if (filename.EndsWith(".woff2")) return "font/woff2";
			return "text/plain";
        }

        public async Task Stop()
        {
            await Console.Out.WriteLineAsync("Stopping server...");

            if (_listener.IsListening)
            {
                _listener.Stop();
                _listener.Close();
            }
        }
    }
}