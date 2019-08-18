import Foundation
import Cocoa

@NSApplicationMain
class Application: NSViewController, NSApplicationDelegate
{
    func applicationDidFinishLaunching(_ aNotification: Notification)
    {
		let task = Process()
		task.launchPath = Bundle.main.path(forResource: "run", ofType:nil)
		task.launch()
		task.waitUntilExit()
    }

    func applicationShouldTerminateAfterLastWindowClosed(_ sender: NSApplication) -> Bool
    {
        return true
    }
    
    func applicationWillTerminate(_ notification: Notification)
    {
        let task = Process()
        task.launchPath = "pkill"
        task.arguments =  ["-9" , "php"]
        task.launch()
    }
}
