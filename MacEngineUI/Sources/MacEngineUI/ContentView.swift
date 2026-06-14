import SwiftUI
import UniformTypeIdentifiers

struct ContentView: View {
    @StateObject private var engineManager = EngineManager()
    @State private var sampleLogs = "[2023-10-25 10:00:00] user_login source=192.168.1.100\n[2023-10-25 10:00:01] db_query latency=50ms\n[2023-10-25 10:00:02] port_scan source=10.0.0.5 target=192.168.1.10\n[2023-10-25 10:00:03] user_logout\n[2023-10-25 10:00:04] failed_login attempt=1\n[2023-10-25 10:00:05] malware_detected signature=TROJAN_01\n"
    @State private var selectedMode = "fifo"
    @State private var selection: String? = "scanner"
    @State private var isImporting = false
    
    let modes = ["fifo", "rr", "weighted"]
    
    var body: some View {
        NavigationView {
            List(selection: $selection) {
                NavigationLink(destination: scannerView, tag: "scanner", selection: $selection) {
                    Label("Log Scanner", systemImage: "magnifyingglass")
                }
                NavigationLink(destination: alertsView, tag: "alerts", selection: $selection) {
                    Label("Threat Alerts", systemImage: "exclamationmark.triangle.fill")
                }
            }
            .listStyle(SidebarListStyle())
            .frame(minWidth: 200)
            
            scannerView 
        }
    }
    
    var scannerView: some View {
        VStack(spacing: 0) {
            HStack {
                Text("Log Scanner & Dashboard")
                    .font(.title)
                    .fontWeight(.semibold)
                Spacer()
                Picker("Scheduler Mode", selection: $selectedMode) {
                    ForEach(modes, id: \.self) { mode in
                        Text(mode.uppercased()).tag(mode)
                    }
                }
                .pickerStyle(SegmentedPickerStyle())
                .frame(width: 200)
                .padding(.horizontal, 20)
                
                Button(action: {
                    engineManager.runLiveScan(mode: selectedMode)
                }) {
                    Text(engineManager.isRunning ? "Scanning..." : "Live OSLog")
                }
                .disabled(engineManager.isRunning)
                .buttonStyle(BorderedProminentButtonStyle())
                .padding(.leading, 20)
                
                Button(action: {
                    engineManager.runEngine(with: sampleLogs, mode: selectedMode)
                }) {
                    Text(engineManager.isRunning ? "Scanning..." : "Run Scan")
                }
                .disabled(engineManager.isRunning)
                .buttonStyle(BorderedProminentButtonStyle())
                .padding(.leading, 10)
            }
            .padding()
            .background(Color(NSColor.windowBackgroundColor))
            
            if let metrics = engineManager.metrics {
                VStack(spacing: 10) {
                    HStack(spacing: 20) {
                        MetricCard(title: "Throughput", value: String(format: "%.2f ev/s", metrics.throughput), color: .blue)
                        MetricCard(title: "Avg Latency", value: String(format: "%.0f ns", metrics.averageLatencyNs), color: .purple)
                        MetricCard(title: "Alerts", value: "\(metrics.alerts)", color: .red)
                        
                        if let latencyBySeverity = metrics.latencyBySeverity {
                            VStack(alignment: .leading, spacing: 4) {
                                Text("Latency by Severity")
                                    .font(.caption)
                                    .foregroundColor(.secondary)
                                ForEach(latencyBySeverity.keys.sorted(), id: \.self) { key in
                                    if let severity = latencyBySeverity[key] {
                                        HStack {
                                            Text(key).font(.caption).frame(width: 70, alignment: .leading)
                                            ProgressView(value: min(severity.avg / 1000000.0, 1.0))
                                                .frame(width: 80)
                                            Text(String(format: "%.0f ns", severity.avg)).font(.caption)
                                        }
                                    }
                                }
                            }
                            .padding(.leading, 20)
                        }
                    }
                }
                .padding()
                .frame(maxWidth: .infinity)
                .background(Color(NSColor.controlBackgroundColor))
                .overlay(Rectangle().frame(height: 1).foregroundColor(Color.gray.opacity(0.2)), alignment: .bottom)
            }
            
            HSplitView {
                VStack(alignment: .leading) {
                    HStack {
                        Text("Input Logs")
                            .font(.headline)
                        Spacer()
                        Button(action: {
                            isImporting = true
                        }) {
                            Image(systemName: "square.and.arrow.up")
                            Text("Upload .txt")
                        }
                        .fileImporter(
                            isPresented: $isImporting,
                            allowedContentTypes: [.text],
                            allowsMultipleSelection: false
                        ) { result in
                            do {
                                guard let selectedFile = try result.get().first else { return }
                                if selectedFile.startAccessingSecurityScopedResource() {
                                    defer { selectedFile.stopAccessingSecurityScopedResource() }
                                    let contents = try String(contentsOf: selectedFile)
                                    sampleLogs = contents
                                } else {
                                    let contents = try String(contentsOf: selectedFile)
                                    sampleLogs = contents
                                }
                            } catch {
                                print("Error reading file: \(error.localizedDescription)")
                            }
                        }
                    }
                    .padding([.top, .leading, .trailing])
                    
                    TextEditor(text: $sampleLogs)
                        .font(.system(.body, design: .monospaced))
                        .padding()
                }
                
                VStack(alignment: .leading) {
                    Text("Engine Output")
                        .font(.headline)
                        .padding([.top, .leading, .trailing])
                    ScrollView {
                        Text(engineManager.consoleOutput)
                            .font(.system(.body, design: .monospaced))
                            .frame(maxWidth: .infinity, alignment: .leading)
                            .padding()
                    }
                    .background(Color.black.opacity(0.05))
                }
            }
        }
    }
    
    var alertsView: some View {
        VStack(alignment: .leading) {
            Text("Suspected Attacks & Malicious Activity")
                .font(.largeTitle)
                .fontWeight(.bold)
                .padding()
            
            if engineManager.alertsList.isEmpty {
                Text("No malicious activity detected during the last scan.")
                    .foregroundColor(.secondary)
                    .padding()
            } else {
                List(engineManager.alertsList) { alert in
                    VStack(alignment: .leading, spacing: 5) {
                        HStack {
                            Image(systemName: alert.level == "CRITICAL" ? "xmark.octagon.fill" : "exclamationmark.triangle.fill")
                                .foregroundColor(alert.level == "CRITICAL" ? .red : .orange)
                            Text(alert.level)
                                .font(.headline)
                                .foregroundColor(alert.level == "CRITICAL" ? .red : .orange)
                        }
                        Text(alert.rawString)
                            .font(.system(.body, design: .monospaced))
                            .foregroundColor(.primary)
                    }
                    .padding(.vertical, 4)
                }
            }
            Spacer()
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
    }
}

struct MetricCard: View {
    var title: String
    var value: String
    var color: Color
    
    var body: some View {
        VStack {
            Text(title)
                .font(.subheadline)
                .foregroundColor(.secondary)
            Text(value)
                .font(.title2)
                .fontWeight(.bold)
                .foregroundColor(color)
        }
        .padding()
        .frame(minWidth: 150)
        .background(Color(NSColor.controlBackgroundColor))
        .cornerRadius(10)
        .shadow(color: Color.black.opacity(0.1), radius: 3, x: 0, y: 1)
    }
}
