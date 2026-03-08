// =============================================================================
// MATER — BUBO OS Build Watchdog
// mater/src/main.rs
//
// ALTER: KIMISHIMA KUNIHIKO — No Alter
// "I can't fight like you. But I can make sure you don't fight blind."
//
// Mater has no flashy power. He watches. He hashes. He signs. He never lies.
// He is the most trustworthy character in any movie ever made.
//
// Features:
//   - SHA-256 integrity hashing of every source file before build
//   - Ed25519 cryptographic signing of every build report
//   - Append-only tamper-evident audit log (JSON Lines)
//   - Build runner with structured error parsing
//   - Colored terminal output with honest verdicts
//
// Built for BUBO OS. Built for Landon. NO MAS DISADVANTAGED.
// Copyright (c) 2025 Nathan Pankuch — MIT License
// =============================================================================

use std::fs::{self, OpenOptions};
use std::io::Write;
use std::path::{Path, PathBuf};
use std::process::{Command, Stdio};
use std::time::{SystemTime, UNIX_EPOCH};

use chrono::Local;
use colored::*;
use ed25519_dalek::{Signature, Signer, SigningKey};
use rand::rngs::OsRng;
use serde::{Deserialize, Serialize};
use sha2::{Digest, Sha256};
use walkdir::WalkDir;

// =============================================================================
// Data structures
// =============================================================================

#[derive(Serialize, Deserialize, Clone, Debug)]
struct SourceFile {
    path: String,
    sha256: String,
    size_bytes: u64,
}

#[derive(Serialize, Deserialize, Clone, Debug)]
struct BuildError {
    file: String,
    line: Option<u32>,
    column: Option<u32>,
    kind: String,
    message: String,
}

#[derive(Serialize, Deserialize, Clone, Debug)]
struct BuildReport {
    timestamp: String,
    unix_ts: u64,
    build_success: bool,
    exit_code: i32,
    source_files_hashed: usize,
    errors: Vec<BuildError>,
    warnings: Vec<BuildError>,
    iso_produced: bool,
    iso_size_bytes: Option<u64>,
    integrity_snapshot: Vec<SourceFile>,
    signature: Option<String>,
    verifying_key: Option<String>,
}

#[derive(Serialize, Deserialize)]
struct AuditEntry {
    entry_type: String,
    timestamp: String,
    unix_ts: u64,
    data: serde_json::Value,
}

// =============================================================================
// Key management
// =============================================================================

fn load_or_generate_key(key_path: &Path) -> SigningKey {
    if key_path.exists() {
        let bytes = fs::read(key_path).expect("Failed to read signing key");
        if bytes.len() == 32 {
            let arr: [u8; 32] = bytes.try_into().expect("Key wrong size");
            return SigningKey::from_bytes(&arr);
        }
    }
    let key = SigningKey::generate(&mut OsRng);
    fs::write(key_path, key.to_bytes()).expect("Failed to write signing key");
    println!("{}", "  [MATER] Generated new Ed25519 signing key.".yellow());
    key
}

// =============================================================================
// Source integrity
// =============================================================================

fn hash_source_files(root: &Path) -> Vec<SourceFile> {
    let mut files = Vec::new();
    let extensions = ["c", "h", "asm", "rs"];

    for entry in WalkDir::new(root)
        .into_iter()
        .filter_map(|e| e.ok())
        .filter(|e| e.file_type().is_file())
    {
        let path = entry.path();
        let path_str = path.to_string_lossy();
        if path_str.contains("/.git/")
            || path_str.contains("/target/")
            || path_str.ends_with(".o")
            || path_str.ends_with(".iso")
        {
            continue;
        }
        let ext = path.extension().and_then(|e| e.to_str()).unwrap_or("");
        if !extensions.contains(&ext) {
            continue;
        }
        if let Ok(content) = fs::read(path) {
            let mut hasher = Sha256::new();
            hasher.update(&content);
            let hash = hex::encode(hasher.finalize());
            files.push(SourceFile {
                path: path.strip_prefix(root).unwrap_or(path)
                    .to_string_lossy().to_string(),
                sha256: hash,
                size_bytes: content.len() as u64,
            });
        }
    }
    files.sort_by(|a, b| a.path.cmp(&b.path));
    files
}

// =============================================================================
// Build runner
// =============================================================================

