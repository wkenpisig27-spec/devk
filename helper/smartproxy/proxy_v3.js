/**
 * PKO Smart Proxy v3 - DDoS Protection Proxy
 * 
 * Features:
 * 1. RSA Challenge-Response - Validates real game clients
 * 2. Connection Fingerprinting - Detects bot patterns from TCP behavior
 * 3. Adaptive Rate Limiting - Stricter limits during attacks
 * 4. PROXY Protocol v1 - Forwards real client IPs to GateServer
 * 5. Hot-Reload Config - Send SIGHUP to reload config without restart
 * 
 * Architecture:
 * Client → Proxy (RSA challenge) → Client responds → Proxy validates
 *                                                   ↓
 *                           Forward to GateServer with PROXY PROTOCOL header
 * 
 * Hot-Reload:
 *   kill -SIGHUP $(pgrep -f proxy_v3.js)
 *   OR: Create/modify config.json and it auto-reloads every 30s
 */

const net = require('net');
const fs = require('fs');
const path = require('path');
const http = require('http');

// =============================================================================
// CLI ARGUMENTS
// =============================================================================
// Usage: node proxy_v3.js [--config path/to/config.json]
// Example: node proxy_v3.js --config config.us.json

const args = process.argv.slice(2);
let cliConfigFile = null;
for (let i = 0; i < args.length; i++) {
    if (args[i] === '--config' && args[i + 1]) {
        cliConfigFile = args[i + 1];
    }
}

// =============================================================================
// CONFIGURATION
// =============================================================================

// Default configuration (can be overridden by config.json)
const DEFAULT_CONFIG = {
    // Region identification
    regionName: 'default',
    regionId: 'default',
    
    // Proxy server
    listenPort: 1973,
    listenHost: '0.0.0.0',
    
    // Health check HTTP endpoint (for monitoring all regional proxies)
    healthCheckPort: 8405,
    healthCheckEnabled: true,
    
    // Backend GateServer
    gateServerHost: '145.239.149.93',
    gateServerPort: 1973,
    
    // Anti-DDoS settings
    authTimeout: 8000,
    maxConnectionsPerIP: 15,
    connectionRateLimit: 20,
    connectionRateWindow: 10000,
    tempBanDuration: 300000,
    
    // Adaptive rate limiting
    attackThreshold: 500,
    attackModeMultiplier: 0.5,
    normalModeDelay: 60000,
    
    // Connection fingerprinting
    fingerprintEnabled: true,
    minResponseTimeMs: 50,
    maxResponseTimeMs: 15000,
    
    // Packet validation
    minPacketSize: 8,
    maxPacketSize: 65535,
    
    // RSA caching
    rsaRefreshInterval: 60000,
    rsaFetchCooldown: 5000,
    gateServerConnectTimeout: 15000,
    
    // Post-auth packet rate limiting
    packetRateLimit: 300,           // Max packets per second per IP
    packetRateWindow: 1000,         // Window in ms (1 second)
    
    // Login rate limiting
    loginRateLimit: 15,             // Max login attempts per IP
    loginRateWindow: 60000,         // Per 60 seconds
    
    // Pending login queue (prevents SyncCall exhaustion)
    maxPendingLoginsPerIP: 2,       // Max concurrent pending logins per IP
    maxPendingLoginsGlobal: 50,     // Max concurrent pending logins total
    pendingLoginTimeout: 30000,     // Auto-clear pending login after 30s
    
    // PKO command IDs for detection
    cmdLoginId: 431,                // CMD_CM_LOGIN (client → server)
    cmdLoginResponseId: 931,        // CMD_MC_LOGIN (server → client)
    
    // Logging (info, warn, error)
    logLevel: 'info',
    statsInterval: 30000,
    
    // Files
    blocklistFile: './blocklist.txt',
    configFile: './config.json',
};

// Active configuration (mutable, can be hot-reloaded)
let CONFIG = { ...DEFAULT_CONFIG };

// Apply CLI config file override
if (cliConfigFile) {
    CONFIG.configFile = cliConfigFile;
}

// Config field aliases: maps old/short names → canonical names
// This ensures config.json files using alternate field names still work
const CONFIG_ALIASES = {
    healthPort: 'healthCheckPort',
    maxConnPerIP: 'maxConnectionsPerIP',
};

function normalizeConfigKeys(obj) {
    const normalized = { ...obj };
    for (const [alias, canonical] of Object.entries(CONFIG_ALIASES)) {
        if (alias in normalized && !(canonical in normalized)) {
            normalized[canonical] = normalized[alias];
            delete normalized[alias];
        } else if (alias in normalized && canonical in normalized) {
            // Both present - canonical wins, remove alias
            delete normalized[alias];
        }
    }
    return normalized;
}

// Hot-reload configuration from config.json
function loadConfig() {
    const configPath = path.resolve(CONFIG.configFile || './config.json');
    try {
        if (fs.existsSync(configPath)) {
            const fileConfig = normalizeConfigKeys(JSON.parse(fs.readFileSync(configPath, 'utf8')));
            const oldConfig = { ...CONFIG };
            
            // Merge with defaults (only override what's in file)
            CONFIG = { ...DEFAULT_CONFIG, ...fileConfig };
            
            // Log what changed
            const changes = [];
            for (const key of Object.keys(fileConfig)) {
                if (oldConfig[key] !== CONFIG[key]) {
                    changes.push(`${key}: ${oldConfig[key]} → ${CONFIG[key]}`);
                }
            }
            
            if (changes.length > 0) {
                log('info', 'Config reloaded', { changes });
            }
            return true;
        }
    } catch (err) {
        log('error', 'Failed to load config.json', { error: err.message });
    }
    return false;
}

