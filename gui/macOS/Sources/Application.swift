import Foundation
import AppKit

@NSApplicationMain
class Application: NSViewController, NSApplicationDelegate, NSUserNotificationCenterDelegate
{
	let webPath: String = Bundle.main.path(forResource: "Web", ofType: nil)!
	
    func applicationDidFinishLaunching(_ aNotification: Notification)
    {
		var reachability: Reachability?
		let fileManager = FileManager.default
		
		if !fileManager.fileExists(atPath: webPath + "/avrdude") {
			do {
				try fileManager.moveItem(atPath: webPath + "/../avrdude", toPath: webPath + "/avrdude")
				try fileManager.moveItem(atPath: webPath + "/../avrdude.conf", toPath: webPath + "/avrdude.conf")
			} catch {}
		}
		
		//if !fileManager.fileExists(atPath: "/opt/local/bin/port") {
		if !fileManager.fileExists(atPath: "/usr/local/bin/brew") {
			reachability = try? Reachability(hostname: "1.1.1.1")
			if reachability?.connection != .unavailable {
				self.downloadInstallerWhenReachable(reachability!)
			}else{
				NotificationCenter.default.addObserver(
					self,
					selector: #selector(self.reachabilityChanged(_:)),
					name: .reachabilityChanged,
					object: reachability
				)
				reachability?.whenReachable = { reachability in
					   self.downloadInstallerWhenReachable(reachability)
				}
			}
		}
		
		//if !fileManager.fileExists(atPath: "/opt/local/lib/libusb-1.0.dylib") {
		if !fileManager.fileExists(atPath: "/usr/local/lib/libusb-1.0.dylib") {
			reachability = try? Reachability(hostname: "1.1.1.1")
			if reachability?.connection != .unavailable {
				self.downloadLibUSBWhenReachable(reachability!)
			}else{
				NotificationCenter.default.addObserver(
					self,
					selector: #selector(self.reachabilityChanged(_:)),
					name: .reachabilityChanged,
					object: reachability
				)
				reachability?.whenReachable = { reachability in
					self.downloadLibUSBWhenReachable(reachability)
				}
			}
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
		/*
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
		*/
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
		//}
	}
	
	private func killProcess(_ processId: Int) {
        if let process = NSRunningApplication.init(processIdentifier: pid_t(processId)) {
            print("Killing \(processId): \(String(describing: process.localizedName!))")
            process.forceTerminate()
        }
    }
	
	func downloadInstallerWhenReachable(_ reachability: Reachability) {
        print("\(reachability.description) - \(reachability.connection.description)")
		
		let notification = NSUserNotification()
		notification.soundName = NSUserNotificationDefaultSoundName
		notification.title = "Installer"
		
		let installer = Process()
		
		/*
		do {
			let fileManager = FileManager.default
			if (fileManager.fileExists(atPath: "/usr/local/bin/brew")) {
				notification.subtitle = "WARNING: Brew is already installed!"
				NSUserNotificationCenter.default.delegate = self
				NSUserNotificationCenter.default.deliver(notification)
				return
			}
			var installerOS: String = ""
			let installerURL: String = "https://distfiles.macports.org/MacPorts/"
			let installerCatalina: String   = "MacPorts-2.6.2-10.15-Catalina.pkg"
			let installerMojave: String     = "MacPorts-2.6.2-10.14-Mojave.pkg"
			let installerHighSierra: String = "MacPorts-2.6.2-10.13-HighSierra.pkg"
			
			let os = ProcessInfo().operatingSystemVersion
			switch (os.majorVersion, os.minorVersion, os.patchVersion) {
			case (10, 13, _):
				installerOS = installerHighSierra
			case (10, 14, _):
				installerOS = installerMojave
			default:
				installerOS = installerCatalina
			}
			let downloadsURL = try fileManager.url(for: .downloadsDirectory, in: .userDomainMask, appropriateFor: nil, create: false)
			let savedURL = downloadsURL.appendingPathComponent(installerOS)
			
			if !fileManager.fileExists(atPath: savedURL.path)
			{
				notification.subtitle = " ...Downloading MacPorts"
				NSUserNotificationCenter.default.delegate = self
				NSUserNotificationCenter.default.deliver(notification)
				
				let downloadTask = URLSession.shared.downloadTask(with: URL(string: installerURL + installerOS)!) {
					urlOrNil, responseOrNil, errorOrNil in
					// check for and handle errors:
					// * errorOrNil should be nil
					// * responseOrNil should be an HTTPURLResponse with statusCode in 200..<299
					
					guard let fileURL = urlOrNil else { return }
					do {
						try FileManager.default.moveItem(at: fileURL, to: savedURL)
					} catch {
						print(error.localizedDescription)
						notification.subtitle = error.localizedDescription
						NSUserNotificationCenter.default.delegate = self
						NSUserNotificationCenter.default.deliver(notification)
					}
				}
				downloadTask.resume()
			}
			
			notification.subtitle = " ...Installing MacPorts"
			NSUserNotificationCenter.default.delegate = self
			NSUserNotificationCenter.default.deliver(notification)
			installer.launchPath = "/usr/bin/open"
			installer.arguments = ["-W", savedURL.path]
		} catch {
			print(error.localizedDescription)
		}
		*/
		
		notification.subtitle = " ...Installing Brew"
		NSUserNotificationCenter.default.delegate = self
		NSUserNotificationCenter.default.deliver(notification)
		installer.launchPath = "/usr/bin/ruby"
		installer.arguments = ["-e", "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install)"]
		
		do
		{
			try installer.run()
			installer.waitUntilExit()
		}
		catch let error
		{
			notification.subtitle = error.localizedDescription
			NSUserNotificationCenter.default.delegate = self
			NSUserNotificationCenter.default.deliver(notification)
		}
    }
	
	func downloadLibUSBWhenReachable(_ reachability: Reachability) {
        print("\(reachability.description) - \(reachability.connection.description)")
		
		startPHPServer("http://127.0.0.1:8080/install.html")
		
		let fileManager = FileManager.default
		let notification = NSUserNotification()
		notification.soundName = NSUserNotificationDefaultSoundName
		notification.title = "Installer"
		notification.subtitle = " ...Installing LibUSB"
		NSUserNotificationCenter.default.delegate = self
		NSUserNotificationCenter.default.deliver(notification)
		
		let libusb = Process()
		
		if fileManager.fileExists(atPath: "/usr/local/bin/brew") {
			libusb.launchPath = "/usr/local/bin/brew"
			libusb.arguments = ["install", "libusb", "libusb-compat"]
		}else{
			libusb.launchPath = "/usr/bin/osascript"
			libusb.arguments = ["-e", "do shell script \"./" + Bundle.main.path(forResource: "macports", ofType: nil)! + "\" with administrator privileges"]
			/*
			libusb.launchPath = "/opt/local/bin/port"
			libusb.arguments = ["sync"]
			libusb.launch()
			libusb.waitUntilExit()
			libusb.arguments = ["install", "libusb", "libusb-compat"]
			*/
		}
		
		let outputPipe = Pipe()
		//let errorPipe = Pipe()

		libusb.standardOutput = outputPipe
		//libusb.standardError = errorPipe
		
		let outputData = outputPipe.fileHandleForReading.readDataToEndOfFile()
		//let errorData = errorPipe.fileHandleForReading.readDataToEndOfFile()
		
		let output = String(decoding: outputData, as: UTF8.self)
		//let error = String(decoding: errorData, as: UTF8.self)
		
		libusb.launch()
		libusb.waitUntilExit()
		
		do {
			try output.write(to: URL(string: webPath + "/log.txt")!, atomically: true, encoding: String.Encoding.utf8)
			//try error.write(to: URL(string: webPath + "/log.txt")!, atomically: true, encoding: String.Encoding.utf8)
		} catch {
			print(error.localizedDescription)
		}
    }
	
	@objc func reachabilityChanged(_ note: Notification) {
        let reachability = note.object as! Reachability
        let notification = NSUserNotification()
		notification.title = "Internet"

        if reachability.connection != .unavailable {
			notification.subtitle = "Connection Available"
        } else {
			notification.subtitle = "Connection Lost"
        }
		notification.soundName = NSUserNotificationDefaultSoundName
		NSUserNotificationCenter.default.delegate = self
		NSUserNotificationCenter.default.deliver(notification)
    }
	
	func userNotificationCenter(_ center: NSUserNotificationCenter, shouldPresent notification: NSUserNotification) -> Bool {
			return true
	}
}
