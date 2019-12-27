using System;
using System.IO;
using System.Net;
using System.Diagnostics;
//using System.Management;
using System.Management.Automation;
using System.Management.Automation.Runspaces;
using System.Reflection;
using Microsoft.Win32;

[assembly:AssemblyVersion("1.0.0.0")]

public class ATtinyPlant
{
	static string path = System.IO.Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location) + "\\";

    public static void Main(string[] args)
    {
    	ServicePointManager.ServerCertificateValidationCallback = new System.Net.Security.RemoteCertificateValidationCallback(AcceptAllCertifications);
		ServicePointManager.SecurityProtocol |= SecurityProtocolType.Tls12;

		Console.ForegroundColor = ConsoleColor.Green;

		extractEmbedResource("avrdude.exe");
		extractEmbedResource("avrdude.conf");

		if (!File.Exists(path + @"php\php.exe")) {

			string phpFile = "php-7.4.0-Win32-vc15-x64.zip";
			extractEmbedResource(phpFile);

			if (File.Exists(path + phpFile)) {
				phpFile = path + phpFile;
			}else{
				if (!File.Exists(Environment.GetEnvironmentVariable("USERPROFILE") + @"\Downloads\" + phpFile)) {
			   		Console.WriteLine("...Downloading PHP");

			   		using (WebClient webClient = new WebClient()) {
				   		try {
				   			webClient.Headers.Add("User-Agent", "Mozilla/4.0 (compatible; MSIE 8.0)");
							webClient.DownloadFile("https://windows.php.net/downloads/releases/archives/" + phpFile , Environment.GetEnvironmentVariable("USERPROFILE") + @"\Downloads\" + phpFile);
				   		} catch (Exception ex) {
		                    Console.WriteLine(ex.ToString());
		                }
		            }
			   	}
			   	phpFile = Environment.GetEnvironmentVariable("USERPROFILE") + @"\Downloads\" + phpFile;
			}

		   	Console.WriteLine("...Installing PHP");

		  	System.IO.Compression.ZipFile.ExtractToDirectory(phpFile, path + "php");

			File.Move(path + @"php\php.ini-production", path + @"php\php.ini");
		}

		var vc2019 = Registry.LocalMachine.OpenSubKey(@"SOFTWARE\Classes\Installer\Dependencies\VC,redist.x64,amd64,14.24,bundle\Dependents\{282975d8-55fe-4991-bbbb-06a72581ce58}");

		if (vc2019 == null)
		{
			string vcFile = "VC_redist.x64.exe";
			extractEmbedResource(vcFile);

			if (File.Exists(path + vcFile)) {
				vcFile = path + vcFile;
			}else{
				if (!File.Exists(Environment.GetEnvironmentVariable("USERPROFILE") + @"\Downloads\" + vcFile)) {
			   	
					Console.WriteLine("...Downloading C++ Redistributable for Visual Studio 2019");

					using (WebClient webClient = new WebClient()) {
				   		try {
				   			webClient.Headers.Add("User-Agent", "Mozilla/4.0 (compatible; MSIE 8.0)");
							webClient.DownloadFile("https://aka.ms/vs/16/release/" + vcFile , Environment.GetEnvironmentVariable("USERPROFILE") + @"\Downloads\" + vcFile);
				   		} catch (Exception ex) {
		                    Console.WriteLine(ex.ToString());
		                }
		            }
				}
				vcFile = Environment.GetEnvironmentVariable("USERPROFILE") + @"\Downloads\" + vcFile;
			}

			ProcessStartInfo start = new ProcessStartInfo();
			start.FileName = vcFile;
			using (Process proc = Process.Start(start))
			{
			    proc.WaitForExit();
			}
		}

		checkDriver();

		Process php = new Process();
		php.StartInfo.FileName = path + @"php\php.exe";
		php.StartInfo.Arguments = "-S 127.0.0.1:8080 -t \"" + path + "Web\"";
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
		powerShell.AddScript("Get-WmiObject Win32_PNPEntity | Where-Object{ $_.Status -match \"Error\" -and ($_.Name -match \"STLink\" -or $_.Name -match \"USBasp\") }");
		var results = powerShell.Invoke();

		if(results.Count > 0)
		{
			Console.WriteLine("...Correcting Drivers");

			string zadig = "zadig-2.4.exe";
			extractEmbedResource(zadig);

			if (File.Exists(path + zadig)) {
				zadig = path + zadig;
			}else{
				if (!File.Exists(Environment.GetEnvironmentVariable("USERPROFILE") + @"\Downloads\" + zadig)) {
			   	
					Console.WriteLine("...Downloading Utility");

					using (WebClient webClient = new WebClient()) {
				   		try {
				   			webClient.Headers.Add("User-Agent", "Mozilla/4.0 (compatible; MSIE 8.0)");
							webClient.DownloadFile("https://zadig.akeo.ie/downloads/" + zadig , Environment.GetEnvironmentVariable("USERPROFILE") + @"\Downloads\" + zadig);
				   		} catch (Exception ex) {
		                    Console.WriteLine(ex.ToString());
		                }
		            }
				}
				zadig = Environment.GetEnvironmentVariable("USERPROFILE") + @"\Downloads\" + zadig;
			}

			ProcessStartInfo start = new ProcessStartInfo();
			start.FileName = zadig;
			using (Process proc = Process.Start(start))
			{
			    proc.WaitForExit();
			}
		}
		/*
		foreach (dynamic result in results)
		{
		    Console.WriteLine(result.ToString());
		}
		*/
		runspace.Close();
	}

 	public static void extractEmbedResource(string res)
	{
	    if (!File.Exists(path + res)) {
			Stream stream = Assembly.GetExecutingAssembly().GetManifestResourceStream("EmbedExe." + res);
			if(stream != null) {
	            FileStream fileStream = new FileStream(path + res, FileMode.CreateNew);
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