// SIGHUP handler for manual hot-reload
process.on('SIGHUP', () => {
    log('info', 'Received SIGHUP - reloading configuration...');
    loadConfig();
    loadBlocklist();
});

// Auto-reload config every 30 seconds if file changed
let lastConfigMtime = 0;
setInterval(() => {
    const configPath = path.resolve(CONFIG.configFile || './config.json');
    try {
        if (fs.existsSync(configPath)) {
            const stat = fs.statSync(configPath);
            if (stat.mtimeMs > lastConfigMtime) {
                lastConfigMtime = stat.mtimeMs;
                loadConfig();
            }
        }
    } catch (err) {
        // Ignore errors during auto-reload check
    }
}, 30000);

// =============================================================================
// STATE MANAGEMENT
// =============================================================================

let cachedRsaHandshake = null;
let rsaLastRefresh = 0;
let rsaFetchInProgress = null;
let rsaLastAttempt = 0;
let attackMode = false;
let lastAttackTime = 0;

const connections = new Map();
const ipTracker = new Map();
const tempBlacklist = new Map();
const blocklist = new Set();
const fingerprints = new Map();
const packetRates = new Map();       // IP → { count, lastReset }
const loginRates = new Map();        // IP → [timestamps]
const pendingLogins = new Map();     // IP → { count, timers[] }
let globalPendingLogins = 0;

const stats = {
    totalConnections: 0,
    activeConnections: 0,
    blockedByBlacklist: 0,
    blockedByRateLimit: 0,
    blockedByAuthTimeout: 0,
    blockedByInvalidPacket: 0,
    blockedByFingerprint: 0,
    blockedByPacketRate: 0,
    blockedByLoginRate: 0,
    blockedByPendingLogin: 0,
    authenticatedClients: 0,
    rsaRefreshes: 0,
    bytesForwarded: 0,
    connectionsPerSecond: 0,
    startTime: Date.now(),
};

// Track connections per second for attack detection
let connectionsThisSecond = 0;
setInterval(() => {
    stats.connectionsPerSecond = connectionsThisSecond;
    
    // Check if under attack
    if (connectionsThisSecond > CONFIG.attackThreshold) {
        if (!attackMode) {
            attackMode = true;
            log('warn', 'ATTACK MODE ACTIVATED', { cps: connectionsThisSecond });
        }
        lastAttackTime = Date.now();
    } else if (attackMode && Date.now() - lastAttackTime > CONFIG.normalModeDelay) {
        attackMode = false;
        log('info', 'Attack mode deactivated');
    }
    
    connectionsThisSecond = 0;
}, 1000);

// =============================================================================
// LOGGING
// =============================================================================

const LOG_LEVELS = { debug: 0, info: 1, warn: 2, error: 3 };

function log(level, message, data = {}) {
    if (LOG_LEVELS[level] >= LOG_LEVELS[CONFIG.logLevel]) {
        const timestamp = new Date().toISOString();
        const region = CONFIG.regionName || 'default';
        const dataStr = Object.keys(data).length ? ` ${JSON.stringify(data)}` : '';
        console.log(`[${timestamp}] [${region}] [${level.toUpperCase()}] ${message}${dataStr}`);
    }
}

// =============================================================================
// CONNECTION FINGERPRINTING
// =============================================================================

function getFingerprint(ip) {
    if (!fingerprints.has(ip)) {
        fingerprints.set(ip, {
            connections: 0,
            validResponses: 0,
            invalidResponses: 0,
            avgResponseTime: 0,
            fastResponses: 0,     // Suspiciously fast (< 50ms)
            lastSeen: Date.now(),
            score: 0,             // -100 to +100
        });
    }
    return fingerprints.get(ip);
}

function updateFingerprint(ip, responseTime, valid) {
    const fp = getFingerprint(ip);
    fp.connections++;
    fp.lastSeen = Date.now();
    
    if (valid) {
        fp.validResponses++;
        // Calculate rolling average response time
        fp.avgResponseTime = (fp.avgResponseTime * (fp.validResponses - 1) + responseTime) / fp.validResponses;
        fp.score = Math.min(100, fp.score + 10);  // Bigger reward for valid responses
    } else {
        fp.invalidResponses++;
        // No score penalty for invalid/missing responses - prevents legitimate players
        // from being blocked due to disconnects during handshake.
        // Bots are still caught by: zero-valid-response rule, fast-response detection, and rate limiting.
    }
    
    // Track suspiciously fast responses
    if (responseTime < CONFIG.minResponseTimeMs) {
        fp.fastResponses++;
        fp.score = Math.max(-100, fp.score - 3);  // Lighter penalty (was -5)
    }
}

function isSuspiciousFingerprint(ip) {
    const fp = getFingerprint(ip);
    
    // Suspicious patterns:
    // 1. Many connections, no valid responses (needs more attempts before blocking)
    if (fp.connections > 10 && fp.validResponses === 0) return true;
    
    // 2. Very negative score (only triggers from fast-response penalties now)
    if (fp.score <= -90) return true;
    
    // 3. High ratio of fast responses (bot-like)
    if (fp.connections > 5 && fp.fastResponses / fp.connections > 0.8) return true;
    
    return false;
}

