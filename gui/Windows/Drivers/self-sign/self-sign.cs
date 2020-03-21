using System;
using System.IO;
using System.Net.Http;
using System.Net.Http.Headers;
using System.Threading.Tasks;
using System.Security.Cryptography;
using System.Security.Cryptography.Pkcs;
using System.Security.Cryptography.X509Certificates;

/* https://www.glennwatson.net/posts/rfc-3161-signing */

public class SignatureUtilities
{
    public static Oid SignatureTimeStampOin { get; } = new Oid("1.2.840.113549.1.9.16.2.14");

    public static async Task Main(string[] args)
    {
    	X509Certificate2 certificateToSign = new X509Certificate2();

    	X509Store store = new X509Store("MY",StoreLocation.CurrentUser);
		store.Open(OpenFlags.ReadOnly | OpenFlags.OpenExistingOnly);
		X509Certificate2Collection certificateCollection = store.Certificates.Find(X509FindType.FindBySubjectDistinguishedName, "CN=ATtiny13 Plant", false);
		store.Close();

		if(certificateCollection.Count == 0)
    	{
    		byte[] pfxBytes = File.ReadAllBytes("usbtiny.cer");
    		certificateCollection = new X509Certificate2Collection();
			certificateCollection.Import(pfxBytes, "", X509KeyStorageFlags.UserKeySet);
    	}

		foreach (X509Certificate2 x509 in certificateCollection)
        {
        	certificateToSign = x509;
        	break;
        }

        byte[] bytesToSign = File.ReadAllBytes("usbtiny.cat");

        byte[] bytesSigned = await SignWithRfc3161(bytesToSign, false, certificateToSign, new Uri("http://timestamp.comodoca.com"));
        
        File.WriteAllBytes("usbtiny0.cat", bytesSigned);
	}

	public static async Task<byte[]> SignWithRfc3161(byte[] bytesToSign, bool isDetached, X509Certificate2 certificate, Uri timeStampAuthorityUri)
    {
        // Sign our contents.
        var contentInfo = new ContentInfo(bytesToSign);
        var cms = new SignedCms(contentInfo, isDetached);
        var signer = new CmsSigner(certificate); // { IncludeOption = X509IncludeOption.WholeChain }; //X509IncludeOption.EndCertOnly;
        signer.SignedAttributes.Add(new Pkcs9SigningTime());

        cms.ComputeSignature(signer, false);

        // Generate our nonce
        byte[] nonce = new byte[8];

        using (var rng = RandomNumberGenerator.Create())
        {
            rng.GetBytes(nonce);
        }

        // Get our signing information and create the RFC3161 request
        SignerInfo newSignerInfo = cms.SignerInfos[0];

        // Now we generate our request for us to send to our RFC3161 signing authority.
        Rfc3161TimestampRequest request = Rfc3161TimestampRequest.CreateFromSignerInfo(newSignerInfo, HashAlgorithmName.SHA256, requestSignerCertificates: true, nonce: nonce);
        
        // You can use your own web request system, in this example we are just going to use a `HttpClient` class.
        var client = new HttpClient();
        var content = new ReadOnlyMemoryContent(request.Encode());
        content.Headers.ContentType = new MediaTypeHeaderValue("application/timestamp-query");
        var httpResponse = await client.PostAsync(timeStampAuthorityUri, content).ConfigureAwait(false);

        // Process our response
        if (!httpResponse.IsSuccessStatusCode)
        {
            throw new CryptographicException("There was a error from the timestamp authority. It responded with {httpResponse.StatusCode} {(int)httpResponse.StatusCode}: {httpResponse.Content}");
        }

        if (httpResponse.Content.Headers.ContentType.MediaType != "application/timestamp-reply")
        {
            throw new CryptographicException("The reply from the time stamp server was in a invalid format.");
        }

        var data = await httpResponse.Content.ReadAsByteArrayAsync().ConfigureAwait(false);

        var timestampToken = request.ProcessResponse(data, out _);

        // The RFC3161 sign certificate is separate to the contents that was signed, we need to add it to the unsigned attributes.
        newSignerInfo.UnsignedAttributes.Add(new AsnEncodedData(SignatureTimeStampOin, timestampToken.AsSignedCms().Encode()));

        return cms.Encode();
    }

    public static void Verify(byte[] body, byte[] signatureBytes)
    {
        // Create our signer.
        SignedCms signedCms;
        if (signatureBytes != null)
        {
            var contentInfo = new ContentInfo(signatureBytes);
            signedCms = new SignedCms(contentInfo, true);
        }
        else
        {
            signedCms = new SignedCms();
        }

        // Decode the body we want to check.
        signedCms.Decode(body);

        // Check the signature, true indicates we want to check the validity against certificate authorities.
        signedCms.CheckSignature(true);

        if (signedCms.SignerInfos.Count == 0)
        {
            throw new CryptographicException("Must have valid signing information. There is none in the signature.");
        }

        foreach (var signedInfo in signedCms.SignerInfos)
        {
            if (CheckRFC3161Timestamp(signedInfo, signedInfo.Certificate.NotBefore, signedInfo.Certificate.NotAfter) == false)
            {
                throw new CryptographicException("The RFC3161 timestamp is invalid.");
            }
        }
    }

    internal static bool CheckRFC3161Timestamp(SignerInfo signerInfo, DateTimeOffset? notBefore, DateTimeOffset? notAfter)
    {
        bool found = false;
        byte[] signatureBytes = null;

        foreach (CryptographicAttributeObject attr in signerInfo.UnsignedAttributes)
        {
            if (attr.Oid.Value == SignatureTimeStampOin.Value)
            {
                foreach (AsnEncodedData attrInst in attr.Values)
                {
                    byte[] attrData = attrInst.RawData;

                    // New API starts here:
                    if (!Rfc3161TimestampToken.TryDecode(attrData, out var token, out var bytesRead))
                    {
                        return false;
                    }

                    if (bytesRead != attrData.Length)
                    {
                        return false;
                    }

                    signatureBytes = signatureBytes ?? signerInfo.GetSignature();

                    // Check that the token was issued based on the SignerInfo's signature value
                    if (!token.VerifySignatureForSignerInfo(signerInfo, out _))
                    {
                        return false;
                    }

                    var timestamp = token.TokenInfo.Timestamp;

                    // Check that the signed timestamp is within the provided policy range
                    // (which may be (signerInfo.Certificate.NotBefore, signerInfo.Certificate.NotAfter);
                    // or some other policy decision)
                    if (timestamp < notBefore.GetValueOrDefault(timestamp) ||
                        timestamp > notAfter.GetValueOrDefault(timestamp))
                    {
                        return false;
                    }

                    var tokenSignerCert = token.AsSignedCms().SignerInfos[0].Certificate;

                    // Implicit policy decision: Tokens required embedded certificates (since this method has
                    // no resolver)
                    if (tokenSignerCert == null)
                    {
                        return false;
                    }

                    found = true;
                }
            }
        }

        // If we found any attributes and none of them returned an early false, then the SignerInfo is
        // conformant to policy.
        if (found)
        {
            return true;
        }

        // Inconclusive, as no signed timestamps were found
        return false; //null;
    }
}