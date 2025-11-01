import { invoke } from '@tauri-apps/api/tauri';
import { appWindow } from '@tauri-apps/api/window';

class KolosalDesktop {
  constructor() {
    this.messages = [];
    this.serverRunning = false;
    this.init();
  }

  async init() {
    this.setupEventListeners();
    await this.checkServerStatus();
    this.addMessage('system', 'Welcome to Kolosal Desktop! Click "Start Server" to begin.');
  }

  setupEventListeners() {
    const sendButton = document.getElementById('send-button');
    const messageInput = document.getElementById('message-input');
    const startServerBtn = document.getElementById('start-server');
    const stopServerBtn = document.getElementById('stop-server');

    sendButton.addEventListener('click', () => this.sendMessage());
    messageInput.addEventListener('keydown', (e) => {
      if (e.key === 'Enter' && !e.shiftKey) {
        e.preventDefault();
        this.sendMessage();
      }
    });

    startServerBtn.addEventListener('click', () => this.startServer());
    stopServerBtn.addEventListener('click', () => this.stopServer());
  }

  async checkServerStatus() {
    try {
      const status = await invoke('check_server_status');
      this.updateServerStatus(status.running);
    } catch (error) {
      console.error('Failed to check server status:', error);
      this.updateServerStatus(false);
    }
  }

  updateServerStatus(running) {
    this.serverRunning = running;
    const statusElement = document.getElementById('server-status');
    const startBtn = document.getElementById('start-server');
    const stopBtn = document.getElementById('stop-server');

    if (running) {
      statusElement.textContent = 'Server: Online';
      statusElement.style.color = 'green';
      startBtn.disabled = true;
      stopBtn.disabled = false;
    } else {
      statusElement.textContent = 'Server: Offline';
      statusElement.style.color = 'red';
      startBtn.disabled = false;
      stopBtn.disabled = true;
    }
  }

  async startServer() {
    try {
      this.addMessage('system', 'Starting Kolosal CLI server...');
      const result = await invoke('start_server');
      this.addMessage('system', `Server started: ${result}`);
      this.updateServerStatus(true);
    } catch (error) {
      this.addMessage('error', `Failed to start server: ${error}`);
      this.updateServerStatus(false);
    }
  }

  async stopServer() {
    try {
      this.addMessage('system', 'Stopping Kolosal CLI server...');
      const result = await invoke('stop_server');
      this.addMessage('system', `Server stopped: ${result}`);
      this.updateServerStatus(false);
    } catch (error) {
      this.addMessage('error', `Failed to stop server: ${error}`);
    }
  }

  async sendMessage() {
    const input = document.getElementById('message-input');
    const message = input.value.trim();
    
    if (!message) return;
    if (!this.serverRunning) {
      this.addMessage('error', 'Please start the server first.');
      return;
    }

    // Add user message
    this.addMessage('user', message);
    input.value = '';

    try {
      // Send message to server
      this.addMessage('assistant', 'Thinking...', true);
      const response = await invoke('send_message', { message });
      
      // Remove thinking message and add actual response
      this.removeLastMessage();
      this.addMessage('assistant', response.content);
      
      if (response.tool_calls) {
        response.tool_calls.forEach(tool_call => {
          this.addMessage('tool', `Called: ${tool_call.name}`);
        });
      }
    } catch (error) {
      this.removeLastMessage();
      this.addMessage('error', `Failed to send message: ${error}`);
    }
  }

  addMessage(type, content, isTemporary = false) {
    const messagesContainer = document.getElementById('messages');
    const messageElement = document.createElement('div');
    messageElement.className = `message ${type}`;
    if (isTemporary) messageElement.classList.add('temporary');
    
    let contentHtml = '';
    switch (type) {
      case 'user':
        contentHtml = `<strong>You:</strong> ${content}`;
        break;
      case 'assistant':
        contentHtml = `<strong>Kolosal:</strong> ${content.replace(/\n/g, '<br>')}`;
        break;
      case 'system':
        contentHtml = `<em>${content}</em>`;
        break;
      case 'error':
        contentHtml = `<span style="color: red;">${content}</span>`;
        break;
      case 'tool':
        contentHtml = `<span style="color: blue;">🔧 ${content}</span>`;
        break;
    }
    
    messageElement.innerHTML = contentHtml;
    messagesContainer.appendChild(messageElement);
    messagesContainer.scrollTop = messagesContainer.scrollHeight;
    
    this.messages.push({ type, content, element: messageElement });
  }

  removeLastMessage() {
    const messagesContainer = document.getElementById('messages');
    const lastMessage = messagesContainer.lastElementChild;
    if (lastMessage && lastMessage.classList.contains('temporary')) {
      messagesContainer.removeChild(lastMessage);
      this.messages.pop();
    }
  }
}

// Initialize the app when DOM is ready
document.addEventListener('DOMContentLoaded', () => {
  new KolosalDesktop();
});