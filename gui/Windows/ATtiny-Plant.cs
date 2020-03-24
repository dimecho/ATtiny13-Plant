using System;
using System.IO;
using System.Net;
using System.Diagnostics;
using System.IO.Compression;
using System.Security.Cryptography;
using System.Security.Cryptography.X509Certificates;
using System.Management;
using System.Reflection;
using Microsoft.Win32;

[assembly:AssemblyVersion("1.0.0.0")]

public class ATtinyPlant
{
	static string path = System.IO.Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location) + "\\";
	static string temp = Environment.GetEnvironmentVariable("TEMP") + "\\";
	static string profile = Environment.GetEnvironmentVariable("USERPROFILE") + "\\";
	static string appdata = Environment.GetFolderPath(Environment.SpecialFolder.CommonApplicationData)+ "\\ATtiny-Plant\\";

	[STAThread]
    public static void Main(string[] args)
    {
    	ServicePointManager.ServerCertificateValidationCallback = new System.Net.Security.RemoteCertificateValidationCallback(AcceptAllCertifications);
		ServicePointManager.SecurityProtocol |= SecurityProtocolType.Tls12;

		Console.ForegroundColor = ConsoleColor.Green;

		if(!Directory.Exists(appdata) || checkVersion() == false)
		{
			Directory.CreateDirectory(appdata);

			Console.WriteLine("...Installing Web-Interface");
			extractEmbedResource("APP", "Web.zip", temp);
			ZipFile.ExtractToDirectory(temp + "Web.zip", appdata);
			File.Delete(temp + "Web.zip");

			extractEmbedResource("AVR", "avrdude.conf", appdata + @"Web\");
			extractEmbedResource("AVR", "avrdude.exe", appdata + @"Web\");
			extractEmbedResource("AVR", "libusb0.dll", appdata + @"Web\");

			extractEmbedResource("APP", "Drivers.zip", temp);
			ZipFile.ExtractToDirectory(temp + "Drivers.zip", appdata);
			File.Delete(temp + "Drivers.zip");
		}

		if (!File.Exists(appdata + @"php\php.exe")) {

			string phpFile = "php-7.4.3-Win32-vc15-x64.zip";
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

		var vc2019 = Registry.LocalMachine.OpenSubKey(@"SOFTWARE\Classes\Installer\Dependencies\VC,redist.x64,amd64,14.25,bundle\Dependents\{6913e92a-b64e-41c9-a5e6-cef39207fe89}");

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

		checkDriver("VID_1781&PID_0C9F");
		//checkDriver("VID_16C0&PID_05DC");

		Process php = new Process();
		php.StartInfo.FileName = appdata + @"php\php.exe";
		php.StartInfo.Arguments = "-S 127.0.0.1:8080 -t \"" + appdata + "Web\"";
		php.Start();
		
		Process browser = new Process();
		browser.StartInfo.FileName = "http://127.0.0.1:8080";
		browser.Start();
		
		//Console.Read();
    }

	public static void checkDriver(string id)
	{
		try
        {
            ManagementObjectSearcher searcher = new ManagementObjectSearcher("root\\CIMV2", "SELECT * FROM Win32_PNPEntity WHERE Status LIKE '%Error%' AND DeviceID LIKE '%" + id + "%'"); 

			if(searcher.Get().Count > 0)
			{
				Console.WriteLine("...Validating Certificate");

				ProcessStartInfo start = new ProcessStartInfo();
			    X509Certificate2 certificateToValidate = new X509Certificate2(X509Certificate2.CreateFromCertFile(appdata + "Drivers\\usbtiny.cer"));
				bool valid = certificateToValidate.Verify();
				
				//https://github.com/adafruit/Adafruit_Windows_Drivers
				
				if (!valid && certificateToValidate.Subject.IndexOf("Adafruit") == -1)
				{
					Console.WriteLine("Certificate Not Valid");
					
					start.FileName = "powershell.exe";
					start.Arguments = "-ExecutionPolicy Bypass -Command \".\\" + appdata + "Drivers\\self-sign.ps1\"";
					using (Process proc = Process.Start(start)) {
					    proc.WaitForExit();
					}
					start.FileName = "powershell.exe";
					start.Arguments = "-ExecutionPolicy Bypass -Command \".\\" + appdata + "Drivers\\install.ps1\"";
					using (Process proc = Process.Start(start)) {
					    proc.WaitForExit();
					}
					
				}else{
					Console.WriteLine("...Installing Driver");

					start.FileName = "pnputil";
					start.Arguments = "-a \"" + appdata + "Drivers\\usbtiny.inf\"";
					using (Process proc = Process.Start(start)) {
					    proc.WaitForExit();
					}
					start.FileName = "InfDefaultInstall";
					start.Arguments = "\"" + appdata + "Drivers\\usbtiny.inf\"";
					using (Process proc = Process.Start(start)) {
					    proc.WaitForExit();
					}
				}
			}
		}
        catch (ManagementException e)
        {
            Console.WriteLine("An error occurred while querying for WMI data: " + e.Message);
        }
	}

	public static void checkDriverFilter(string id)
	{
		RegistryKey key = Registry.LocalMachine.OpenSubKey(@"SYSTEM\CurrentControlSet\Enum\USB\" + id);

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
		installDriverFilter(id);
	}

    public static void installDriverFilter(string id)
	{
		try
        {
            ManagementObjectSearcher searcher = new ManagementObjectSearcher("root\\CIMV2", "SELECT * FROM Win32_PNPEntity WHERE DeviceID LIKE '%" + id + "%'"); 
            
            foreach (ManagementObject queryObj in searcher.Get())
            {
            	Console.WriteLine("...Installing Filter");

            	ProcessStartInfo start = new ProcessStartInfo();
				start.FileName = appdata + @"Drivers\amd64\install-filter.exe";
				start.Arguments = "install \"--device=" + queryObj["HardwareID"] + "\"";
				start.Verb = "runas";
				using (Process proc = Process.Start(start))
				{
				    proc.WaitForExit();
				}
			    break;
            }
		}
        catch (ManagementException e)
        {
            Console.WriteLine("An error occurred while querying for WMI data: " + e.Message);
        }
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