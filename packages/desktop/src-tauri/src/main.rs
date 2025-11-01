// Prevents additional console window on Windows in release, DO NOT REMOVE!!
#![cfg_attr(not(debug_assertions), windows_subsystem = "windows")]

use serde::{Deserialize, Serialize};
use std::process::{Command, Child, Stdio};
use std::sync::{Arc, Mutex};
use std::thread;
use std::time::Duration;

#[derive(Debug, Serialize, Deserialize)]
struct ServerStatus {
    running: bool,
    port: u16,
    pid: Option<u32>,
}

#[derive(Debug, Serialize, Deserialize)]
struct ChatMessage {
    content: String,
    tool_calls: Option<Vec<ToolCall>>,
}

#[derive(Debug, Serialize, Deserialize)]
struct ToolCall {
    name: String,
    arguments: serde_json::Value,
}

struct ServerState {
    process: Option<Child>,
    port: u16,
}

#[tauri::command]
async fn start_server(state: tauri::State<'_, Arc<Mutex<ServerState>>>) -> Result<String, String> {
    // Check if server is already running
    {
        let server_state = state.lock().map_err(|e| format!("Failed to lock state: {}", e))?;
        if server_state.process.is_some() {
            return Err("Server is already running".to_string());
        }
    }

    // Find the kolosal-code directory
    let kolosal_path = std::env::current_dir()
        .map_err(|e| format!("Failed to get current directory: {}", e))?
        .parent()
        .and_then(|p| p.to_str())
        .ok_or("Failed to find kolosal-code directory")?
        .to_string();

    println!("Starting Kolosal server in: {}", kolosal_path);

    // Start the CLI server
    let child = Command::new("npm")
        .args(&[
            "start",
            "--",
            "--server-only",
            "--api-port",
            "38080",
            "--no_ui_output"
        ])
        .current_dir(&kolosal_path)
        .stdout(Stdio::piped())
        .stderr(Stdio::piped())
        .spawn()
        .map_err(|e| format!("Failed to start server: {}", e))?;

    let pid = child.id();
    
    // Store the process in the state
    {
        let mut server_state = state.lock().map_err(|e| format!("Failed to lock state: {}", e))?;
        server_state.process = Some(child);
    }

    // Wait a moment for server to start
    thread::sleep(Duration::from_secs(3));

    // Check if server is responsive
    if check_server_health().await {
        Ok(format!("Server started successfully (PID: {})", pid))
    } else {
        // If server is not responsive, kill it and return error
        {
            let mut server_state = state.lock().map_err(|e| format!("Failed to lock state: {}", e))?;
            if let Some(mut process) = server_state.process.take() {
                let _ = process.kill();
            }
        }
        Err("Server failed to start properly".to_string())
    }
}

#[tauri::command]
async fn stop_server(state: tauri::State<'_, Arc<Mutex<ServerState>>>) -> Result<String, String> {
    let mut server_state = state.lock().map_err(|e| format!("Failed to lock state: {}", e))?;
    
    if let Some(mut process) = server_state.process.take() {
        match process.kill() {
            Ok(_) => {
                // Wait for process to actually stop
                let _ = process.wait();
                Ok("Server stopped successfully".to_string())
            }
            Err(e) => Err(format!("Failed to stop server: {}", e)),
        }
    } else {
        Err("Server is not running".to_string())
    }
}

#[tauri::command]
async fn check_server_status(state: tauri::State<'_, Arc<Mutex<ServerState>>>) -> Result<ServerStatus, String> {
    let has_process = {
        let server_state = state.lock().map_err(|e| format!("Failed to lock state: {}", e))?;
        server_state.process.is_some()
    };

    let running = if has_process {
        check_server_health().await
    } else {
        false
    };

    let pid = {
        let server_state = state.lock().map_err(|e| format!("Failed to lock state: {}", e))?;
        server_state.process.as_ref().map(|p| p.id())
    };

    Ok(ServerStatus {
        running,
        port: 38080,
        pid,
    })
}

#[tauri::command]
async fn send_message(message: String) -> Result<ChatMessage, String> {
    let client = reqwest::Client::new();
    
    let request_body = serde_json::json!({
        "input": message,
        "stream": false
    });

    let response = client
        .post("http://127.0.0.1:38080/v1/generate")
        .json(&request_body)
        .send()
        .await
        .map_err(|e| format!("Failed to send request: {}", e))?;

    if !response.status().is_success() {
        return Err(format!("Server returned error: {}", response.status()));
    }

    let response_json: serde_json::Value = response
        .json()
        .await
        .map_err(|e| format!("Failed to parse response: {}", e))?;

    let content = response_json
        .get("output")
        .and_then(|v| v.as_str())
        .unwrap_or("No response")
        .to_string();

    // Extract tool calls if present
    let tool_calls = response_json
        .get("messages")
        .and_then(|messages| messages.as_array())
        .and_then(|arr| {
            let mut calls = Vec::new();
            for msg in arr {
                if let Some(_tool_call) = msg.get("type").and_then(|t| t.as_str()).filter(|&t| t == "tool_call") {
                    if let (Some(name), Some(args)) = (
                        msg.get("name").and_then(|n| n.as_str()),
                        msg.get("arguments")
                    ) {
                        calls.push(ToolCall {
                            name: name.to_string(),
                            arguments: args.clone(),
                        });
                    }
                }
            }
            if !calls.is_empty() {
                Some(calls)
            } else {
                None
            }
        });

    Ok(ChatMessage {
        content,
        tool_calls,
    })
}

async fn check_server_health() -> bool {
    let client = reqwest::Client::new();
    
    match client
        .get("http://127.0.0.1:38080/healthz")
        .timeout(Duration::from_secs(2))
        .send()
        .await
    {
        Ok(response) => response.status().is_success(),
        Err(_) => false,
    }
}

fn main() {
    let server_state = Arc::new(Mutex::new(ServerState {
        process: None,
        port: 38080,
    }));

    tauri::Builder::default()
        .manage(server_state)
        .invoke_handler(tauri::generate_handler![
            start_server,
            stop_server,
            check_server_status,
            send_message
        ])
        .run(tauri::generate_context!())
        .expect("error while running tauri application");
}