// =============================================================================
// BLOCKLIST & RATE LIMITING (with adaptive mode)
// =============================================================================

function loadBlocklist() {
    try {
        if (fs.existsSync(CONFIG.blocklistFile)) {
            const content = fs.readFileSync(CONFIG.blocklistFile, 'utf8');
            const ips = content.split('\n')
                .map(line => line.trim())
                .filter(line => line && !line.startsWith('#'));
            
            blocklist.clear();
            ips.forEach(ip => blocklist.add(ip));
            log('info', `Loaded ${blocklist.size} IPs from blocklist`);
        }
    } catch (err) {
        log('error', 'Failed to load blocklist', { error: err.message });
    }
}

function isBlocked(ip) {
    if (blocklist.has(ip)) return 'blocklist';
    
    const tempBan = tempBlacklist.get(ip);
    if (tempBan && tempBan > Date.now()) return 'tempban';
    if (tempBan) tempBlacklist.delete(ip);
    
    // Fingerprint-based blocking
    if (CONFIG.fingerprintEnabled && isSuspiciousFingerprint(ip)) {
        return 'fingerprint';
    }
    
    return false;
}

function tempBan(ip, reason) {
    const duration = attackMode ? CONFIG.tempBanDuration * 2 : CONFIG.tempBanDuration;
    tempBlacklist.set(ip, Date.now() + duration);
}

function checkRateLimit(ip) {
    const now = Date.now();
    let tracker = ipTracker.get(ip);
    
    if (!tracker) {
        tracker = { connections: [], current: 0 };
        ipTracker.set(ip, tracker);
    }
    
    tracker.connections = tracker.connections.filter(
        time => now - time < CONFIG.connectionRateWindow
    );
    
    // Apply adaptive multiplier during attacks
    const rateLimit = attackMode 
        ? Math.floor(CONFIG.connectionRateLimit * CONFIG.attackModeMultiplier)
        : CONFIG.connectionRateLimit;
    
    const maxConcurrent = attackMode
        ? Math.floor(CONFIG.maxConnectionsPerIP * CONFIG.attackModeMultiplier)
        : CONFIG.maxConnectionsPerIP;
    
    if (tracker.connections.length >= rateLimit) {
        return { allowed: false, reason: 'rate_limit' };
    }
    
    if (tracker.current >= maxConcurrent) {
        return { allowed: false, reason: 'concurrent_limit' };
    }
    
    tracker.connections.push(now);
    tracker.current++;
    
    return { allowed: true };
}

function releaseConnection(ip) {
    const tracker = ipTracker.get(ip);
    if (tracker) {
        tracker.current = Math.max(0, tracker.current - 1);
    }
}

// =============================================================================
// POST-AUTH PROTECTIONS (Packet Rate, Login Rate, Pending Login Queue)
// =============================================================================

// Count packets in a TCP buffer using PKO packet framing (2-byte BE length header)
function countPacketsInBuffer(buffer) {
    let count = 0;
    let offset = 0;
    while (offset + 2 <= buffer.length) {
        const pktLen = buffer.readUInt16BE(offset);
        if (pktLen < 2) break;  // Invalid packet
        if (offset + pktLen > buffer.length) {
            count++;  // Partial packet counts as one
            break;
        }
        count++;
        offset += pktLen;
    }
    return Math.max(count, 1);  // At least 1 data event = 1 packet
}

// Extract command IDs from a TCP buffer containing PKO packets
// Uses a residual buffer to handle packets split across TCP segments
function extractCommandIdsBuffered(newData, residualBuffer) {
    const buffer = residualBuffer.length > 0
        ? Buffer.concat([residualBuffer, newData])
        : newData;
    
    const commands = [];
    let offset = 0;
    
    while (offset + 2 <= buffer.length) {
        const pktLen = buffer.readUInt16BE(offset);
        if (pktLen < 8) { offset += Math.max(pktLen, 2); continue; }  // Skip invalid
        if (offset + pktLen > buffer.length) break;  // Incomplete packet — keep as residual
        const commandId = buffer.readUInt16BE(offset + 6);
        commands.push(commandId);
        offset += pktLen;
    }
    
    // Return remaining unparsed bytes for next TCP segment
    const residual = offset < buffer.length ? buffer.slice(offset) : Buffer.alloc(0);
    return { commands, residual };
}

// Simple non-buffered version for one-off checks
function extractCommandIds(buffer) {
    const result = extractCommandIdsBuffered(buffer, Buffer.alloc(0));
    return result.commands;
}

// Check packet rate limit for an authenticated connection
// Returns true if allowed, false if rate exceeded
function checkPacketRate(ip, packetCount) {
    const now = Date.now();
    let tracker = packetRates.get(ip);
    
    if (!tracker || now - tracker.lastReset >= CONFIG.packetRateWindow) {
        tracker = { count: 0, lastReset: now };
        packetRates.set(ip, tracker);
    }
    
    tracker.count += packetCount;
    return tracker.count <= CONFIG.packetRateLimit;
}

// Check login rate limit for an IP
// Returns true if allowed, false if rate exceeded
function checkLoginRate(ip) {
    const now = Date.now();
    let timestamps = loginRates.get(ip);
    
    if (!timestamps) {
        timestamps = [];
        loginRates.set(ip, timestamps);
    }
    
    // Remove expired entries
    const cutoff = now - CONFIG.loginRateWindow;
    while (timestamps.length > 0 && timestamps[0] < cutoff) {
        timestamps.shift();
    }
    
    if (timestamps.length >= CONFIG.loginRateLimit) {
        return false;  // Rate exceeded
    }
    
    timestamps.push(now);
    return true;
}

