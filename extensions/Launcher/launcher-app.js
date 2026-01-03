// Launcher Web UI Application
(function() {
    'use strict';

    const CONTROL_SESSION_TIMEOUT = 5 * 60 * 1000;  // 5 minutes
    const HEARTBEAT_INTERVAL = 30 * 1000;  // 30 seconds
    const TIMER_UPDATE_INTERVAL = 1000;  // 1 second

    let readToken = localStorage.getItem('launcher_read_token') || '';
    let controlToken = localStorage.getItem('launcher_control_token') || '';
    let refreshInterval = null;
    let heartbeatInterval = null;
    let timerInterval = null;
    let authMode = null;  // 'read' or 'control'
    let hasControl = false;

    // Initialize
    document.addEventListener('DOMContentLoaded', function() {
        document.getElementById('auto-refresh').addEventListener('change', function(e) {
            if (e.target.checked) {
                startAutoRefresh();
            } else {
                stopAutoRefresh();
            }
        });

        // Check if control session is expired
        if (controlToken) {
            if (isControlSessionExpired()) {
                console.log('Control session expired, releasing');
                releaseControlSilent();
            } else {
                startHeartbeat();
            }
        }

        // Show Take Control button if control is enabled
        if (CONTROL_ENABLED) {
            updateControlUI();
        }

        // If read auth is required and no token, show auth modal
        if (READ_AUTH_REQUIRED && !readToken) {
            showReadAuth();
        } else {
            fetchStatus();
            startAutoRefresh();
        }
    });

    // Heartbeat functions
    function isControlSessionExpired() {
        const lastHeartbeat = localStorage.getItem('launcher_control_heartbeat');
        if (!lastHeartbeat) return true;
        const elapsed = Date.now() - parseInt(lastHeartbeat, 10);
        return elapsed > CONTROL_SESSION_TIMEOUT;
    }

    function updateHeartbeat() {
        localStorage.setItem('launcher_control_heartbeat', Date.now().toString());
    }

    function startHeartbeat() {
        stopHeartbeat();
        updateHeartbeat();
        heartbeatInterval = setInterval(updateHeartbeat, HEARTBEAT_INTERVAL);
        // Start timer display update
        updateTimerDisplay();
        timerInterval = setInterval(checkSessionAndUpdateTimer, TIMER_UPDATE_INTERVAL);
    }

    function stopHeartbeat() {
        if (heartbeatInterval) {
            clearInterval(heartbeatInterval);
            heartbeatInterval = null;
        }
        if (timerInterval) {
            clearInterval(timerInterval);
            timerInterval = null;
        }
        localStorage.removeItem('launcher_control_heartbeat');
        const timerEl = document.getElementById('session-timer');
        if (timerEl) timerEl.textContent = '';
    }

    function checkSessionAndUpdateTimer() {
        if (isControlSessionExpired()) {
            console.log('Control session expired');
            releaseControl();
            return;
        }
        updateTimerDisplay();
    }

    function updateTimerDisplay() {
        const timerEl = document.getElementById('session-timer');
        if (!timerEl) return;

        const lastHeartbeat = localStorage.getItem('launcher_control_heartbeat');
        if (!lastHeartbeat) {
            timerEl.textContent = '';
            return;
        }

        const elapsed = Date.now() - parseInt(lastHeartbeat, 10);
        const remaining = Math.max(0, CONTROL_SESSION_TIMEOUT - elapsed);
        const minutes = Math.floor(remaining / 60000);
        const seconds = Math.floor((remaining % 60000) / 1000);
        timerEl.textContent = minutes + ':' + seconds.toString().padStart(2, '0');
    }

    function releaseControlSilent() {
        controlToken = '';
        localStorage.removeItem('launcher_control_token');
        stopHeartbeat();
        hasControl = false;
    }

    function startAutoRefresh() {
        stopAutoRefresh();
        refreshInterval = setInterval(fetchStatus, 5000);
    }

    function stopAutoRefresh() {
        if (refreshInterval) {
            clearInterval(refreshInterval);
            refreshInterval = null;
        }
    }

    function updateControlUI() {
        const controlArea = document.getElementById('control-area');
        const takeControlBtn = document.getElementById('take-control-btn');
        const controlModeDiv = document.getElementById('control-mode');

        // Show control area if control is enabled
        if (CONTROL_ENABLED) {
            controlArea.classList.remove('hidden');
        }

        if (controlToken) {
            hasControl = true;
            takeControlBtn.classList.add('hidden');
            controlModeDiv.classList.remove('hidden');
        } else {
            hasControl = false;
            takeControlBtn.classList.remove('hidden');
            controlModeDiv.classList.add('hidden');
        }
    }

    async function fetchStatus() {
        const errorDiv = document.getElementById('error');
        const contentDiv = document.getElementById('content');
        const statusDot = document.querySelector('#status-indicator .status-dot');
        const statusText = document.getElementById('status-text');

        try {
            const headers = {};
            if (readToken) {
                headers['Authorization'] = 'Bearer ' + readToken;
            }

            const resp = await fetch(API_URL + '/api/v2/launcher/status', { headers });

            if (resp.status === 401) {
                if (READ_AUTH_REQUIRED) {
                    showReadAuth();
                    return;
                }
                errorDiv.textContent = 'Unauthorized';
                errorDiv.style.display = 'block';
                return;
            }

            if (!resp.ok) {
                throw new Error('HTTP ' + resp.status);
            }

            const data = await resp.json();
            errorDiv.style.display = 'none';
            renderProcesses(data);

            // Update status indicator
            statusDot.className = 'status-dot';
            if (data.allRunning && !data.anyCriticalFailed) {
                statusDot.classList.add('ok');
                statusText.textContent = 'All Running';
            } else if (data.anyCriticalFailed) {
                statusDot.classList.add('fail');
                statusText.textContent = 'Critical Failed';
            } else {
                statusDot.classList.add('warn');
                statusText.textContent = 'Partial';
            }

        } catch (e) {
            errorDiv.textContent = 'Error loading status: ' + e.message;
            errorDiv.style.display = 'block';
            statusDot.className = 'status-dot fail';
            statusText.textContent = 'Error';
        }
    }

    function renderProcesses(data) {
        const contentDiv = document.getElementById('content');

        let html = '<table class="process-table">';
        html += '<thead><tr>';
        html += '<th>Process</th>';
        html += '<th>State</th>';
        html += '<th>PID</th>';
        html += '<th>Uptime</th>';
        html += '<th>Group</th>';
        html += '<th>Restarts</th>';
        if (hasControl) {
            html += '<th>Actions</th>';
        }
        html += '</tr></thead>';
        html += '<tbody>';

        for (const proc of data.processes) {
            html += '<tr>';

            // Name with badges
            html += '<td><div class="process-name"><span class="name">' + escapeHtml(proc.name) + '</span>';
            if (proc.skip) {
                html += '<span class="badge badge-muted">skip</span>';
            }
            if (proc.oneshot) {
                html += '<span class="badge badge-purple">oneshot</span>';
            }
            html += '</div></td>';

            // State
            const stateClass = 'state-' + proc.state.toLowerCase();
            html += '<td><span class="state-badge ' + stateClass + '">';
            html += '<span class="status-dot ' + getStateDotClass(proc.state) + '"></span>';
            html += escapeHtml(proc.state) + '</span>';
            if (proc.lastError) {
                html += '<div style="font-size:11px;color:var(--accent-red);margin-top:2px">' + escapeHtml(proc.lastError) + '</div>';
            }
            html += '</td>';

            // PID
            html += '<td class="pid">' + (proc.pid > 0 ? proc.pid : '-') + '</td>';

            // Uptime
            html += '<td class="uptime">' + formatUptime(proc.uptime) + '</td>';

            // Group
            html += '<td>' + escapeHtml(proc.group || '-') + '</td>';

            // Restarts
            html += '<td>' + proc.restartCount + '</td>';

            // Actions
            if (hasControl) {
                html += '<td class="actions">';
                if (!proc.skip) {
                    const state = proc.state.toLowerCase();
                    if (state === 'running') {
                        html += '<button class="btn btn-warning btn-sm" onclick="restartProcess(\'' + escapeHtml(proc.name) + '\')" title="Restart">&#8635;</button>';
                        html += '<button class="btn btn-danger btn-sm" onclick="stopProcess(\'' + escapeHtml(proc.name) + '\')" title="Stop">&#9632;</button>';
                    } else if (state === 'stopped' || state === 'failed') {
                        html += '<button class="btn btn-success btn-sm" onclick="startProcess(\'' + escapeHtml(proc.name) + '\')" title="Start">&#9654;</button>';
                    } else if (state === 'completed' && proc.oneshot) {
                        html += '<button class="btn btn-warning btn-sm" onclick="restartProcess(\'' + escapeHtml(proc.name) + '\')" title="Re-run">&#8635;</button>';
                    } else {
                        html += '<span style="color:var(--text-muted)">-</span>';
                    }
                } else {
                    html += '<span style="color:var(--text-muted)">-</span>';
                }
                html += '</td>';
            }

            html += '</tr>';
        }

        html += '</tbody></table>';
        contentDiv.innerHTML = html;
    }

    function getStateDotClass(state) {
        switch(state.toLowerCase()) {
            case 'running':
            case 'completed':
                return 'ok';
            case 'failed':
                return 'fail';
            case 'starting':
            case 'stopping':
                return 'warn';
            default:
                return '';
        }
    }

    function escapeHtml(text) {
        if (!text) return '';
        const div = document.createElement('div');
        div.textContent = text;
        return div.innerHTML;
    }

    function formatUptime(seconds) {
        if (seconds === undefined || seconds === null) return '-';
        if (seconds < 60) return seconds + 's';
        if (seconds < 3600) {
            const m = Math.floor(seconds / 60);
            const s = seconds % 60;
            return m + 'm ' + s + 's';
        }
        const h = Math.floor(seconds / 3600);
        const m = Math.floor((seconds % 3600) / 60);
        if (h < 24) {
            return h + 'h ' + m + 'm';
        }
        const d = Math.floor(h / 24);
        const hh = h % 24;
        return d + 'd ' + hh + 'h';
    }

    async function controlAction(action, name) {
        const errorDiv = document.getElementById('error');

        if (!controlToken) {
            errorDiv.textContent = 'Please take control first.';
            errorDiv.style.display = 'block';
            return;
        }

        // Update heartbeat on user action
        updateHeartbeat();

        try {
            const resp = await fetch(API_URL + '/api/v2/launcher/process/' + encodeURIComponent(name) + '/' + action, {
                method: 'POST',
                headers: {
                    'Authorization': 'Bearer ' + controlToken
                }
            });

            const data = await resp.json();

            if (resp.status === 401) {
                errorDiv.textContent = 'Control token invalid. Please re-authenticate.';
                errorDiv.style.display = 'block';
                releaseControl();
                return;
            }

            if (resp.status === 403) {
                errorDiv.textContent = 'Forbidden: ' + (data.message || 'Control operations disabled');
                errorDiv.style.display = 'block';
                return;
            }

            if (!data.success) {
                errorDiv.textContent = 'Error: ' + (data.error || 'Unknown error');
                errorDiv.style.display = 'block';
            } else {
                errorDiv.style.display = 'none';
            }

            // Refresh status after action
            setTimeout(fetchStatus, 500);

        } catch (e) {
            errorDiv.textContent = 'Error: ' + e.message;
            errorDiv.style.display = 'block';
        }
    }

    // Auth modal functions
    function showAuthModal(title, mode) {
        authMode = mode;
        document.getElementById('modal-title').textContent = title;
        document.getElementById('auth-token').value = '';
        document.getElementById('modal-error').classList.add('hidden');
        document.getElementById('auth-modal').classList.remove('hidden');
        document.getElementById('auth-token').focus();
    }

    function showReadAuth() {
        showAuthModal('Authorization Required', 'read');
    }

    function showControlAuth() {
        showAuthModal('Take Control', 'control');
    }

    function hideAuthModal() {
        document.getElementById('auth-modal').classList.add('hidden');
        authMode = null;
    }

    async function submitAuth() {
        const token = document.getElementById('auth-token').value;
        const modalError = document.getElementById('modal-error');

        if (!token) {
            modalError.textContent = 'Please enter a token';
            modalError.classList.remove('hidden');
            return;
        }

        // Validate token by making a request
        try {
            const endpoint = authMode === 'control'
                ? '/api/v2/launcher/auth'  // Use auth endpoint for control validation
                : '/api/v2/launcher/status';

            const resp = await fetch(API_URL + endpoint, {
                headers: { 'Authorization': 'Bearer ' + token }
            });

            if (resp.status === 401) {
                modalError.textContent = 'Invalid token';
                modalError.classList.remove('hidden');
                return;
            }

            if (resp.status === 403) {
                modalError.textContent = 'Control operations disabled';
                modalError.classList.remove('hidden');
                return;
            }

            // Token is valid
            if (authMode === 'read') {
                readToken = token;
                localStorage.setItem('launcher_read_token', token);
                hideAuthModal();
                fetchStatus();
                startAutoRefresh();
            } else if (authMode === 'control') {
                controlToken = token;
                localStorage.setItem('launcher_control_token', token);
                startHeartbeat();  // Start heartbeat for control session
                hideAuthModal();
                updateControlUI();
                fetchStatus();  // Re-render with actions
            }

        } catch (e) {
            modalError.textContent = 'Connection error: ' + e.message;
            modalError.classList.remove('hidden');
        }
    }

    function releaseControl() {
        controlToken = '';
        localStorage.removeItem('launcher_control_token');
        stopHeartbeat();
        hasControl = false;
        updateControlUI();
        fetchStatus();  // Re-render without actions
    }

    // Custom confirm dialog
    let confirmCallback = null;

    function showConfirm(title, message, buttonClass, callback) {
        document.getElementById('confirm-title').textContent = title;
        document.getElementById('confirm-message').textContent = message;
        const okBtn = document.getElementById('confirm-ok-btn');
        okBtn.className = 'btn ' + (buttonClass || 'btn-primary');
        confirmCallback = callback;
        document.getElementById('confirm-modal').classList.remove('hidden');
    }

    function hideConfirm() {
        document.getElementById('confirm-modal').classList.add('hidden');
        confirmCallback = null;
    }

    function confirmOk() {
        const cb = confirmCallback;
        hideConfirm();
        if (cb) cb();
    }

    function confirmCancel() {
        hideConfirm();
    }

    // Global functions for onclick handlers
    window.restartProcess = function(name) {
        showConfirm('Restart Process', 'Restart process "' + name + '"?', 'btn-warning', function() {
            controlAction('restart', name);
        });
    };

    window.stopProcess = function(name) {
        showConfirm('Stop Process', 'Stop process "' + name + '"?', 'btn-danger', function() {
            controlAction('stop', name);
        });
    };

    window.startProcess = function(name) {
        controlAction('start', name);
    };

    window.showControlAuth = showControlAuth;
    window.hideAuthModal = hideAuthModal;
    window.submitAuth = submitAuth;
    window.releaseControl = releaseControl;
    window.fetchStatus = fetchStatus;
    window.confirmOk = confirmOk;
    window.confirmCancel = confirmCancel;

})();
