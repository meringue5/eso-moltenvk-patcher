import AppKit
import CryptoKit
import Foundation

struct TargetProfile: Decodable {
    let description: String
    let sha256: String
    let original_bink_sha256: String
    let uuid: String
}

struct InstallState: Codable {
    let targetPath: String
    let executableSHA256: String
    let originalBinkSHA256: String
    let installedAt: Date
}

enum PatcherError: LocalizedError {
    case message(String)
    var errorDescription: String? {
        switch self { case .message(let message): return message }
    }
}

final class PatcherEngine {
    private let files = FileManager.default
    private let resourceRoot: URL
    private let profile: TargetProfile

    init() throws {
        guard let root = Bundle.main.resourceURL else {
            throw PatcherError.message("The application resources are unavailable.")
        }
        resourceRoot = root
        let profileURL = root.appendingPathComponent("Payload/target-profile.json")
        profile = try JSONDecoder().decode(TargetProfile.self, from: Data(contentsOf: profileURL))
    }

    func candidates() -> [URL] {
        let home = files.homeDirectoryForCurrentUser
        let known = [
            home.appendingPathComponent("Library/Application Support/Steam/steamapps/common/Zenimax Online/The Elder Scrolls Online/game_mac/pubplayerclient/eso.app"),
            URL(fileURLWithPath: "/Applications/ZeniMax Online/Launcher.app/game_mac/pubplayerclient/eso.app")
        ]
        return known.filter { files.fileExists(atPath: $0.path) }
    }

    func resolve(_ selected: URL) throws -> URL {
        if selected.pathExtension == "app", selected.lastPathComponent == "eso.app" { return selected }
        let candidate = selected.appendingPathComponent("game_mac/pubplayerclient/eso.app")
        guard files.fileExists(atPath: candidate.path) else {
            throw PatcherError.message("Select eso.app or the ESO Launcher.app that contains it.")
        }
        return candidate
    }

    func check(_ app: URL) throws -> String {
        let layout = try layout(app)
        let digest = try sha256(layout.executable)
        guard digest == profile.sha256 else {
            return "Unsupported ESO client\n\nFound: \(digest)\nSupported: \(profile.sha256)\n\nNo files were changed."
        }
        let marker = layout.macOS.appendingPathComponent(".teso4m4-enable")
        if files.fileExists(atPath: marker.path) {
            let bridge = resourceRoot.appendingPathComponent("Payload/libBink2Macx64.dylib")
            guard try sha256(layout.bink) == sha256(bridge) else {
                return "Incomplete patch state\n\nThe patch marker exists but the installed bridge does not match this release. Use Remove to restore the verified backup."
            }
            return "Supported ESO client\n\n\(profile.description)\n\nPatch status: installed\nLocation: \(app.path)"
        }
        guard try sha256(layout.bink) == profile.original_bink_sha256 else {
            return "Unsupported Bink library\n\nThe selected ESO client does not contain the original Bink library expected by this profile.\n\nNo files were changed."
        }
        return "Supported ESO client\n\n\(profile.description)\n\nPatch status: not installed\nLocation: \(app.path)"
    }

