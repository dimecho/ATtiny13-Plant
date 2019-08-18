using System;
using System.IO;
using System.Net;
using System.Diagnostics;
using Microsoft.Win32;

public class ATtinyPlant
{
	static string path = System.IO.Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location) + "\\";

    public static void Main(string[] args)
    {
    	ServicePointManager.ServerCertificateValidationCallback = new System.Net.Security.RemoteCertificateValidationCallback(AcceptAllCertifications);
		ServicePointManager.SecurityProtocol |= SecurityProtocolType.Tls12;

		Console.ForegroundColor = ConsoleColor.Green;

		if (!File.Exists(path + "fboot.exe")) {
			Stream stream = Assembly.GetExecutingAssembly().GetManifestResourceStream("EmbedExe.fboot.exe");
            FileStream fileStream = new FileStream(path + "fboot.exe", FileMode.CreateNew);
            for (int i = 0; i < stream.Length; i++)
                fileStream.WriteByte((byte)stream.ReadByte());
            fileStream.Close();
        }
		
		if (!File.Exists(path + @"php\php.exe")) {

			string phpFile = "php-7.3.7-Win32-VC15-x64.zip";

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

		string vcFile = "vc_redist.x64.exe";
		var vc2015 = Registry.LocalMachine.OpenSubKey(@"SOFTWARE\Classes\Installer\Dependencies\{d992c12e-cab2-426f-bde3-fb8c53950b0d}");
		var vc2017 = Registry.LocalMachine.OpenSubKey(@"SOFTWARE\Classes\Installer\Dependencies\,,amd64,14.0,bundle\Dependents\{e2ee15e2-a480-4bc5-bfb7-e9803d1d9823}");

		if (vc2015 == null && vc2017 == null)
		{
			if (File.Exists(path + vcFile)) {
				vcFile = path + vcFile;
			}else{
				if (!File.Exists(Environment.GetEnvironmentVariable("USERPROFILE") + @"\Downloads\" + vcFile)) {
			   	
					Console.WriteLine("...Downloading C++ Redistributable for Visual Studio 2015");

					using (WebClient webClient = new WebClient()) {
				   		try {
				   			webClient.Headers.Add("User-Agent", "Mozilla/4.0 (compatible; MSIE 8.0)");
							webClient.DownloadFile("https://download.microsoft.com/download/6/A/A/6AA4EDFF-645B-48C5-81CC-ED5963AEAD48/" + vcFile , Environment.GetEnvironmentVariable("USERPROFILE") + @"\Downloads\" + vcFile);
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

		Process php = new Process();
		php.StartInfo.FileName = path + @"php\php.exe";
		php.StartInfo.Arguments = "-S 0.0.0.0:8080 -t \"" + path + "Web\"";
		php.Start();
		
		Process browser = new Process();
		browser.StartInfo.FileName = "http://127.0.0.1:8080";
		browser.Start();
		
		//Console.Read();
    }

    public static bool AcceptAllCertifications(object sender, System.Security.Cryptography.X509Certificates.X509Certificate certification, System.Security.Cryptography.X509Certificates.X509Chain chain, System.Net.Security.SslPolicyErrors sslPolicyErrors)
	{
	    return true;
	}
}