// Launcher Web UI Application
(function() {
    'use strict';

    let token = localStorage.getItem('launcher_token') || '';
    let refreshInterval = null;

    // Initialize
    document.addEventListener('DOMContentLoaded', function() {
        if (CONTROL_ENABLED) {
            document.getElementById('token-form').classList.remove('hidden');
        }

        document.getElementById('auto-refresh').addEventListener('change', function(e) {
            if (e.target.checked) {
                startAutoRefresh();
            } else {
                stopAutoRefresh();
            }
        });

        fetchStatus();
        startAutoRefresh();
    });

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

    async function fetchStatus() {
        const errorDiv = document.getElementById('error');
        const contentDiv = document.getElementById('content');
        const statusIndicator = document.getElementById('status-indicator');

        try {
            const headers = {};
            if (token) {
                headers['Authorization'] = 'Bearer ' + token;
            }

            const resp = await fetch(API_URL + '/api/v2/launcher/status', { headers });

            if (resp.status === 401) {
                errorDiv.textContent = 'Unauthorized. Please enter a valid read token.';
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
            if (data.allRunning && !data.anyCriticalFailed) {
                statusIndicator.textContent = 'All Running';
                statusIndicator.className = 'status-indicator status-healthy';
            } else if (data.anyCriticalFailed) {
                statusIndicator.textContent = 'Critical Failed';
                statusIndicator.className = 'status-indicator status-unhealthy';
            } else {
                statusIndicator.textContent = 'Some Not Running';
                statusIndicator.className = 'status-indicator status-unhealthy';
            }

        } catch (e) {
            errorDiv.textContent = 'Error loading status: ' + e.message;
            errorDiv.style.display = 'block';
            statusIndicator.textContent = 'Error';
            statusIndicator.className = 'status-indicator status-unhealthy';
        }
    }

    function renderProcesses(data) {
        const contentDiv = document.getElementById('content');

        let html = '<table>';
        html += '<thead><tr>';
        html += '<th>Process</th>';
        html += '<th>State</th>';
        html += '<th>PID</th>';
        html += '<th>Group</th>';
        html += '<th>Restarts</th>';
        if (CONTROL_ENABLED) {
            html += '<th>Actions</th>';
        }
        html += '</tr></thead>';
        html += '<tbody>';

        for (const proc of data.processes) {
            html += '<tr>';

            // Name with badges
            html += '<td>' + escapeHtml(proc.name);
            if (proc.critical) {
                html += '<span class="critical-badge">critical</span>';
            }
            if (proc.skip) {
                html += '<span class="skip-badge">skip</span>';
            }
            if (proc.oneshot) {
                html += '<span class="oneshot-badge">oneshot</span>';
            }
            html += '</td>';

            // State
            const stateClass = 'state-' + proc.state.toLowerCase();
            html += '<td><span class="state ' + stateClass + '">' + escapeHtml(proc.state) + '</span>';
            if (proc.lastError) {
                html += '<br><small style="color:#721c24">' + escapeHtml(proc.lastError) + '</small>';
            }
            html += '</td>';

            // PID
            html += '<td>' + (proc.pid > 0 ? proc.pid : '-') + '</td>';

            // Group
            html += '<td>' + escapeHtml(proc.group || '-') + '</td>';

            // Restarts
            html += '<td>' + proc.restartCount + '</td>';

            // Actions
            if (CONTROL_ENABLED) {
                html += '<td>';
                if (!proc.skip) {
                    if (proc.state === 'Running') {
                        html += '<button class="btn btn-restart" onclick="restartProcess(\'' + escapeHtml(proc.name) + '\')">Restart</button>';
                        html += '<button class="btn btn-stop" onclick="stopProcess(\'' + escapeHtml(proc.name) + '\')">Stop</button>';
                    } else if (proc.state === 'Stopped' || proc.state === 'Failed') {
                        html += '<button class="btn btn-start" onclick="startProcess(\'' + escapeHtml(proc.name) + '\')">Start</button>';
                    } else if (proc.state === 'Completed' && proc.oneshot) {
                        html += '<button class="btn btn-restart" onclick="restartProcess(\'' + escapeHtml(proc.name) + '\')">Re-run</button>';
                    } else {
                        html += '<span style="color:#666">-</span>';
                    }
                } else {
                    html += '<span style="color:#666">skipped</span>';
                }
                html += '</td>';
            }

            html += '</tr>';
        }

        html += '</tbody></table>';
        contentDiv.innerHTML = html;
    }

    function escapeHtml(text) {
        if (!text) return '';
        const div = document.createElement('div');
        div.textContent = text;
        return div.innerHTML;
    }

    async function controlAction(action, name) {
        const errorDiv = document.getElementById('error');

        if (!token) {
            errorDiv.textContent = 'Please enter a control token first.';
            errorDiv.style.display = 'block';
            document.getElementById('token').focus();
            return;
        }

        try {
            const resp = await fetch(API_URL + '/api/v2/launcher/process/' + encodeURIComponent(name) + '/' + action, {
                method: 'POST',
                headers: {
                    'Authorization': 'Bearer ' + token
                }
            });

            const data = await resp.json();

            if (resp.status === 401) {
                errorDiv.textContent = 'Unauthorized. Please check your control token.';
                errorDiv.style.display = 'block';
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

    // Global functions for onclick handlers
    window.restartProcess = function(name) {
        if (confirm('Restart process "' + name + '"?')) {
            controlAction('restart', name);
        }
    };

    window.stopProcess = function(name) {
        if (confirm('Stop process "' + name + '"?')) {
            controlAction('stop', name);
        }
    };

    window.startProcess = function(name) {
        controlAction('start', name);
    };

    window.saveToken = function() {
        token = document.getElementById('token').value;
        localStorage.setItem('launcher_token', token);
        document.getElementById('token').value = '';
        fetchStatus();
    };

    window.clearToken = function() {
        token = '';
        localStorage.removeItem('launcher_token');
        document.getElementById('token').value = '';
    };

    window.fetchStatus = fetchStatus;

})();
