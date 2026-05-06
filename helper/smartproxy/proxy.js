/**
 * PKO Smart Proxy v2 - Protocol-Aware DDoS Protection with Connection Pool
 * 
 * CRITICAL FIX: Don't connect to GateServer until AFTER client proves it's real.
 * 
 * Architecture:
 * 1. Pre-fetch RSA handshake from GateServer once
 * 2. Send cached handshake to ALL clients immediately
 * 3. Only clients that respond correctly get forwarded to GateServer
 * 4. Bots that just connect and wait get killed at the proxy level
 * 
 * This protects GateServer from connection exhaustion attacks.
 */

const net = require('net');
const fs = require('fs');

// =============================================================================
// CONFIGURATION
// =============================================================================

const CONFIG = {
    // Proxy listens on this port (players connect here)
    listenPort: 1973,
    listenHost: '0.0.0.0',
    
    // Backend GateServer
    gateServerHost: '145.239.149.93',
    gateServerPort: 1973,
    
    // Anti-DDoS settings
    authTimeout: 8000,            // 8 seconds to respond to RSA (kills slow bots)
    maxConnectionsPerIP: 5,       // Max concurrent connections per IP
    connectionRateLimit: 10,      // Max new connections per IP per window
    connectionRateWindow: 10000,  // Rate limit window (10 seconds)
    tempBanDuration: 300000,      // 5 minutes temp ban for bad behavior
    
    // Packet validation
    minPacketSize: 4,             // Minimum valid packet size
    maxPacketSize: 65535,         // Maximum packet size
    
    // Connection pool for GateServer
    rsaRefreshInterval: 60000,    // Refresh RSA handshake every 60 seconds
    gateServerConnectTimeout: 15000, // Timeout for GateServer connections (15 seconds)
    
    // Logging
    logLevel: 'debug',            // 'debug', 'info', 'warn', 'error'
    statsInterval: 30000,         // Print stats every 30 seconds
    
    // Blocklist file
    blocklistFile: './blocklist.txt',
};

// =============================================================================
// PKO PACKET PROTOCOL
// =============================================================================

// PKO uses BIG ENDIAN (network byte order) for packet headers
// Bytes 0-1: Packet length (uint16 BE)
// Bytes 2-3: Packet ID (uint16 BE)

const CMD_CM_ROLEBASE = 300;
const PKO_PACKETS = {
    // RSA handshake (client responds with same packet ID as server sends)
    MC_RSA_HANDSHAKE_1: 943,      // Server -> Client RSA challenge
    CM_RSA_HANDSHAKE_1: 348,      // Client -> Server RSA response (EXPECTED FIRST PACKET)
};

// =============================================================================
// STATE MANAGEMENT
// =============================================================================

// Cached RSA handshake from GateServer (sent to all clients)
let cachedRsaHandshake = null;
let rsaLastRefresh = 0;

// Active connections
const connections = new Map();

// IP tracking for rate limiting
const ipTracker = new Map();

// Temporary blacklist (auto-expires)
const tempBlacklist = new Map();

// Permanent blocklist
const blocklist = new Set();

// Statistics
const stats = {
    totalConnections: 0,
    activeConnections: 0,
    blockedByBlacklist: 0,
    blockedByRateLimit: 0,
    blockedByAuthTimeout: 0,
    blockedByInvalidPacket: 0,
    authenticatedClients: 0,
    rsaRefreshes: 0,
    bytesForwarded: 0,
    startTime: Date.now(),
};

// =============================================================================
// LOGGING
// =============================================================================

const LOG_LEVELS = { debug: 0, info: 1, warn: 2, error: 3 };

function log(level, message, data = {}) {
    if (LOG_LEVELS[level] >= LOG_LEVELS[CONFIG.logLevel]) {
        const timestamp = new Date().toISOString();
        const dataStr = Object.keys(data).length ? ` ${JSON.stringify(data)}` : '';
        console.log(`[${timestamp}] [${level.toUpperCase()}] ${message}${dataStr}`);
    }
}

// =============================================================================
// BLOCKLIST MANAGEMENT
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
    
    return false;
}

function tempBan(ip, reason) {
    tempBlacklist.set(ip, Date.now() + CONFIG.tempBanDuration);
    log('debug', `Temp banned IP`, { ip, reason, duration: `${CONFIG.tempBanDuration/1000}s` });
}