    func install(_ app: URL) throws -> String {
        let layout = try supportedLayout(app)
        try requireIdle(app)
        let marker = layout.macOS.appendingPathComponent(".teso4m4-enable")
        guard !files.fileExists(atPath: marker.path) else {
            throw PatcherError.message("The patch is already installed. Use Repair or Remove.")
        }
        for name in ["libBink2Macx64.teso4m4-original.dylib", "libMoltenVK.teso4m4.dylib"] {
            guard !files.fileExists(atPath: layout.macOS.appendingPathComponent(name).path) else {
                throw PatcherError.message("A previous patch attempt left \(name). Use Remove to restore the verified backup first.")
            }
        }
        let stateDir = try stateDirectory(for: app)
        try files.createDirectory(at: stateDir, withIntermediateDirectories: true)
        let backup = stateDir.appendingPathComponent("original-libBink2Macx64.dylib")
        try copyVerified(layout.bink, to: backup)
        let originalHash = try sha256(backup)
        let renamedOriginal = layout.macOS.appendingPathComponent("libBink2Macx64.teso4m4-original.dylib")
        let replacement = resourceRoot.appendingPathComponent("Payload/libBink2Macx64.dylib")
        let runtime = resourceRoot.appendingPathComponent("Payload/libMoltenVK.teso4m4.dylib")
        do {
            try copyVerified(layout.bink, to: renamedOriginal)
            try retagOriginalBink(renamedOriginal)
            try installReplacement(from: layout.bink, copyOriginalTo: renamedOriginal, replacement: replacement, preserveSource: false)
            try installReplacement(from: runtime, copyOriginalTo: layout.macOS.appendingPathComponent("libMoltenVK.teso4m4.dylib"), replacement: runtime, preserveSource: false)
            try "startup-compositor-neutralize\n".write(to: marker, atomically: true, encoding: .utf8)
            let state = InstallState(targetPath: app.path, executableSHA256: try sha256(layout.executable), originalBinkSHA256: originalHash, installedAt: Date())
            try JSONEncoder().encode(state).write(to: stateDir.appendingPathComponent("install-state.json"))
        } catch {
            try? restoreBink(from: backup, to: layout.bink)
            throw error
        }
        return "Installed successfully. Launch ESO through your usual Steam or ZeniMax launcher."
    }

    func remove(_ app: URL) throws -> String {
        let layout = try layout(app)
        try requireIdle(app)
        let stateDir = try stateDirectory(for: app)
        let backup = stateDir.appendingPathComponent("original-libBink2Macx64.dylib")
        let stateURL = stateDir.appendingPathComponent("install-state.json")
        guard files.fileExists(atPath: backup.path) else {
            throw PatcherError.message("A verified restore backup was not found for this ESO installation.")
        }
        guard let state = try? JSONDecoder().decode(InstallState.self, from: Data(contentsOf: stateURL)),
              state.targetPath == app.path,
              state.executableSHA256 == profile.sha256,
              try sha256(layout.executable) == profile.sha256,
              state.originalBinkSHA256 == profile.original_bink_sha256,
              try sha256(backup) == profile.original_bink_sha256 else {
            throw PatcherError.message("The restore record or backup does not match this exact ESO client. No files were changed.")
        }
        try restoreBink(from: backup, to: layout.bink)
        for name in ["libBink2Macx64.teso4m4-original.dylib", "libMoltenVK.teso4m4.dylib", ".teso4m4-enable"] {
            try? files.removeItem(at: layout.macOS.appendingPathComponent(name))
        }
        return "Removed successfully. The original Bink library has been restored."
    }

    func repair(_ app: URL) throws -> String {
        _ = try remove(app)
        return try install(app)
    }

    private func layout(_ app: URL) throws -> (macOS: URL, executable: URL, bink: URL) {
        let macOS = app.appendingPathComponent("Contents/MacOS")
        let executable = macOS.appendingPathComponent("eso")
        let bink = macOS.appendingPathComponent("libBink2Macx64.dylib")
        guard files.fileExists(atPath: executable.path), files.fileExists(atPath: bink.path) else {
            throw PatcherError.message("This does not contain a supported macOS ESO client layout.")
        }
        return (macOS, executable, bink)
    }

    private func supportedLayout(_ app: URL) throws -> (macOS: URL, executable: URL, bink: URL) {
        let result = try layout(app)
        guard try sha256(result.executable) == profile.sha256 else {
            throw PatcherError.message("This ESO build is not in the supported profile. No files were changed.")
        }
        guard try sha256(result.bink) == profile.original_bink_sha256 else {
            throw PatcherError.message("This ESO client does not contain the original Bink library expected by this profile. No files were changed.")
        }
        return result
    }

    private func stateDirectory(for app: URL) throws -> URL {
        let id = SHA256.hash(data: Data(app.path.utf8)).map { String(format: "%02x", $0) }.joined()
        let root = files.homeDirectoryForCurrentUser.appendingPathComponent("Library/Application Support/ESO MoltenVK Patcher/Installations")
        return root.appendingPathComponent(id)
    }