fn run_build(repo_root: &Path) -> (bool, i32, Vec<BuildError>, Vec<BuildError>) {
    let mut errors = Vec::new();
    let mut warnings = Vec::new();

    println!("{}", "\n  [MATER] Running make... I'll tell you exactly what happens.\n"
        .cyan().bold());

    let output = match Command::new("make")
        .arg("-j4")
        .current_dir(repo_root)
        .stdout(Stdio::piped())
        .stderr(Stdio::piped())
        .output()
    {
        Ok(o) => o,
        Err(e) => {
            eprintln!("  [MATER] Failed to run make: {}", e);
            return (false, -1, errors, warnings);
        }
    };

    let exit_code = output.status.code().unwrap_or(-1);
    let stderr = String::from_utf8_lossy(&output.stderr);

    for line in stderr.lines() {
        if let Some(parsed) = parse_gcc_line(line) {
            if parsed.kind == "error" || parsed.kind == "linker" {
                errors.push(parsed);
            } else {
                warnings.push(parsed);
            }
        }
    }
    (exit_code == 0, exit_code, errors, warnings)
}

fn parse_gcc_line(line: &str) -> Option<BuildError> {
    if line.contains("undefined reference") || line.starts_with("ld:") {
        return Some(BuildError {
            file: extract_file(line),
            line: None,
            column: None,
            kind: "linker".to_string(),
            message: line.trim().to_string(),
        });
    }
    if line.contains(": error:") {
        let parts: Vec<&str> = line.splitn(5, ':').collect();
        if parts.len() >= 4 {
            return Some(BuildError {
                file: parts[0].trim().to_string(),
                line: parts[1].trim().parse().ok(),
                column: parts[2].trim().parse().ok(),
                kind: "error".to_string(),
                message: parts.get(4).unwrap_or(&"").trim().to_string(),
            });
        }
    }
    if line.contains(": warning:") {
        let parts: Vec<&str> = line.splitn(5, ':').collect();
        if parts.len() >= 4 {
            return Some(BuildError {
                file: parts[0].trim().to_string(),
                line: parts[1].trim().parse().ok(),
                column: parts[2].trim().parse().ok(),
                kind: "warning".to_string(),
                message: parts.get(4).unwrap_or(&"").trim().to_string(),
            });
        }
    }
    None
}

fn extract_file(line: &str) -> String {
    for ext in &[".c:", ".o:", ".h:"] {
        if let Some(pos) = line.find(ext) {
            let start = line[..pos].rfind(' ').map(|i| i + 1).unwrap_or(0);
            return line[start..pos + ext.len() - 1].to_string();
        }
    }
    "unknown".to_string()
}

// =============================================================================
// Audit log
// =============================================================================

fn append_audit_log(log_path: &Path, entry: &AuditEntry) {
    let mut file = OpenOptions::new()
        .create(true).append(true)
        .open(log_path).expect("Failed to open audit log");
    let line = serde_json::to_string(entry).expect("Serialize failed");
    writeln!(file, "{}", line).expect("Write failed");
}

// =============================================================================
// Signing
// =============================================================================

fn sign_report(report: &BuildReport, key: &SigningKey) -> (String, String) {
    let json = serde_json::to_string(report).expect("Serialize failed");
    let sig: Signature = key.sign(json.as_bytes());
    (hex::encode(sig.to_bytes()), hex::encode(key.verifying_key().to_bytes()))
}

// =============================================================================
// Output
// =============================================================================

fn print_banner() {
    println!("{}", "\n  ╔╦╗╔═╗╔╦╗╔═╗╦═╗".yellow().bold());
    println!("{}", "  ║║║╠═╣ ║ ║╣ ╠╦╝".yellow().bold());
    println!("{}", "  ╩ ╩╩ ╩ ╩ ╚═╝╩╚═".yellow().bold());
    println!("{}", "\n  BUBO OS Build Watchdog — Kimishima Alter — No Alter, Just Truth".white().bold());
    println!("{}", "  \"I can't fight like you. But I can make sure you don't fight blind.\"".white().italic());
    println!("{}", "  ─────────────────────────────────────────────────────────────────\n".white());
}