// Try to acquire a pending login slot
// Returns true if slot acquired, false if queue full
function acquirePendingLogin(ip, connState) {
    if (globalPendingLogins >= CONFIG.maxPendingLoginsGlobal) {
        return false;  // Global queue full
    }
    
    let tracker = pendingLogins.get(ip);
    if (!tracker) {
        tracker = { count: 0 };
        pendingLogins.set(ip, tracker);
    }
    
    if (tracker.count >= CONFIG.maxPendingLoginsPerIP) {
        return false;  // Per-IP queue full
    }
    
    tracker.count++;
    globalPendingLogins++;
    
    // Track on the connection state for proper per-connection cleanup
    connState.pendingLoginCount = (connState.pendingLoginCount || 0) + 1;
    
    // Auto-expire after timeout (safety net for stuck logins)
    const timer = setTimeout(() => {
        releasePendingLogin(ip, connState);
    }, CONFIG.pendingLoginTimeout);
    connState.pendingLoginTimers = connState.pendingLoginTimers || [];
    connState.pendingLoginTimers.push(timer);
    
    return true;
}

// Release a pending login slot (called on login response or timeout)
function releasePendingLogin(ip, connState) {
    if (!connState || (connState.pendingLoginCount || 0) <= 0) return;
    
    connState.pendingLoginCount = Math.max(0, connState.pendingLoginCount - 1);
    globalPendingLogins = Math.max(0, globalPendingLogins - 1);
    
    // Update per-IP tracker
    const tracker = pendingLogins.get(ip);
    if (tracker) {
        tracker.count = Math.max(0, tracker.count - 1);
        if (tracker.count === 0) {
            pendingLogins.delete(ip);
        }
    }
    
    // Clear the oldest timer
    if (connState.pendingLoginTimers && connState.pendingLoginTimers.length > 0) {
        clearTimeout(connState.pendingLoginTimers.shift());
    }
}

// Release all pending logins for a specific connection (called on disconnect)
function releaseConnPendingLogins(ip, connState) {
    if (!connState) return;
    const count = connState.pendingLoginCount || 0;
    if (count <= 0) return;
    
    globalPendingLogins = Math.max(0, globalPendingLogins - count);
    connState.pendingLoginCount = 0;
    
    // Clear all timers for this connection
    if (connState.pendingLoginTimers) {
        connState.pendingLoginTimers.forEach(t => clearTimeout(t));
        connState.pendingLoginTimers = [];
    }
    
    // Update per-IP tracker
    const tracker = pendingLogins.get(ip);
    if (tracker) {
        tracker.count = Math.max(0, tracker.count - count);
        if (tracker.count === 0) {
            pendingLogins.delete(ip);
        }
    }
}

// =============================================================================
// RSA HANDSHAKE CACHING
// =============================================================================

async function refreshRsaHandshake() {
    return new Promise((resolve, reject) => {
        log('info', 'Fetching RSA handshake from GateServer...');
        
        const socket = net.createConnection({
            host: CONFIG.gateServerHost,
            port: CONFIG.gateServerPort,
            timeout: CONFIG.gateServerConnectTimeout,
        });
        
        let received = false;
        
        socket.on('connect', () => {
            const proxyHeader = `PROXY TCP4 127.0.0.1 ${CONFIG.gateServerHost} 12345 ${CONFIG.gateServerPort}\r\n`;
            socket.write(proxyHeader);
        });
        
        socket.on('data', (data) => {
            if (!received) {
                received = true;
                cachedRsaHandshake = Buffer.from(data);
                rsaLastRefresh = Date.now();
                stats.rsaRefreshes++;
                socket.destroy();
                log('info', 'RSA handshake cached', { length: data.length });
                resolve(cachedRsaHandshake);
            }
        });
        
        socket.on('timeout', () => {
            log('error', 'Timeout fetching RSA handshake');
            socket.destroy();
            reject(new Error('Timeout'));
        });
        
        socket.on('error', (err) => {
            log('error', 'Error fetching RSA handshake', { error: err.message });
            socket.destroy();
            reject(err);
        });
    });
}

async function getRsaHandshake() {
    if (cachedRsaHandshake && (Date.now() - rsaLastRefresh <= CONFIG.rsaRefreshInterval)) {
        return cachedRsaHandshake;
    }
    
    if (rsaFetchInProgress) {
        try {
            await rsaFetchInProgress;
            return cachedRsaHandshake;
        } catch (err) {
            // Fall through to retry
        }
    }
    
    const timeSinceLastAttempt = Date.now() - rsaLastAttempt;
    if (timeSinceLastAttempt < CONFIG.rsaFetchCooldown) {
        if (cachedRsaHandshake) {
            return cachedRsaHandshake;
        }
        throw new Error('RSA fetch cooldown active, no cached handshake available');
    }
    
    rsaLastAttempt = Date.now();
    
    rsaFetchInProgress = (async () => {
        try {
            await refreshRsaHandshake();
        } finally {
            rsaFetchInProgress = null;
        }
    })();
    
    try {
        await rsaFetchInProgress;
    } catch (err) {
        if (cachedRsaHandshake) {
            log('warn', 'Using stale RSA handshake');
        } else {
            throw err;
        }
    }
    
    return cachedRsaHandshake;
}

