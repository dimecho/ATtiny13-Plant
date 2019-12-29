using System;
using System.IO;
using System.Net;
using System.Diagnostics;
using System.IO.Compression;
using System.Management.Automation;
using System.Management.Automation.Runspaces;
using System.Reflection;
using Microsoft.Win32;

[assembly:AssemblyVersion("1.0.0.0")]

public class ATtinyPlant
{
	static string path = System.IO.Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location) + "\\";
	static string temp = Environment.GetEnvironmentVariable("TEMP") + "\\";
	static string profile = Environment.GetEnvironmentVariable("USERPROFILE") + "\\";
	static string appdata = Environment.GetFolderPath(Environment.SpecialFolder.CommonApplicationData)+ "\\ATtiny-Plant\\";

    public static void Main(string[] args)
    {
    	ServicePointManager.ServerCertificateValidationCallback = new System.Net.Security.RemoteCertificateValidationCallback(AcceptAllCertifications);
		ServicePointManager.SecurityProtocol |= SecurityProtocolType.Tls12;

		Console.ForegroundColor = ConsoleColor.Green;

		if(!Directory.Exists(appdata) || checkVersion() == false)
		{
			Directory.CreateDirectory(appdata);

			Console.WriteLine("...Installing Web-Interface");
			extractEmbedResource("APP", "ATtiny-Plant-Web.zip", temp);
			ZipFile.ExtractToDirectory(temp + "ATtiny-Plant-Web.zip", appdata);
			File.Delete(temp + "ATtiny-Plant-Web.zip");
		}

		extractEmbedResource("AVR", "avrdude.conf", appdata + @"Web\");
		extractEmbedResource("AVR", "avrdude.exe", appdata + @"Web\");
		extractEmbedResource("AVR", "libusb0.dll", appdata + @"Web\");
		
		if (!File.Exists(appdata + @"php\php.exe")) {

			string phpFile = "php-7.4.0-Win32-vc15-x64.zip";
			extractEmbedResource("PHP",phpFile,temp);

			if (File.Exists(temp + phpFile)) {
				phpFile = temp + phpFile;
			}else{
				if (!File.Exists(profile+ @"Downloads\" + phpFile)) {
			   		Console.WriteLine("...Downloading PHP");

			   		using (WebClient webClient = new WebClient()) {
				   		try {
				   			webClient.Headers.Add("User-Agent", "Mozilla/4.0 (compatible; MSIE 8.0)");
							webClient.DownloadFile("https://windows.php.net/downloads/releases/archives/" + phpFile, profile + @"Downloads\" + phpFile);
				   		} catch (Exception ex) {
		                    Console.WriteLine(ex.ToString());
		                }
		            }
			   	}
			   	phpFile = profile + @"Downloads\" + phpFile;
			}

		   	Console.WriteLine("...Installing PHP");

		  	ZipFile.ExtractToDirectory(phpFile, appdata + "php");
			File.Move(appdata + @"php\php.ini-production", appdata + @"php\php.ini");
		}

		var vc2019 = Registry.LocalMachine.OpenSubKey(@"SOFTWARE\Classes\Installer\Dependencies\VC,redist.x64,amd64,14.24,bundle\Dependents\{282975d8-55fe-4991-bbbb-06a72581ce58}");

		if (vc2019 == null)
		{
			string vcFile = "VC_redist.x64.exe";
			extractEmbedResource("VC",vcFile,temp);

			if (File.Exists(temp + vcFile)) {
				vcFile = temp + vcFile;
			}else{
				if (!File.Exists(profile + @"Downloads\" + vcFile)) {
			   	
					Console.WriteLine("...Downloading C++ Redistributable for Visual Studio 2019");

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
		}

		checkDriver();
		checkDriverFilter();

		Process php = new Process();
		php.StartInfo.FileName = appdata + @"php\php.exe";
		php.StartInfo.Arguments = "-S 127.0.0.1:8080 -t \"" + appdata + "Web\"";
		php.Start();
		
		Process browser = new Process();
		browser.StartInfo.FileName = "http://127.0.0.1:8080";
		browser.Start();
		
		//Console.Read();
    }

	public static void checkDriver()
	{
		Runspace runspace = RunspaceFactory.CreateRunspace();
		runspace.Open();

		PowerShell powerShell = PowerShell.Create();
		powerShell.Runspace = runspace;
		powerShell.AddScript("Get-WmiObject Win32_PNPEntity | Where { $_.Status -match \"Error\" -and $_.HardwareID -like \"*VID_16C0&PID_05DC*\"}");
		var results = powerShell.Invoke();

		if(results.Count > 0)
		{
			string zadig = "zadig-2.4.exe";
			extractEmbedResource("Drivers", zadig, temp);

			if (File.Exists(temp + zadig)) {
				zadig = temp + zadig;
			}else{
				if (!File.Exists(profile + @"Downloads\" + zadig)) {
			   	
					Console.WriteLine("...Downloading Utility");

					using (WebClient webClient = new WebClient()) {
				   		try {
				   			webClient.Headers.Add("User-Agent", "Mozilla/4.0 (compatible; MSIE 8.0)");
							webClient.DownloadFile("https://zadig.akeo.ie/downloads/" + zadig, profile + @"Downloads\" + zadig);
				   		} catch (Exception ex) {
		                    Console.WriteLine(ex.ToString());
		                }
		            }
				}
				zadig = profile + @"Downloads\" + zadig;
			}

			ProcessStartInfo start = new ProcessStartInfo();
			start.FileName = zadig;
			using (Process proc = Process.Start(start))
			{
			    proc.WaitForExit();
			}
		}
		runspace.Close();
	}

	public static void checkDriverFilter()
	{
		RegistryKey key = Registry.LocalMachine.OpenSubKey(@"SYSTEM\CurrentControlSet\Enum\USB\VID_16C0&PID_05DC");

		if (key != null) {
			foreach (var v in key.GetSubKeyNames()) {
	            //Console.WriteLine(v);
	            RegistryKey driverKey = key.OpenSubKey(v);

	            if (driverKey != null) {
	            	var getValue = driverKey.GetValue("UpperFilters",null); //REG_MULTI_SZ

	            	if(getValue != null) {
		            	foreach (string filter in (string[])getValue) {
		            		//Console.WriteLine(filter);
		            		if(filter == "libusb0"){
		            			return;
		            		}
						}
					}
	            }
	        }
		}
		installDriverFilter();
	}

    public static void installDriverFilter()
	{
		extractEmbedResource("Drivers", "install-filter.exe", temp);
		extractEmbedResource("Drivers", "libusb0.dll", temp);
		extractEmbedResource("Drivers", "libusb0.sys", temp);

		Runspace runspace = RunspaceFactory.CreateRunspace();
		runspace.Open();

		PowerShell powerShell = PowerShell.Create();
		powerShell.Runspace = runspace;
		powerShell.AddScript("Get-WmiObject Win32_PNPEntity | Where {$_.HardwareID -like \"*VID_16C0&PID_05DC*\"} | Select -ExpandProperty HardwareID");
		var results = powerShell.Invoke();

		if(results.Count > 0)
		{
			Console.WriteLine("...Installing Filter");
					
			foreach (dynamic result in results)
			{
			    Console.WriteLine(result.ToString());

			    ProcessStartInfo start = new ProcessStartInfo();
				start.FileName = temp + "install-filter.exe";
				start.Arguments = "install \"--device=" + result.ToString() + "\"";
				start.Verb = "runas";
				using (Process proc = Process.Start(start))
				{
				    proc.WaitForExit();
				}
			    break;
			}
		}
		runspace.Close();
	}

	public static bool checkVersion()
	{
		try
        {
			System.Reflection.Assembly assembly = System.Reflection.Assembly.GetExecutingAssembly();
            Version version = assembly.GetName().Version;

            StreamReader file = new System.IO.StreamReader(appdata + @"Web\firmware\version.txt");  
			string line = file.ReadLine();
			file.Close();

            if (line != version.Major + "." + version.Minor) {
            	Directory.Delete(appdata + "Web", true);
            	Directory.Delete(appdata + "php", true);
            	return false;
            }
        }
        catch
        {
        	return false;
        }

		return true;
	}

 	public static void extractEmbedResource(string name, string resource, string destination)
	{
	    if (!File.Exists(destination + resource)) {
			Stream stream = Assembly.GetExecutingAssembly().GetManifestResourceStream(name + "." + resource);
			if(stream != null) {
	            FileStream fileStream = new FileStream(destination + resource, FileMode.CreateNew);
	            for (int i = 0; i < stream.Length; i++)
	                fileStream.WriteByte((byte)stream.ReadByte());
	            fileStream.Close();
        	}
        }
	}

    public static bool AcceptAllCertifications(object sender, System.Security.Cryptography.X509Certificates.X509Certificate certification, System.Security.Cryptography.X509Certificates.X509Chain chain, System.Net.Security.SslPolicyErrors sslPolicyErrors)
	{
	    return true;
	}
}