fn print_report(report: &BuildReport) {
    println!("{}", "  ─────────────────────────────────────────────────────────────────".white());
    println!("  {} {}", "Timestamp:".bold(), report.timestamp);
    println!("  {} {}", "Source files hashed:".bold(), report.source_files_hashed);
    println!("  {} {}", "Errors:".bold(),
        if report.errors.is_empty() { "0".green().to_string() }
        else { report.errors.len().to_string().red().to_string() });
    println!("  {} {}", "Warnings:".bold(), report.warnings.len().to_string().yellow());
    println!("  {} {}", "ISO produced:".bold(),
        if report.iso_produced { "YES".green().to_string() }
        else { "NO".red().to_string() });

    if let (Some(sig), Some(vk)) = (&report.signature, &report.verifying_key) {
        println!("  {} {}...{}", "Report signed:".bold(), &sig[..16], &sig[sig.len()-8..]);
        println!("  {} {}...{}", "Verify key:".bold(), &vk[..16], &vk[vk.len()-8..]);
    }

    if !report.errors.is_empty() {
        println!("\n{}", "  ERRORS:".red().bold());
        for e in &report.errors {
            println!("    {} {}:{} — {}",
                "✗".red(), e.file.yellow(),
                e.line.map(|l| l.to_string()).unwrap_or_default(),
                e.message);
        }
    }

    println!("{}", "\n  ─────────────────────────────────────────────────────────────────".white());

    if report.build_success && report.iso_produced {
        println!("{}", "\n  [MATER] BUILD CLEAN. ISO CONFIRMED. READY TO FLASH.\n".green().bold());
        println!("{}", "  \"That's what I call trustworthy. Just like me.\"\n".green().italic());
    } else if report.build_success {
        println!("{}", "\n  [MATER] Build passed but no ISO found. Check grub-mkrescue.\n".yellow().bold());
    } else {
        println!("{}", "\n  [MATER] BUILD FAILED. I found the problems. Fix them and run me again.\n".red().bold());
        println!("{}", "  \"I don't sugarcoat. That's not what friends do.\"\n".red().italic());
    }
}

// =============================================================================
// Main
// =============================================================================

fn main() {
    print_banner();

    let exe_path = std::env::current_exe().expect("Cannot find exe path");
    let repo_root = std::env::var("BUBO_REPO")
        .map(PathBuf::from)
        .unwrap_or_else(|_| {
            exe_path.parent().and_then(|p| p.parent())
                .and_then(|p| p.parent()).and_then(|p| p.parent())
                .unwrap_or(Path::new("..")).to_path_buf()
        });

    println!("  {} {}", "Watching:".bold(), repo_root.display().to_string().cyan());

    let key_path = repo_root.join("mater").join(".mater_signing_key");
    let signing_key = load_or_generate_key(&key_path);
    let log_path = repo_root.join("mater").join("audit.jsonl");

    println!("{}", "\n  [MATER] Hashing source files...".cyan());
    let snapshot = hash_source_files(&repo_root);
    println!("  {} {} files hashed.", "✓".green(), snapshot.len().to_string().bold());

    let now_ts = SystemTime::now().duration_since(UNIX_EPOCH).unwrap().as_secs();
    let now_str = Local::now().format("%Y-%m-%d %H:%M:%S").to_string();

    append_audit_log(&log_path, &AuditEntry {
        entry_type: "integrity_snapshot".to_string(),
        timestamp: now_str.clone(),
        unix_ts: now_ts,
        data: serde_json::json!({ "file_count": snapshot.len() }),
    });

    let (success, exit_code, errors, warnings) = run_build(&repo_root);

    let iso_path = repo_root.join("bubo.iso");
    let iso_produced = iso_path.exists();
    let iso_size = if iso_produced { fs::metadata(&iso_path).ok().map(|m| m.len()) } else { None };

    if iso_produced {
        println!("\n  {} bubo.iso confirmed: {} bytes", "✓".green().bold(),
            iso_size.unwrap_or(0).to_string().bold());
    } else {
        println!("\n  {} bubo.iso not found.", "✗".red().bold());
    }

    let mut report = BuildReport {
        timestamp: now_str.clone(), unix_ts: now_ts,
        build_success: success, exit_code,
        source_files_hashed: snapshot.len(),
        errors, warnings, iso_produced, iso_size_bytes: iso_size,
        integrity_snapshot: snapshot,
        signature: None, verifying_key: None,
    };

    let (sig, vk) = sign_report(&report, &signing_key);
    report.signature = Some(sig);
    report.verifying_key = Some(vk);

    append_audit_log(&log_path, &AuditEntry {
        entry_type: "build_report".to_string(),
        timestamp: now_str,
        unix_ts: now_ts,
        data: serde_json::to_value(&report).unwrap(),
    });

    let report_path = repo_root.join("mater").join("latest_report.json");
    fs::write(&report_path, serde_json::to_string_pretty(&report).unwrap())
        .expect("Failed to write report");

    println!("  {} Report saved: {}", "✓".green(), report_path.display().to_string().cyan());
    println!("  {} Audit log: {}", "✓".green(), log_path.display().to_string().cyan());

    print_report(&report);
}