// =============================================================================
// PACKET PARSING
// =============================================================================

function parsePacketHeader(buffer) {
    if (buffer.length < 8) return null;
    
    // PKO packet format (Big Endian):
    // Bytes 0-1: total packet length (uint16 BE)
    // Bytes 2-5: session ID (uint32, RPC layer)
    // Bytes 6-7: command ID (uint16 BE)
    // Bytes 8+:  payload data
    //
    // Header size = len_offset(0) + len_size(2) + RPCMGR::__ses_size(4) = 6
    // Command ID written at GetDataAddr() = offset + m_head(6)
    const length = buffer.readUInt16BE(0);
    const sessionId = buffer.readUInt32BE(2);
    const commandId = buffer.readUInt16BE(6);
    
    return { length, sessionId, commandId };
}

function isValidClientResponse(buffer) {
    const header = parsePacketHeader(buffer);
    if (!header) return { valid: false, reason: 'too_short' };
    
    if (header.length < CONFIG.minPacketSize || header.length > CONFIG.maxPacketSize) {
        return { valid: false, reason: 'invalid_size', size: header.length };
    }
    
    // Validate the command ID (bytes 6-7) is in the valid client->server range
    // CMD_CM_BASE = 0, max client commands ~500
    // CMD_CM_RSA_HANDSHAKE_1 = 348
    if (header.commandId > 500) {
        return { valid: false, reason: 'invalid_command', commandId: header.commandId };
    }
    
    return { valid: true, commandId: header.commandId, length: header.length };
}

// =============================================================================
// CONNECTION HANDLER
// =============================================================================

async function handleConnection(clientSocket) {
    const clientIP = clientSocket.remoteAddress?.replace('::ffff:', '') || 'unknown';
    const clientPort = clientSocket.remotePort;
    const connId = `${clientIP}:${clientPort}`;
    
    stats.totalConnections++;
    stats.activeConnections++;
    connectionsThisSecond++;
    
    // Check blocklist + fingerprint
    const blockReason = isBlocked(clientIP);
    if (blockReason) {
        if (blockReason === 'fingerprint') stats.blockedByFingerprint++;
        else stats.blockedByBlacklist++;
        clientSocket.destroy();
        stats.activeConnections--;
        return;
    }
    
    // Check rate limit (adaptive during attacks)
    const rateCheck = checkRateLimit(clientIP);
    if (!rateCheck.allowed) {
        stats.blockedByRateLimit++;
        clientSocket.destroy();
        stats.activeConnections--;
        return;
    }
    
    // Connection state
    const state = {
        phase: 'handshake',
        gateSocket: null,
        authTimer: null,
        buffer: Buffer.alloc(0),
        handshakeSentTime: null,
        clientParseBuffer: Buffer.alloc(0),  // Residual buffer for client→gate CMD parsing
        gateParseBuffer: Buffer.alloc(0),    // Residual buffer for gate→client CMD parsing
        pendingLoginCount: 0,                // Per-connection pending login counter
        pendingLoginTimers: [],              // Per-connection login timeout timers
    };
    
    connections.set(clientSocket, state);
    let cleanedUp = false;
    
    function cleanup() {
        if (cleanedUp) return;
        cleanedUp = true;
        
        if (state.authTimer) {
            clearTimeout(state.authTimer);
            state.authTimer = null;
        }
        
        if (state.gateSocket) {
            state.gateSocket.destroy();
        }
        
        connections.delete(clientSocket);
        releaseConnection(clientIP);
        releaseConnPendingLogins(clientIP, state);
        stats.activeConnections--;
    }
    
    // Send RSA challenge
    try {
        const rsaPacket = await getRsaHandshake();
        clientSocket.write(rsaPacket);
        state.handshakeSentTime = Date.now();
    } catch (err) {
        log('error', 'Failed to get RSA handshake', { connId, error: err.message });
        clientSocket.destroy();
        cleanup();
        return;
    }
    
    // Set auth timeout
    state.authTimer = setTimeout(() => {
        if (state.phase === 'handshake') {
            stats.blockedByAuthTimeout++;
            const responseTime = Date.now() - state.handshakeSentTime;
            updateFingerprint(clientIP, responseTime, false);
            // Don't temp-ban on auth timeout - slow connections are not attacks
            // Only ban if fingerprint shows bot-like patterns (handled by fingerprinting)
            log('info', 'Auth timeout', { ip: clientIP, responseTime: `${responseTime}ms` });
            clientSocket.destroy();
        }
    }, CONFIG.authTimeout);
    
    // Handle client data
    clientSocket.on('data', async (data) => {
        if (state.phase === 'handshake') {
            state.buffer = Buffer.concat([state.buffer, data]);
            
            if (state.buffer.length < 4) return;
            
            const header = parsePacketHeader(state.buffer);
            if (!header || state.buffer.length < header.length) return;
            
            const responseTime = Date.now() - state.handshakeSentTime;
            const validation = isValidClientResponse(state.buffer);
            
            updateFingerprint(clientIP, responseTime, validation.valid);
            
            if (!validation.valid) {
                stats.blockedByInvalidPacket++;
                const rawHex = state.buffer.slice(0, Math.min(16, state.buffer.length)).toString('hex');
                log('info', 'Invalid packet', { ip: clientIP, reason: validation.reason, commandId: validation.commandId, size: validation.size, rawHex, bufLen: state.buffer.length });
                clientSocket.destroy();
                return;
            }
            
            // CLIENT AUTHENTICATED!
            clearTimeout(state.authTimer);
            state.authTimer = null;
            state.phase = 'authenticated';
            stats.authenticatedClients++;
            
            log('info', 'Authenticated', { connId, responseTime: `${responseTime}ms` });
            
            connectToGateServer(clientSocket, state, clientIP, clientPort);
            
        } else if (state.phase === 'forwarding' && state.gateSocket) {
            // === POST-AUTH PACKET RATE LIMITING ===
            const packetCount = countPacketsInBuffer(data);
            if (!checkPacketRate(clientIP, packetCount)) {
                stats.blockedByPacketRate++;
                log('warn', 'Packet rate exceeded', { ip: clientIP, count: packetRates.get(clientIP)?.count });
                tempBan(clientIP, 'packet_rate');
                clientSocket.destroy();
                return;
            }
            
            // === LOGIN RATE LIMITING & PENDING LOGIN QUEUE ===
            const parseResult = extractCommandIdsBuffered(data, state.clientParseBuffer);
            state.clientParseBuffer = parseResult.residual;
            for (const cmdId of parseResult.commands) {
                if (cmdId === CONFIG.cmdLoginId) {
                    // Check login rate
                    if (!checkLoginRate(clientIP)) {
                        stats.blockedByLoginRate++;
                        log('warn', 'Login rate exceeded', { ip: clientIP });
                        tempBan(clientIP, 'login_rate');
                        clientSocket.destroy();
                        return;
                    }
                    // Check pending login queue
                    if (!acquirePendingLogin(clientIP, state)) {
                        stats.blockedByPendingLogin++;
                        log('warn', 'Pending login queue full', { 
                            ip: clientIP, 
                            perIP: pendingLogins.get(clientIP)?.count || 0,
                            global: globalPendingLogins 
                        });
                        clientSocket.destroy();
                        return;
                    }
                }
            }
            
            state.gateSocket.write(data);
            stats.bytesForwarded += data.length;
        } else if (state.phase === 'authenticated') {
            // Buffer data arriving while gate TCP connect is in progress
            state.postAuthBuffer = state.postAuthBuffer || [];
            state.postAuthBuffer.push(data);
        }
    });
    
    clientSocket.on('close', cleanup);
    clientSocket.on('error', cleanup);
}

