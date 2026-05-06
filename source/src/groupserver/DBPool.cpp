#include "stdafx.h"
#include "GroupServerApp.h"  // brings in all types (cChar, uLong, Player, etc.) in correct order
#include <chrono>

// ---- DBConnection ----

DBConnection::~DBConnection() {
	delete tblX1;       tblX1 = nullptr;
	delete tblparam;    tblparam = nullptr;
	delete tblguilds;   tblguilds = nullptr;
	delete tblmaster;   tblmaster = nullptr;
	delete tblfriends;  tblfriends = nullptr;
	delete tblcharaters; tblcharaters = nullptr;
	delete tblaccounts; tblaccounts = nullptr;
	delete db;          db = nullptr;
}

// ---- DBConnectionGuard ----

DBConnectionGuard::~DBConnectionGuard() {
	Release();
}

void DBConnectionGuard::Release() {
	if (m_pool && m_index >= 0) {
		m_pool->ReleaseConnection(m_index);
		m_pool = nullptr;
		m_index = -1;
		m_conn = nullptr;
	}
}

// ---- DBPool ----

DBPool::~DBPool() {
	// DBConnection destructor handles cleanup of table objects
	m_connections.clear();
	m_initialized = false;
}

bool DBPool::Initialize(int poolSize, const char* ip, const char* db,
                        const char* login, const char* passwd) {
	if (m_initialized) {
		LG("DBPool", "ERROR: Pool already initialized\n");
		return false;
	}
	if (poolSize < 1 || poolSize > 32) {
		LG("DBPool", "ERROR: Invalid pool size %d (must be 1-32)\n", poolSize);
		return false;
	}

	m_poolSize = poolSize;
	m_connections.resize(poolSize);

	LG("DBPool", "Initializing DB connection pool with %d connections...\n", poolSize);

	for (int i = 0; i < poolSize; i++) {
		DBConnection& conn = m_connections[i];
		conn.db = new cfl_db();
		conn.db->enable_errinfo();

		std::string errinfo;
		if (!conn.db->connect(ip, db, login, passwd, errinfo)) {
			LG("DBPool", "ERROR: Connection %d/%d failed: %s\n", i + 1, poolSize, errinfo.c_str());
			m_connections.clear();
			m_poolSize = 0;
			return false;
		}

		// Create table objects bound to this connection
		conn.tblaccounts  = new TBLAccounts(conn.db);
		conn.tblcharaters = new TBLCharacters(conn.db);
		conn.tblfriends   = new TBLFriends(conn.db);
		conn.tblmaster    = new TBLMaster(conn.db);
		conn.tblguilds    = new TBLGuilds(conn.db);
		conn.tblX1        = new friend_tbl(conn.db);
		conn.tblparam     = new TBLParam(conn.db);
		conn.in_use       = false;

		LG("DBPool", "Connection %d/%d established\n", i + 1, poolSize);
	}

	m_initialized = true;
	LG("DBPool", "DB connection pool initialized successfully (%d connections)\n", poolSize);
	return true;
}

DBConnectionGuard DBPool::GetConnection(unsigned long timeoutMs) {
	std::unique_lock<std::mutex> lock(m_poolMutex);

	auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(timeoutMs);

	while (true) {
		// Try to find a free connection
		for (int i = 0; i < m_poolSize; i++) {
			if (!m_connections[i].in_use) {
				m_connections[i].in_use = true;
				return DBConnectionGuard(this, i, &m_connections[i]);
			}
		}

		// No free connection - wait
		if (timeoutMs == 0) {
			// Wait indefinitely
			m_poolCV.wait(lock);
		} else {
			if (m_poolCV.wait_until(lock, deadline) == std::cv_status::timeout) {
				// Check one more time after timeout
				for (int i = 0; i < m_poolSize; i++) {
					if (!m_connections[i].in_use) {
						m_connections[i].in_use = true;
						return DBConnectionGuard(this, i, &m_connections[i]);
					}
				}
				LG("DBPool", "WARNING: GetConnection timed out after %lu ms (all %d connections busy)\n",
				   timeoutMs, m_poolSize);
				return DBConnectionGuard(); // Invalid guard
			}
		}
	}
}

void DBPool::ReleaseConnection(int index) {
	{
		std::lock_guard<std::mutex> lock(m_poolMutex);
		if (index >= 0 && index < m_poolSize) {
			m_connections[index].in_use = false;
		}
	}
	m_poolCV.notify_one();
}