// =============================================================================
// RATE LIMITING
// =============================================================================

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
    
    if (tracker.connections.length >= CONFIG.connectionRateLimit) {
        return { allowed: false, reason: 'rate_limit' };
    }
    
    if (tracker.current >= CONFIG.maxConnectionsPerIP) {
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
// RSA HANDSHAKE CACHING
// =============================================================================

/**
 * Fetch RSA handshake from GateServer and cache it.
 * This is the packet that starts the connection protocol.
 */
async function refreshRsaHandshake() {
    return new Promise((resolve, reject) => {
        log('info', 'Fetching fresh RSA handshake from GateServer...');
        
        const socket = net.createConnection({
            host: CONFIG.gateServerHost,
            port: CONFIG.gateServerPort,
            timeout: CONFIG.gateServerConnectTimeout,
        });
        
        let received = false;
        
        socket.on('connect', () => {
            log('debug', 'Connected to GateServer for RSA fetch');
            
            // Send PROXY protocol v1 header (required when GateServer has ProxyProtocol = 1)
            // Use a dummy client IP for the initial fetch
            const proxyHeader = `PROXY TCP4 127.0.0.1 ${CONFIG.gateServerHost} 12345 ${CONFIG.gateServerPort}\r\n`;
            socket.write(proxyHeader);
            log('debug', 'Sent PROXY protocol header for RSA fetch');
        });
        
        socket.on('data', (data) => {
            if (!received) {
                received = true;
                
                // Parse and log the handshake packet
                if (data.length >= 4) {
                    const len = data.readUInt16BE(0);
                    const id = data.readUInt16BE(2);
                    log('info', 'RSA handshake captured', {
                        length: data.length,
                        packetLen: len,
                        packetId: id,
                        packetIdHex: `0x${id.toString(16)}`,
                        hex: data.slice(0, Math.min(20, data.length)).toString('hex'),
                    });
                }
                
                cachedRsaHandshake = Buffer.from(data);
                rsaLastRefresh = Date.now();
                stats.rsaRefreshes++;
                
                socket.destroy();
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

/**
 * Get cached RSA handshake, refreshing if needed
 */
async function getRsaHandshake() {
    // Refresh if expired or not cached
    if (!cachedRsaHandshake || (Date.now() - rsaLastRefresh > CONFIG.rsaRefreshInterval)) {
        try {
            await refreshRsaHandshake();
        } catch (err) {
            if (cachedRsaHandshake) {
                log('warn', 'Using stale RSA handshake');
            } else {
                throw err;
            }
        }
    }
    return cachedRsaHandshake;
}

// =============================================================================
// PACKET PARSING
// =============================================================================

/**
 * PKO Packet Format (8-byte header):
 * Bytes 0-1: Length (Big Endian)
 * Bytes 2-3: Flags (0x8000 = normal packet)
 * Bytes 4-5: Unknown/Reserved (usually 0x0000)
 * Bytes 6-7: Command ID (Big Endian) - THIS is what we need to check!
 * Bytes 8+:  Payload
 * 
 * Example from logs:
 * Server RSA: 027c 8000 0000 03af = Len=636, Flags=0x8000, ???=0, CmdID=943
 * Client RSA: 027c 8000 0000 015c = Len=636, Flags=0x8000, ???=0, CmdID=348
 */
function parsePacketHeader(buffer) {
    if (buffer.length < 8) return null;
    
    const length = buffer.readUInt16BE(0);
    const flags = buffer.readUInt16BE(2);
    const reserved = buffer.readUInt16BE(4);
    const packetId = buffer.readUInt16BE(6);  // Command ID is at offset 6!
    
    return { length, flags, reserved, packetId };
}

function isValidClientResponse(buffer) {
    const header = parsePacketHeader(buffer);
    if (!header) return { valid: false, reason: 'too_short' };
    
    // Check packet size bounds
    if (header.length < CONFIG.minPacketSize || header.length > CONFIG.maxPacketSize) {
        return { valid: false, reason: 'invalid_size', size: header.length };
    }
    
    // Check flags - should be 0x8000 for normal packets
    // But be lenient - any packet with reasonable structure is ok
    
    // Client should respond with RSA handshake response (packet 348)
    // or other valid first packets like login (300-400 range)
    // Be lenient - any non-garbage packet ID is acceptable
    if (header.packetId === 0 || header.packetId > 2000) {
        return { valid: false, reason: 'invalid_packet_id', packetId: header.packetId };
    }
    
    return { valid: true, packetId: header.packetId, length: header.length, flags: header.flags };
}

// =============================================================================
// CONNECTION HANDLER (NEW ARCHITECTURE)
// =============================================================================

async function handleConnection(clientSocket) {
    const clientIP = clientSocket.remoteAddress?.replace('::ffff:', '') || 'unknown';
    const clientPort = clientSocket.remotePort;
    const connId = `${clientIP}:${clientPort}`;
    
    stats.totalConnections++;
    stats.activeConnections++;
    
    log('debug', 'New connection', { connId });
    
    // Check blocklist
    const blockReason = isBlocked(clientIP);
    if (blockReason) {
        stats.blockedByBlacklist++;
        log('debug', 'Blocked', { connId, reason: blockReason });
        clientSocket.destroy();
        stats.activeConnections--;
        return;
    }
    
    // Check rate limit
    const rateCheck = checkRateLimit(clientIP);
    if (!rateCheck.allowed) {
        stats.blockedByRateLimit++;
        log('debug', 'Rate limited', { connId, reason: rateCheck.reason });
        clientSocket.destroy();
        stats.activeConnections--;
        return;
    }
    
    // Connection state
    const state = {
        phase: 'handshake',       // handshake -> authenticated -> forwarding
        gateSocket: null,
        authTimer: null,
        buffer: Buffer.alloc(0),
        gateConnected: false,
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
            state.gateSocket = null;
        }
        
        if (!clientSocket.destroyed) {
            clientSocket.destroy();
        }
        
        connections.delete(clientSocket);
        releaseConnection(clientIP);
        stats.activeConnections--;
        
        log('debug', 'Closed', { connId, phase: state.phase });
    }
    
    // PHASE 1: Send cached RSA handshake to client
    // This happens IMMEDIATELY without connecting to GateServer
    try {
        const rsaHandshake = await getRsaHandshake();
        
        if (clientSocket.destroyed) {
            cleanup();
            return;
        }
        
        // Send RSA handshake to client
        clientSocket.write(rsaHandshake);
        log('debug', 'Sent RSA handshake', { connId });
        
        // Start auth timeout - client must respond within timeout
        state.authTimer = setTimeout(() => {
            if (state.phase === 'handshake') {
                stats.blockedByAuthTimeout++;
                log('debug', 'Auth timeout (no response)', { connId });
                tempBan(clientIP, 'auth_timeout');
                cleanup();
            }
        }, CONFIG.authTimeout);
        
    } catch (err) {
        log('error', 'Failed to get RSA handshake', { connId, error: err.message });
        cleanup();
        return;
    }
    
    // Handle client data
    clientSocket.on('data', (data) => {
        if (state.phase === 'handshake') {
            // PHASE 2: Waiting for client's RSA response
            state.buffer = Buffer.concat([state.buffer, data]);
            
            if (state.buffer.length >= 4) {
                const validation = isValidClientResponse(state.buffer);
                
                log('debug', 'Client response', {
                    connId,
                    bufLen: state.buffer.length,
                    valid: validation.valid,
                    packetId: validation.packetId,
                    hex: state.buffer.slice(0, Math.min(16, state.buffer.length)).toString('hex'),
                });
                
                if (validation.valid) {
                    // Valid response! Now connect to GateServer
                    clearTimeout(state.authTimer);
                    state.authTimer = null;
                    state.phase = 'connecting';
                    
                    stats.authenticatedClients++;
                    log('info', 'Authenticated', { connId, packetId: validation.packetId });
                    
                    // NOW connect to real GateServer
                    connectToGateServer(clientSocket, state, connId, clientIP, cleanup);
                } else {
                    // Invalid response - kill connection
                    stats.blockedByInvalidPacket++;
                    log('warn', 'Invalid response', { connId, ...validation });
                    tempBan(clientIP, `invalid_packet: ${validation.reason}`);
                    cleanup();
                }
            }
        } else if (state.phase === 'forwarding') {
            // PHASE 4: Forward data to GateServer
            if (state.gateSocket && !state.gateSocket.destroyed) {
                state.gateSocket.write(data);
                stats.bytesForwarded += data.length;
            }
        } else if (state.phase === 'connecting') {
            // Still connecting to GateServer - buffer the data
            state.buffer = Buffer.concat([state.buffer, data]);
        }
    });
    
    clientSocket.on('error', (err) => {
        log('debug', 'Client error', { connId, error: err.message });
        cleanup();
    });
    
    clientSocket.on('close', () => {
        cleanup();
    });
}

/**
 * Connect to real GateServer after client authentication
 */
function connectToGateServer(clientSocket, state, connId, clientIP, cleanup) {
    const clientPort = clientSocket.remotePort || 0;
    log('debug', 'Connecting to GateServer', { connId });
    
    state.gateSocket = net.createConnection({
        host: CONFIG.gateServerHost,
        port: CONFIG.gateServerPort,
        timeout: CONFIG.gateServerConnectTimeout,
    });
    
    state.gateSocket.on('connect', () => {
        log('debug', 'GateServer connected', { connId });
        state.gateConnected = true;
        
        // Send PROXY protocol v1 header (required when GateServer has ProxyProtocol = 1)
        // Format: "PROXY TCP4 <client_ip> <proxy_ip> <client_port> <proxy_port>\r\n"
        const proxyHeader = `PROXY TCP4 ${clientIP} ${CONFIG.gateServerHost} ${clientPort} ${CONFIG.gateServerPort}\r\n`;
        state.gateSocket.write(proxyHeader);
        log('debug', 'Sent PROXY protocol header', { connId, header: proxyHeader.trim() });
        
        // GateServer will send RSA handshake - we need to skip it since client already got one
        // Wait for first packet from GateServer
    });
    
    let skippedFirstPacket = false;
    
    state.gateSocket.on('data', (data) => {
        if (!skippedFirstPacket) {
            // Skip the first packet (RSA handshake - client already has it)
            skippedFirstPacket = true;
            
            // Now send client's buffered response to GateServer
            if (state.buffer.length > 0) {
                state.gateSocket.write(state.buffer);
                stats.bytesForwarded += state.buffer.length;
                state.buffer = Buffer.alloc(0);
            }
            
            state.phase = 'forwarding';
            log('debug', 'Now forwarding', { connId });
            return;
        }
        
        // Forward GateServer response to client
        if (!clientSocket.destroyed) {
            clientSocket.write(data);
            stats.bytesForwarded += data.length;
        }
    });
    
    state.gateSocket.on('timeout', () => {
        log('warn', 'GateServer connection timeout', { connId });
        cleanup();
    });
    
    state.gateSocket.on('error', (err) => {
        log('debug', 'GateServer error', { connId, error: err.message });
        cleanup();
    });
    
    state.gateSocket.on('close', () => {
        cleanup();
    });
}

// =============================================================================
// STATISTICS
// =============================================================================

function printStats() {
    const uptime = Math.floor((Date.now() - stats.startTime) / 1000);
    const hours = Math.floor(uptime / 3600);
    const minutes = Math.floor((uptime % 3600) / 60);
    const seconds = uptime % 60;
    
    console.log('\n' + '='.repeat(60));
    console.log('PKO Smart Proxy v2 Statistics');
    console.log('='.repeat(60));
    console.log(`Uptime: ${hours}h ${minutes}m ${seconds}s`);
    console.log(`Active connections: ${stats.activeConnections}`);
    console.log(`Total connections: ${stats.totalConnections}`);
    console.log(`Authenticated clients: ${stats.authenticatedClients}`);
    console.log(`Blocked by blocklist: ${stats.blockedByBlacklist}`);
    console.log(`Blocked by rate limit: ${stats.blockedByRateLimit}`);
    console.log(`Blocked by auth timeout: ${stats.blockedByAuthTimeout}`);
    console.log(`Blocked by invalid packet: ${stats.blockedByInvalidPacket}`);
    console.log(`RSA handshake refreshes: ${stats.rsaRefreshes}`);
    console.log(`Temp blacklist size: ${tempBlacklist.size}`);
    console.log(`Bytes forwarded: ${(stats.bytesForwarded / 1024 / 1024).toFixed(2)} MB`);
    console.log('='.repeat(60) + '\n');
}

// =============================================================================
// MAIN
// =============================================================================

async function main() {
    log('info', 'PKO Smart Proxy v2 starting...');
    log('info', `Backend: ${CONFIG.gateServerHost}:${CONFIG.gateServerPort}`);
    
    // Load blocklist
    loadBlocklist();
    
    // Pre-fetch RSA handshake
    try {
        await refreshRsaHandshake();
        log('info', 'RSA handshake cached successfully');
    } catch (err) {
        log('error', 'FATAL: Could not fetch initial RSA handshake', { error: err.message });
        process.exit(1);
    }
    
    // Create server
    const server = net.createServer(handleConnection);
    
    server.on('error', (err) => {
        log('error', 'Server error', { error: err.message });
    });
    
    server.listen(CONFIG.listenPort, CONFIG.listenHost, () => {
        log('info', `Proxy listening on ${CONFIG.listenHost}:${CONFIG.listenPort}`);
        log('info', 'DDoS Protection ACTIVE');
        log('info', `- Auth timeout: ${CONFIG.authTimeout}ms`);
        log('info', `- Rate limit: ${CONFIG.connectionRateLimit} conn/${CONFIG.connectionRateWindow/1000}s per IP`);
    });
    
    // Statistics interval
    setInterval(printStats, CONFIG.statsInterval);
    
    // RSA handshake refresh
    setInterval(async () => {
        try {
            await refreshRsaHandshake();
        } catch (err) {
            log('warn', 'Failed to refresh RSA handshake', { error: err.message });
        }
    }, CONFIG.rsaRefreshInterval);
    
    // Handle shutdown
    process.on('SIGINT', () => {
        log('info', 'Shutting down...');
        printStats();
        server.close();
        process.exit(0);
    });
    
    // Reload blocklist on SIGHUP
    process.on('SIGHUP', () => {
        log('info', 'Reloading blocklist...');
        loadBlocklist();
    });
}

main().catch(err => {
    console.error('Fatal error:', err);
    process.exit(1);
});
