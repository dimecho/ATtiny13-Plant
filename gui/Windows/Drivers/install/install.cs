using System;
using System.IO;
using System.Diagnostics;
using System.Security.Principal;
using System.Security.Cryptography;
using System.Security.Cryptography.X509Certificates;
using Microsoft.CertificateServices.Certenroll.Interop;
//using CERTENROLLLib;

public class SignatureUtilities
{
    public static void Main(string[] args)
    {
    	string path = System.IO.Path.GetDirectoryName(System.Reflection.Assembly.GetExecutingAssembly().Location) + "\\";
	
		Console.WriteLine("...Self-Signing Certificate");

		X509Certificate2 certificateToValidate = new X509Certificate2();
		X509Store store = new X509Store("MY",StoreLocation.CurrentUser);
		store.Open(OpenFlags.ReadOnly | OpenFlags.OpenExistingOnly);
		X509Certificate2Collection collection = store.Certificates.Find(X509FindType.FindBySubjectDistinguishedName, "CN=ATtiny13 Plant", false);
		store.Close();

    	if(collection.Count == 0)
    	{
    		Console.WriteLine("...Generating New Certificate");

    		var dn = new CX500DistinguishedName();
			dn.Encode("CN=ATtiny13 Plant", X500NameFlags.XCN_CERT_NAME_STR_NONE);

    		var privateKey = new CX509PrivateKey();
		    privateKey.ProviderName = "Microsoft Base Cryptographic Provider v1.0";
		    privateKey.MachineContext = false; //true;
		    privateKey.Length = 2048;
		    privateKey.KeySpec = X509KeySpec.XCN_AT_SIGNATURE; // use is not limited
		    privateKey.ExportPolicy = X509PrivateKeyExportFlags.XCN_NCRYPT_ALLOW_PLAINTEXT_EXPORT_FLAG;
		    privateKey.Create();

		    var hashobj = new CObjectId();
		    hashobj.InitializeFromAlgorithmName(ObjectIdGroupId.XCN_CRYPT_HASH_ALG_OID_GROUP_ID, ObjectIdPublicKeyFlags.XCN_CRYPT_OID_INFO_PUBKEY_ANY, AlgorithmFlags.AlgorithmFlagsNone, "SHA256");

		    var oid = new CObjectId();
		    oid.InitializeFromValue("1.3.6.1.5.5.7.3.3"); //eku for code signing
		    var oidlist = new CObjectIds();
		    oidlist.Add(oid);
		    var eku = new CX509ExtensionEnhancedKeyUsage();
		    eku.InitializeEncode(oidlist);

		    var cert = new CX509CertificateRequestCertificate();
		    cert.InitializeFromPrivateKey(X509CertificateEnrollmentContext.ContextUser, privateKey, "");
		    cert.Subject = dn;
		    cert.Issuer = dn;
		    cert.NotBefore = DateTime.Now;
		    cert.NotAfter = cert.NotBefore.AddYears(5);
		    cert.X509Extensions.Add((CX509Extension)eku);
		    cert.HashAlgorithm = hashobj;
		    cert.Encode();

		    var enroll = new CX509Enrollment();
		    enroll.InitializeFromRequest(cert);
		    enroll.CertificateFriendlyName = "ATtiny13 Plant";
		    string csr = enroll.CreateRequest(); // Output the request in base64 and install it back as the response
		    enroll.InstallResponse(InstallResponseRestrictionFlags.AllowUntrustedCertificate, csr, EncodingType.XCN_CRYPT_STRING_BASE64, "");
		    
		    var base64encoded = enroll.CreatePFX("", PFXExportOptions.PFXExportChainWithRoot);
		    Console.WriteLine(base64encoded);

		    certificateToValidate = new System.Security.Cryptography.X509Certificates.X509Certificate2(System.Convert.FromBase64String(base64encoded), "", System.Security.Cryptography.X509Certificates.X509KeyStorageFlags.Exportable);
    	
    	}else{
    		foreach (X509Certificate2 x509 in collection)
	        {
	        	certificateToValidate = x509;
	        	break;
	        }
	        //store.AddRange(collection);
    	}

		if (IsAdministrator() == false)
		{
			Console.WriteLine("...Restart program and run as Admin");

		    var exeName = System.Diagnostics.Process.GetCurrentProcess().MainModule.FileName;
		    ProcessStartInfo startInfo = new ProcessStartInfo(exeName);
		    startInfo.Verb = "runas";
		    System.Diagnostics.Process.Start(startInfo);
		    Environment.Exit(0);
		    //Application.Current.Shutdown();
		}

		Console.WriteLine("...Installing Certificate");

		store = new X509Store(StoreName.Root, StoreLocation.LocalMachine);
		store.Open(OpenFlags.ReadWrite);
		store.Add(certificateToValidate);
		store.Close();

		store = new X509Store(StoreName.TrustedPublisher, StoreLocation.LocalMachine);
		store.Open(OpenFlags.ReadWrite);
		store.Add(certificateToValidate);
		store.Close();

		byte[] certBytes = certificateToValidate.Export(X509ContentType.Cert, ""); //X509ContentType.Authenticode
		System.IO.File.WriteAllBytes(path + "usbtiny.cer", certBytes);

		Console.WriteLine("...Driver Signing and Time Stamping");

		ProcessStartInfo start = new ProcessStartInfo();
		start.FileName = "self-sign.exe";
		using (Process proc = Process.Start(start)) {
		    proc.WaitForExit();
		}
		
		Console.WriteLine("...Installing Driver");

		start = new ProcessStartInfo();
		start.FileName = "pnputil";
		start.Arguments = "-a \"" + path  + "usbtiny.inf\"";
		using (Process proc = Process.Start(start)) {
		    proc.WaitForExit();
		}
		start.FileName = "InfDefaultInstall";
		start.Arguments = "\"" + path + "usbtiny.inf\"";
		using (Process proc = Process.Start(start)) {
		    proc.WaitForExit();
		}
	}

    private static bool IsAdministrator()
	{
	    WindowsIdentity identity = WindowsIdentity.GetCurrent();
	    WindowsPrincipal principal = new WindowsPrincipal(identity);
	    return principal.IsInRole(WindowsBuiltInRole.Administrator);
	}
}