    private func sha256(_ url: URL) throws -> String {
        let digest = SHA256.hash(data: try Data(contentsOf: url, options: .mappedIfSafe))
        return digest.map { String(format: "%02x", $0) }.joined()
    }

    private func copyVerified(_ source: URL, to destination: URL) throws {
        let temporary = destination.appendingPathExtension("installing")
        try? files.removeItem(at: temporary)
        try files.copyItem(at: source, to: temporary)
        guard try sha256(source) == sha256(temporary) else { throw PatcherError.message("Copy verification failed.") }
        try? files.removeItem(at: destination)
        try files.moveItem(at: temporary, to: destination)
    }

    private func installReplacement(from source: URL, copyOriginalTo original: URL, replacement: URL, preserveSource: Bool = true) throws {
        if preserveSource { try copyVerified(source, to: original) }
        let temporary = source.appendingPathExtension("eso-moltenvk-patcher-installing")
        try? files.removeItem(at: temporary)
        try files.copyItem(at: replacement, to: temporary)
        guard try sha256(replacement) == sha256(temporary) else { throw PatcherError.message("Payload verification failed.") }
        if files.fileExists(atPath: source.path) { try files.removeItem(at: source) }
        try files.moveItem(at: temporary, to: source)
    }

    private func restoreBink(from backup: URL, to destination: URL) throws {
        let temporary = destination.appendingPathExtension("eso-moltenvk-patcher-restoring")
        try? files.removeItem(at: temporary)
        try files.copyItem(at: backup, to: temporary)
        guard try sha256(backup) == sha256(temporary) else { throw PatcherError.message("Restore verification failed.") }
        try files.removeItem(at: destination)
        try files.moveItem(at: temporary, to: destination)
    }

    private func retagOriginalBink(_ original: URL) throws {
        let task = Process()
        task.executableURL = URL(fileURLWithPath: "/usr/bin/install_name_tool")
        task.arguments = ["-id", "@loader_path/libBink2Macx64.teso4m4-original.dylib", original.path]
        try task.run(); task.waitUntilExit()
        guard task.terminationStatus == 0 else {
            throw PatcherError.message("Could not prepare the original Bink library for the reversible bridge.")
        }
    }

    private func requireIdle(_ app: URL) throws {
        let running = NSWorkspace.shared.runningApplications.contains { $0.executableURL?.lastPathComponent == "eso" }
        guard !running else { throw PatcherError.message("Quit ESO before changing its files.") }
        let launcher = Process(); launcher.executableURL = URL(fileURLWithPath: "/usr/bin/pgrep")
        launcher.arguments = ["-f", "/ZeniMax Online Studios Launcher"]
        try launcher.run(); launcher.waitUntilExit()
        guard launcher.terminationStatus == 1 else {
            throw PatcherError.message("Quit the ZeniMax launcher before changing ESO files.")
        }
        let task = Process(); task.executableURL = URL(fileURLWithPath: "/usr/sbin/lsof")
        task.arguments = ["-Fn", "+D", app.path]
        let output = Pipe(); task.standardOutput = output; task.standardError = Pipe()
        try task.run(); task.waitUntilExit()
        let openFiles = String(data: output.fileHandleForReading.readDataToEndOfFile(), encoding: .utf8) ?? ""
        guard openFiles.isEmpty, task.terminationStatus == 1 else {
            throw PatcherError.message("Close any process using this ESO installation before continuing.")
        }
        try requireSteamDownloadIdle(for: app)
    }

