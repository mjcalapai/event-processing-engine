import Foundation
import Combine
import OSLog

struct SeverityMetrics: Decodable {
    let avg: Double
    let max: Double
    let count: Int
}

struct MetricsResult: Decodable {
    let runtimeMs: Int
    let processed: Int
    let throughput: Double
    let averageLatencyNs: Double
    let latencyBySeverity: [String: SeverityMetrics]?
    let alerts: Int
    let agingBoosts: Int
}

class EngineManager: ObservableObject, @unchecked Sendable {
    @Published var isRunning = false
    @Published var consoleOutput: String = ""
    @Published var metrics: MetricsResult?

    func runEngine(with logs: String, mode: String = "fifo") {
        guard !isRunning else { return }
        isRunning = true
        consoleOutput = "Starting Event Engine in \(mode.uppercased()) mode...\n"
        metrics = nil
        
        executeEngine(with: logs, mode: mode)
    }

    func runLiveScan(mode: String = "fifo") {
        guard !isRunning else { return }
        isRunning = true
        consoleOutput = "Fetching live OSLogs (this might take a moment)...\n"
        metrics = nil
        
        Task {
            let logs = await Task.detached(priority: .userInitiated) { [weak self] in
                return self?.fetchOSLogs(last: 60) ?? ""
            }.value
            
            await MainActor.run {
                self.consoleOutput += "Fetched logs. Starting Engine...\n"
                self.executeEngine(with: logs, mode: mode)
            }
        }
    }

    private func executeEngine(with logs: String, mode: String) {
        DispatchQueue.global(qos: .userInitiated).async {
            let process = Process()
            
            let executablePath = "/Users/nathanielparryluff/Desktop/event-processing-engine/event_engine_app"
            
            process.executableURL = URL(fileURLWithPath: executablePath)
            process.arguments = [mode, "2", "2", "5", "-", "--json"]
            
            let pipeIn = Pipe()
            let pipeOut = Pipe()
            let pipeErr = Pipe()
            
            process.standardInput = pipeIn
            process.standardOutput = pipeOut
            process.standardError = pipeErr
            
            do {
                try process.run()
                
                var outData = Data()
                var errData = Data()
                let readGroup = DispatchGroup()
                let ioQueue = DispatchQueue(label: "ioQueue")
                
                readGroup.enter()
                pipeOut.fileHandleForReading.readabilityHandler = { fh in
                    let data = fh.availableData
                    if data.isEmpty {
                        pipeOut.fileHandleForReading.readabilityHandler = nil
                        readGroup.leave()
                    } else {
                        ioQueue.async { outData.append(data) }
                    }
                }
                
                readGroup.enter()
                pipeErr.fileHandleForReading.readabilityHandler = { fh in
                    let data = fh.availableData
                    if data.isEmpty {
                        pipeErr.fileHandleForReading.readabilityHandler = nil
                        readGroup.leave()
                    } else {
                        ioQueue.async { errData.append(data) }
                    }
                }
                
                if let data = logs.data(using: .utf8) {
                    do {
                        try pipeIn.fileHandleForWriting.write(contentsOf: data)
                    } catch {
                        print("Write to stdin failed: \(error)")
                    }
                }
                try pipeIn.fileHandleForWriting.close()
                
                process.waitUntilExit()
                readGroup.wait()
                
                let outString = String(data: outData, encoding: .utf8) ?? ""
                let errString = String(data: errData, encoding: .utf8) ?? ""
                
                DispatchQueue.main.async {
                    self.consoleOutput += outString
                    if !errString.isEmpty {
                        self.consoleOutput += "\n[Error]\n" + errString
                    }
                    self.isRunning = false
                    
                    if let jsonStart = outString.range(of: "{"), let jsonEnd = outString.range(of: "}", options: .backwards) {
                        let jsonStr = String(outString[jsonStart.lowerBound...jsonEnd.upperBound])
                        if let jsonData = jsonStr.data(using: .utf8) {
                            do {
                                self.metrics = try JSONDecoder().decode(MetricsResult.self, from: jsonData)
                            } catch {
                                print("JSON Decode error: \(error)")
                            }
                        }
                    }
                }
            } catch {
                DispatchQueue.main.async {
                    self.consoleOutput += "\nFailed to start process: \(error.localizedDescription)"
                    self.isRunning = false
                }
            }
        }
    }
    
    func fetchOSLogs(last seconds: TimeInterval) -> String {
        var logLines = [String]()
        logLines.reserveCapacity(20000)
        do {
            let store = try OSLogStore(scope: .system)
            let position = store.position(timeIntervalSinceEnd: -seconds)
            let entries = try store.getEntries(at: position)
            
            var idCounter = 1
            let dateFormatter = DateFormatter()
            dateFormatter.dateFormat = "yyyy-MM-dd'T'HH:mm:ss'Z'"
            dateFormatter.timeZone = TimeZone(secondsFromGMT: 0)
            
            for entry in entries {
                guard let log = entry as? OSLogEntryLog else { continue }
                
                let ts = dateFormatter.string(from: log.date)
                let source = log.subsystem.isEmpty ? "system" : log.subsystem
                
                let severity: String
                switch log.level {
                case .fault: severity = "CRITICAL"
                case .error: severity = "ERROR"
                case .info: severity = "INFO"
                case .debug: severity = "DEBUG"
                default: severity = "WARNING"
                }
                
                let payload = log.composedMessage.replacingOccurrences(of: "|", with: " ").replacingOccurrences(of: "\n", with: " ")
                
                logLines.append("\(idCounter)|\(ts)|\(source)|127.0.0.1|\(severity)|SYSTEM|\(payload)")
                idCounter += 1
                
                if logLines.count >= 20000 { break } // Limit logs to prevent massive memory usage
            }
        } catch {
            print("Failed to fetch OSLogs: \(error)")
        }
        return logLines.joined(separator: "\n") + (logLines.isEmpty ? "" : "\n")
    }
}
