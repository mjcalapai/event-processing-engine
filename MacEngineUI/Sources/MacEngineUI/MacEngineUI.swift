import SwiftUI

@main
struct MacEngineUIApp: App {
    var body: some Scene {
        WindowGroup {
            ContentView()
                .frame(minWidth: 900, minHeight: 600)
        }
        .windowStyle(HiddenTitleBarWindowStyle())
    }
}