function connectToGateServer(clientSocket, state, clientIP, clientPort) {
    state.gateSocket = net.createConnection({
        host: CONFIG.gateServerHost,
        port: CONFIG.gateServerPort,
        timeout: CONFIG.gateServerConnectTimeout,
    });
    
    state.gateSocket.on('connect', () => {
        // Send PROXY protocol v1 header
        const proxyHeader = `PROXY TCP4 ${clientIP} ${CONFIG.gateServerHost} ${clientPort} ${CONFIG.gateServerPort}\r\n`;
        state.gateSocket.write(proxyHeader);
        
        // Forward buffered RSA response
        if (state.buffer.length > 0) {
            state.gateSocket.write(state.buffer);
            stats.bytesForwarded += state.buffer.length;
        }
        
        state.phase = 'forwarding';
        
        // Flush any data that arrived during the authenticated→forwarding transition
        if (state.postAuthBuffer && state.postAuthBuffer.length > 0) {
            for (const buf of state.postAuthBuffer) {
                state.gateSocket.write(buf);
                stats.bytesForwarded += buf.length;
            }
            state.postAuthBuffer = null;
        }
    });
    
    state.gateSocket.on('data', (data) => {
        if (!clientSocket.destroyed) {
            // === DETECT LOGIN RESPONSE → Release pending login slot ===
            const parseResult = extractCommandIdsBuffered(data, state.gateParseBuffer);
            state.gateParseBuffer = parseResult.residual;
            for (const cmdId of parseResult.commands) {
                if (cmdId === CONFIG.cmdLoginResponseId) {
                    releasePendingLogin(clientIP, state);
                }
            }
            
            clientSocket.write(data);
            stats.bytesForwarded += data.length;
        }
    });
    
    state.gateSocket.on('close', () => {
        releaseConnPendingLogins(clientIP, state);  // Immediately free this connection's pending login slots
        if (!clientSocket.destroyed) {
            clientSocket.destroy();
        }
    });
    
    state.gateSocket.on('error', () => {
        releaseConnPendingLogins(clientIP, state);  // Immediately free this connection's pending login slots
        if (!clientSocket.destroyed) {
            clientSocket.destroy();
        }
    });
    
    state.gateSocket.on('timeout', () => {
        state.gateSocket.destroy();
        if (!clientSocket.destroyed) {
            clientSocket.destroy();
        }
    });
}

// =============================================================================
// STATISTICS
// =============================================================================

function printStats() {
    const uptime = Math.floor((Date.now() - stats.startTime) / 1000);
    const hours = Math.floor(uptime / 3600);
    const mins = Math.floor((uptime % 3600) / 60);
    const region = CONFIG.regionName || 'default';
    
    console.log(`\n=== PKO Smart Proxy v3 [${region}] ===`);
    console.log(`Uptime: ${hours}h ${mins}m | Mode: ${attackMode ? 'ATTACK' : 'NORMAL'}`);
    console.log(`Active: ${stats.activeConnections} | Total: ${stats.totalConnections} | Auth: ${stats.authenticatedClients}`);
    console.log(`Rate: ${stats.connectionsPerSecond}/s | Blocked: ${stats.blockedByBlacklist + stats.blockedByRateLimit + stats.blockedByAuthTimeout + stats.blockedByInvalidPacket + stats.blockedByFingerprint + stats.blockedByPacketRate + stats.blockedByLoginRate + stats.blockedByPendingLogin}`);
    console.log(`Post-auth: PktRate=${stats.blockedByPacketRate} LoginRate=${stats.blockedByLoginRate} PendingQ=${stats.blockedByPendingLogin} | PendingLogins: ${globalPendingLogins}`);
    console.log(`Forwarded: ${(stats.bytesForwarded / 1024 / 1024).toFixed(2)} MB`);
    console.log(`================================\n`);
}

