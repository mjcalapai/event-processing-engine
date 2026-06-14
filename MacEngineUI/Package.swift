// swift-tools-version: 6.2

import PackageDescription

let package = Package(
    name: "MacEngineUI",
    platforms: [.macOS(.v12)],
    targets: [
        .executableTarget(
            name: "MacEngineUI"
        ),
    ]
)
