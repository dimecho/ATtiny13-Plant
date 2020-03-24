import Foundation
import AppKit

@NSApplicationMain
class Application: NSViewController, NSApplicationDelegate
{
    func applicationDidFinishLaunching(_ aNotification: Notification)
    {
		let fileManager = FileManager.default

		if !fileManager.fileExists(atPath: "/usr/local/bin/brew")
		{
			let ruby = Process()
			ruby.launchPath = "/usr/bin/ruby"
			ruby.arguments = ["-e", "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"]
			do
			{
				startPHPServer("http://127.0.0.1:8080/install.html")
				
				try ruby.run()
				ruby.waitUntilExit()
				
				let outputPipe = Pipe()
				//let errorPipe = Pipe()

				ruby.standardOutput = outputPipe
				//ruby.standardError = errorPipe
				
				let outputData = outputPipe.fileHandleForReading.readDataToEndOfFile()
				//let errorData = errorPipe.fileHandleForReading.readDataToEndOfFile()
				
				let output = String(decoding: outputData, as: UTF8.self)
				//let error = String(decoding: errorData, as: UTF8.self)
			
				let outputFile: URL = Bundle.main.url(forResource: "Web", withExtension: nil)!

				do {
					try output.write(to: outputFile.appendingPathComponent("log.txt"), atomically: true, encoding: String.Encoding.utf8)
				} catch {}
			}
			catch let error
			{
				print(error.localizedDescription)
			}
		}
		
		if !fileManager.fileExists(atPath: "/usr/local/bin/avrdude")
		{
			let avrdude = Process()
			avrdude.launchPath = "/usr/local/bin/brew"
			avrdude.arguments = ["install", "avrdude"]
			avrdude.launch()
			avrdude.waitUntilExit()
		}
		
		if !fileManager.fileExists(atPath: "/usr/local/lib/libusb.dylib")
		{
			let libusb = Process()
			libusb.launchPath = "/usr/local/bin/brew"
			libusb.arguments = ["install", "libusb"]
			libusb.launch()
			libusb.waitUntilExit()
		}
		
		 startPHPServer("http://127.0.0.1:8080")
    }

    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool
    {
        return true
    }
    
    func applicationWillTerminate(_ notification: Notification)
    {
        let pkill = Process()
        pkill.launchPath = "/usr/bin/pkill"
        pkill.arguments =  ["-9" , "php"]
        pkill.launch()
    }
	
	private func startPHPServer(_ webURL: String)
	{
		let webPath: String = Bundle.main.path(forResource: "Web", ofType: nil)!
		let fileManager = FileManager.default
		
		let pkill = Process()
		pkill.launchPath = "/usr/bin/pkill"
		pkill.arguments =  ["-9" , "php"]
		pkill.launch()
		pkill.waitUntilExit()
	   
		let php = Process()
		php.launchPath = "/usr/bin/php"
		php.arguments =  ["-S", "127.0.0.1:8080", "-t", webPath]
		php.launch()

		let browserIdentifier: [String] = ["org.mozilla.firefox","com.apple.Safari"]
		var browserOpen = false

		let runningApplications = NSWorkspace.shared.runningApplications
		for currentApplication in runningApplications.enumerated() {
		   let runningApplication = runningApplications[currentApplication.offset]
		   if (runningApplication.activationPolicy == .regular) { // normal macOS application
			   //print(runningApplication.bundleIdentifier!)
			   if (browserIdentifier.contains(runningApplication.bundleIdentifier!)) {
				   //killProcess(Int(runningApplication.processIdentifier))
				   browserOpen = true
				   break
			   }
		   }
		}
		if(browserOpen == false) {
		   let browser = Process()
		   browser.launchPath = "/usr/bin/open"
		   if fileManager.fileExists(atPath: "/Applications/Firefox.app") {
			   browser.arguments =  ["-a", "Firefox", webURL]
		   }else if fileManager.fileExists(atPath: "/Applications/Chrome.app") {
			   browser.arguments =  ["-a", "Chrome", webURL]
		   }else{
			   browser.arguments =  ["-a", "Safari", webURL]
		   }
		   browser.launch()
		}
	}
	
	private func killProcess(_ processId: Int) {
        if let process = NSRunningApplication.init(processIdentifier: pid_t(processId)) {
            print("Killing \(processId): \(String(describing: process.localizedName!))")
            process.forceTerminate()
        }
    }
}