// =============================================================================
// HEALTH CHECK HTTP ENDPOINT
// =============================================================================

function getHealthData() {
    const uptime = Math.floor((Date.now() - stats.startTime) / 1000);
    return {
        status: 'ok',
        region: CONFIG.regionName,
        regionId: CONFIG.regionId,
        uptime,
        mode: attackMode ? 'attack' : 'normal',
        backend: `${CONFIG.gateServerHost}:${CONFIG.gateServerPort}`,
        rsaCached: !!cachedRsaHandshake,
        rsaAge: cachedRsaHandshake ? Math.floor((Date.now() - rsaLastRefresh) / 1000) : null,
        connections: {
            active: stats.activeConnections,
            total: stats.totalConnections,
            authenticated: stats.authenticatedClients,
            perSecond: stats.connectionsPerSecond,
        },
        blocked: {
            blocklist: stats.blockedByBlacklist,
            rateLimit: stats.blockedByRateLimit,
            authTimeout: stats.blockedByAuthTimeout,
            invalidPacket: stats.blockedByInvalidPacket,
            fingerprint: stats.blockedByFingerprint,
            packetRate: stats.blockedByPacketRate,
            loginRate: stats.blockedByLoginRate,
            pendingLogin: stats.blockedByPendingLogin,
            total: stats.blockedByBlacklist + stats.blockedByRateLimit + stats.blockedByAuthTimeout + stats.blockedByInvalidPacket + stats.blockedByFingerprint + stats.blockedByPacketRate + stats.blockedByLoginRate + stats.blockedByPendingLogin,
        },
        traffic: {
            bytesForwarded: stats.bytesForwarded,
            mbForwarded: parseFloat((stats.bytesForwarded / 1024 / 1024).toFixed(2)),
        },
        pendingLogins: {
            global: globalPendingLogins,
            maxGlobal: CONFIG.maxPendingLoginsGlobal,
            maxPerIP: CONFIG.maxPendingLoginsPerIP,
        },
        trackedIPs: ipTracker.size,
        tempBans: tempBlacklist.size,
        fingerprints: fingerprints.size,
        timestamp: new Date().toISOString(),
    };
}

function startHealthCheckServer() {
    if (!CONFIG.healthCheckEnabled) return;
    
    const server = http.createServer((req, res) => {
        const url = new URL(req.url, `http://${req.headers.host || 'localhost'}`);
        const pathname = url.pathname;
        
        if (pathname === '/health' || pathname === '/') {
            const health = getHealthData();
            res.writeHead(200, { 'Content-Type': 'application/json' });
            res.end(JSON.stringify(health, null, 2));
        } else if (pathname === '/health/simple') {
            // Simple endpoint for uptime monitors (just returns 200 if running)
            res.writeHead(200, { 'Content-Type': 'text/plain' });
            res.end(`OK ${CONFIG.regionName}`);
        } else if (pathname === '/check-ip') {
            // Check fingerprint/status for a specific IP
            // Usage: curl http://127.0.0.1:8080/check-ip?ip=1.2.3.4
            const ip = url.searchParams.get('ip');
            if (!ip) {
                res.writeHead(400, { 'Content-Type': 'application/json' });
                res.end(JSON.stringify({ error: 'Missing ?ip= parameter' }));
                return;
            }
            const fp = fingerprints.get(ip) || null;
            const tempBan = tempBlacklist.get(ip) || null;
            const tracker = ipTracker.get(ip) || null;
            const blocked = isBlocked(ip);
            res.writeHead(200, { 'Content-Type': 'application/json' });
            res.end(JSON.stringify({
                ip,
                blocked: blocked || false,
                fingerprint: fp,
                tempBan: tempBan ? { expires: new Date(tempBan).toISOString(), remainingMs: Math.max(0, tempBan - Date.now()) } : null,
                rateTracker: tracker ? { activeConnections: tracker.current, recentConnections: tracker.connections.length } : null,
            }, null, 2));
        } else if (pathname === '/reset-ip') {
            // Reset fingerprint, temp ban, and rate tracking for a specific IP
            // Usage: curl http://127.0.0.1:8080/reset-ip?ip=1.2.3.4
            const ip = url.searchParams.get('ip');
            if (!ip) {
                res.writeHead(400, { 'Content-Type': 'application/json' });
                res.end(JSON.stringify({ error: 'Missing ?ip= parameter' }));
                return;
            }
            const cleared = [];
            if (fingerprints.has(ip)) { fingerprints.delete(ip); cleared.push('fingerprint'); }
            if (tempBlacklist.has(ip)) { tempBlacklist.delete(ip); cleared.push('tempBan'); }
            if (ipTracker.has(ip)) { ipTracker.delete(ip); cleared.push('rateTracker'); }
            log('info', `Manual IP reset via admin endpoint`, { ip, cleared });
            res.writeHead(200, { 'Content-Type': 'application/json' });
            res.end(JSON.stringify({
                ip,
                cleared,
                message: cleared.length > 0 ? `Cleared ${cleared.join(', ')} for ${ip}` : `No data found for ${ip}`,
            }, null, 2));
        } else {
            res.writeHead(404);
            res.end('Not found');
        }
    });
    
    server.on('error', (err) => {
        log('error', 'Health check server error', { error: err.message });
    });
    
    server.listen(CONFIG.healthCheckPort, '127.0.0.1', () => {
        log('info', `Health check endpoint: http://127.0.0.1:${CONFIG.healthCheckPort}/health (localhost only)`);
    });
}