    private func requireSteamDownloadIdle(for app: URL) throws {
        let marker = "/steamapps/common/"
        guard let range = app.path.range(of: marker) else { return }
        let steamapps = String(app.path[..<range.lowerBound]) + "/steamapps"
        let manifest = URL(fileURLWithPath: steamapps).appendingPathComponent("appmanifest_306130.acf")
        guard files.fileExists(atPath: manifest.path) else { return }
        let acf = try String(contentsOf: manifest, encoding: .utf8)
        func value(_ key: String) -> String? {
            let expression = "\\\"" + NSRegularExpression.escapedPattern(for: key) + "\\\"\\\\s*\\\"([^\\\"]*)\\\""
            guard let regex = try? NSRegularExpression(pattern: expression),
                  let match = regex.firstMatch(in: acf, range: NSRange(acf.startIndex..., in: acf)),
                  let range = Range(match.range(at: 1), in: acf) else { return nil }
            return String(acf[range])
        }
        let staged = URL(fileURLWithPath: steamapps).appendingPathComponent("downloading/306130")
        let stagedFiles = (try? files.contentsOfDirectory(atPath: staged.path)) ?? []
        guard value("StateFlags") == "4",
              let total = value("BytesToDownload"), total == value("BytesDownloaded"),
              stagedFiles.isEmpty else {
            throw PatcherError.message("Steam reports ESO as updating or incomplete. Wait for its ESO download to finish.")
        }
    }
}

@main final class AppDelegate: NSObject, NSApplicationDelegate {
    private var engine: PatcherEngine!
    private var selected: URL?
    private let status = NSTextView()
    private let title = NSTextField(labelWithString: "ESO MoltenVK Patcher")

    func applicationDidFinishLaunching(_ notification: Notification) {
        do { engine = try PatcherEngine() } catch { present(error); return }
        let window = NSWindow(contentRect: NSRect(x: 0, y: 0, width: 620, height: 390), styleMask: [.titled, .closable], backing: .buffered, defer: false)
        window.title = "ESO MoltenVK Patcher"; window.center()
        let view = NSView(frame: window.contentView!.bounds); window.contentView = view
        title.font = .systemFont(ofSize: 24, weight: .semibold); title.frame = NSRect(x: 28, y: 330, width: 540, height: 34); view.addSubview(title)
        let subtitle = NSTextField(wrappingLabelWithString: "Select an ESO installation. The patch is applied only when the exact client build is recognized.")
        subtitle.frame = NSRect(x: 28, y: 282, width: 564, height: 40); view.addSubview(subtitle)
        status.isEditable = false; status.backgroundColor = .textBackgroundColor; status.frame = NSRect(x: 28, y: 96, width: 564, height: 170); view.addSubview(status)
        let choose = button("Choose ESO…", #selector(chooseESO)); choose.frame = NSRect(x: 28, y: 42, width: 120, height: 32); view.addSubview(choose)
        for (index, item) in [("Check", #selector(check)), ("Install", #selector(install)), ("Repair", #selector(repair)), ("Remove", #selector(remove))].enumerated() {
            let b = button(item.0, item.1); b.frame = NSRect(x: 164 + index * 102, y: 42, width: 90, height: 32); view.addSubview(b)
        }
        window.makeKeyAndOrderFront(nil); NSApp.activate(ignoringOtherApps: true)
        if let candidate = engine.candidates().first { selected = candidate; update("Found ESO at \(candidate.path)\n\nUse Check to verify this installation.") } else { update("Choose eso.app or the ESO Launcher.app to begin.") }
    }
    private func button(_ title: String, _ action: Selector) -> NSButton { let button = NSButton(title: title, target: self, action: action); button.bezelStyle = .rounded; return button }
    @objc private func chooseESO() { let panel = NSOpenPanel(); panel.canChooseFiles = false; panel.canChooseDirectories = true; panel.allowsMultipleSelection = false; if panel.runModal() == .OK, let url = panel.url { do { selected = try engine.resolve(url); update("Selected: \(selected!.path)") } catch { present(error) } } }
    @objc private func check() { run { try self.engine.check(try self.required()) } }
    @objc private func install() { run { try self.engine.install(try self.required()) } }
    @objc private func repair() { run { try self.engine.repair(try self.required()) } }
    @objc private func remove() { run { try self.engine.remove(try self.required()) } }
    private func required() throws -> URL { guard let selected else { throw PatcherError.message("Choose an ESO installation first.") }; return selected }
    private func run(_ block: () throws -> String) { do { update(try block()) } catch { present(error) } }
    private func update(_ text: String) { status.string = text }
    private func present(_ error: Error) { update(error.localizedDescription); NSSound.beep() }
}
