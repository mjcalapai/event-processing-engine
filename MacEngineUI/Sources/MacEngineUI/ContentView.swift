import SwiftUI

struct ContentView: View {
    @StateObject private var engineManager = EngineManager()
    @State private var sampleLogs = "[2023-10-25 10:00:00] user_login source=192.168.1.100\n[2023-10-25 10:00:01] db_query latency=50ms\n[2023-10-25 10:00:02] port_scan source=10.0.0.5 target=192.168.1.10\n[2023-10-25 10:00:03] user_logout\n[2023-10-25 10:00:04] failed_login attempt=1\n[2023-10-25 10:00:05] malware_detected signature=TROJAN_01\n"
    @State private var selectedMode = "fifo"
    @State private var selection: String? = "scanner"
    
    let modes = ["fifo", "rr", "weighted"]
    
    var body: some View {
        NavigationView {
            List(selection: $selection) {
                NavigationLink(destination: dashboardView, tag: "dashboard", selection: $selection) {
                    Label("Dashboard", systemImage: "chart.bar.fill")
                }
                NavigationLink(destination: scannerView, tag: "scanner", selection: $selection) {
                    Label("Log Scanner", systemImage: "magnifyingglass")
                }
            }
            .listStyle(SidebarListStyle())
            .frame(minWidth: 200)
            
            scannerView 
        }
    }
    
    var dashboardView: some View {
        VStack {
            Text("System Overview")
                .font(.largeTitle)
                .fontWeight(.bold)
                .padding()
            
            if let metrics = engineManager.metrics {
                HStack(spacing: 20) {
                    MetricCard(title: "Throughput", value: String(format: "%.2f ev/s", metrics.throughput), color: .blue)
                    MetricCard(title: "Avg Latency", value: String(format: "%.0f ns", metrics.averageLatencyNs), color: .purple)
                    MetricCard(title: "Alerts", value: "\(metrics.alerts)", color: .red)
                }
                .padding()
                
                if let latencyBySeverity = metrics.latencyBySeverity {
                    VStack(alignment: .leading) {
                        Text("Latency by Severity")
                            .font(.headline)
                        ForEach(latencyBySeverity.keys.sorted(), id: \.self) { key in
                            if let severity = latencyBySeverity[key] {
                                HStack {
                                    Text(key)
                                        .frame(width: 100, alignment: .leading)
                                    ProgressView(value: min(severity.avg / 1000000.0, 1.0)) // normalize to max 1ms for display
                                    Text(String(format: "%.0f ns", severity.avg))
                                }
                            }
                        }
                    }
                    .padding()
                    .background(Color(NSColor.controlBackgroundColor))
                    .cornerRadius(10)
                    .padding()
                }
            } else {
                Text("Run a scan to see metrics.")
                    .foregroundColor(.secondary)
            }
            Spacer()
        }
        .frame(maxWidth: .infinity, maxHeight: .infinity)
    }
    
    var scannerView: some View {
        VStack(spacing: 0) {
            HStack {
                Text("Host Log Scanner")
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
                
                Button(action: {
                    let logs = engineManager.fetchOSLogs(last: 60) // last 1 minute
                    sampleLogs = String(logs.prefix(2000)) + (logs.count > 2000 ? "\n... (truncated for display, full logs sent to engine)" : "")
                    engineManager.runEngine(with: logs, mode: selectedMode)
                }) {
                    Text(engineManager.isRunning ? "Scanning..." : "Scan Live OSLog (1m)")
                }
                .disabled(engineManager.isRunning)
                .buttonStyle(BorderedProminentButtonStyle())
                .padding(.leading, 10)
                
                Button(action: {
                    engineManager.runEngine(with: sampleLogs, mode: selectedMode)
                }) {
                    Text(engineManager.isRunning ? "Scanning..." : "Run Editor Scan")
                }
                .disabled(engineManager.isRunning)
                .buttonStyle(BorderedProminentButtonStyle())
                .padding(.leading, 10)
            }
            .padding()
            .background(Color(NSColor.windowBackgroundColor))
            
            HSplitView {
                VStack(alignment: .leading) {
                    Text("Input Logs")
                        .font(.headline)
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