// =============================================================================
// MAIN
// =============================================================================

async function main() {
    console.log('PKO Smart Proxy v3 starting...');
    
    // Load config from file if exists
    if (loadConfig()) {
        console.log('Loaded configuration from config.json');
    } else {
        console.log('Using default configuration (no config.json found)');
    }
    
    console.log(`Region: ${CONFIG.regionName} (${CONFIG.regionId})`);
    console.log(`Backend: ${CONFIG.gateServerHost}:${CONFIG.gateServerPort}`);
    console.log(`Max connections per IP: ${CONFIG.maxConnectionsPerIP} (${Math.floor(CONFIG.maxConnectionsPerIP * CONFIG.attackModeMultiplier)} during attack)`);
    
    loadBlocklist();
    
    // Pre-fetch RSA handshake
    console.log('Fetching RSA handshake...');
    let rsaFetched = false;
    for (let attempt = 1; attempt <= 5; attempt++) {
        try {
            await refreshRsaHandshake();
            console.log('RSA handshake cached successfully');
            rsaFetched = true;
            break;
        } catch (err) {
            console.error(`RSA fetch attempt ${attempt}/5 failed: ${err.message}`);
            if (attempt < 5) {
                const delay = attempt * 2000;
                console.log(`Retrying in ${delay/1000}s...`);
                await new Promise(r => setTimeout(r, delay));
            }
        }
    }
    
    if (!rsaFetched) {
        console.error('WARNING: Could not fetch RSA handshake!');
    }
    
    // Start health check HTTP server
    startHealthCheckServer();
    
    // Start game proxy server
    const server = net.createServer(handleConnection);
    
    server.on('error', (err) => {
        log('error', 'Server error', { error: err.message });
    });
    
    server.listen(CONFIG.listenPort, CONFIG.listenHost, () => {
        console.log(`Proxy listening on ${CONFIG.listenHost}:${CONFIG.listenPort}`);
        console.log(`Auth timeout: ${CONFIG.authTimeout}ms | Rate limit: ${CONFIG.connectionRateLimit}/${CONFIG.connectionRateWindow/1000}s`);
        console.log(`Attack threshold: ${CONFIG.attackThreshold}/s | Fingerprinting: ${CONFIG.fingerprintEnabled ? 'ON' : 'OFF'}`);
    });
    
    // Periodic tasks
    setInterval(printStats, CONFIG.statsInterval);
    setInterval(() => refreshRsaHandshake().catch(() => {}), CONFIG.rsaRefreshInterval);
    setInterval(loadBlocklist, 60000);
    
    // Clean old fingerprints, rate tracking, and stale map entries
    setInterval(() => {
        const now = Date.now();
        const maxAge = 30 * 60 * 1000;
        let cleaned = { fingerprints: 0, ipTracker: 0, tempBlacklist: 0, loginRates: 0, packetRates: 0 };
        
        const maxNegativeAge = 1 * 60 * 1000; // 1 minute expiry for negative-score entries
        for (const [ip, fp] of fingerprints) {
            const age = now - fp.lastSeen;
            if ((age > maxAge && fp.score >= 0) || age > maxNegativeAge) {
                fingerprints.delete(ip);
                cleaned.fingerprints++;
            }
        }
        // Clean stale ipTracker entries (no active connections, no recent history)
        for (const [ip, tracker] of ipTracker) {
            // Remove timestamps older than the rate window
            tracker.connections = tracker.connections.filter(
                time => now - time < CONFIG.connectionRateWindow
            );
            // Delete entry if no active connections and no recent history
            if (tracker.current <= 0 && tracker.connections.length === 0) {
                ipTracker.delete(ip);
                cleaned.ipTracker++;
            }
        }
        // Clean expired temp bans
        for (const [ip, expiry] of tempBlacklist) {
            if (expiry <= now) {
                tempBlacklist.delete(ip);
                cleaned.tempBlacklist++;
            }
        }
        // Clean old login rate entries
        for (const [ip, timestamps] of loginRates) {
            const cutoff = now - CONFIG.loginRateWindow;
            while (timestamps.length > 0 && timestamps[0] < cutoff) {
                timestamps.shift();
            }
            if (timestamps.length === 0) {
                loginRates.delete(ip);
                cleaned.loginRates++;
            }
        }
        // Clean old packet rate entries
        for (const [ip, tracker] of packetRates) {
            if (now - tracker.lastReset > 10000) {
                packetRates.delete(ip);
                cleaned.packetRates++;
            }
        }
        
        const total = Object.values(cleaned).reduce((a, b) => a + b, 0);
        if (total > 0) {
            log('info', 'Cleanup cycle', cleaned);
        }
    }, 10 * 60 * 1000);
}

main().catch(err => {
    console.error('Fatal error:', err);
    process.exit(1);
});
