#ifndef DBPOOL_H
#define DBPOOL_H

#include <vector>
#include <mutex>
#include <condition_variable>
#include <string>

// Forward declarations - full headers included in DBPool.cpp and via GroupServerApp.h
class cfl_db;
class TBLAccounts;
class TBLCharacters;
class TBLFriends;
class TBLMaster;
class TBLGuilds;
class TBLParam;
class friend_tbl;

/// A single database connection slot in the pool.
/// Each slot has its own ODBC connection and table objects,
/// allowing fully independent concurrent database access.
struct DBConnection {
	cfl_db*         db        = nullptr;
	TBLAccounts*    tblaccounts   = nullptr;
	TBLCharacters*  tblcharaters  = nullptr;  // note: original misspelling kept
	TBLFriends*     tblfriends    = nullptr;
	TBLMaster*      tblmaster     = nullptr;
	TBLGuilds*      tblguilds     = nullptr;
	TBLParam*       tblparam      = nullptr;
	friend_tbl*     tblX1         = nullptr;
	bool            in_use        = false;

	~DBConnection();
};

class DBPool;

/// RAII guard: automatically releases the connection back to the pool
/// when it goes out of scope. Access table objects via operator->.
class DBConnectionGuard {
public:
	DBConnectionGuard() : m_pool(nullptr), m_index(-1), m_conn(nullptr) {}
	DBConnectionGuard(DBPool* pool, int index, DBConnection* conn)
		: m_pool(pool), m_index(index), m_conn(conn) {}
	~DBConnectionGuard();

	// Move-only (no copying)
	DBConnectionGuard(const DBConnectionGuard&) = delete;
	DBConnectionGuard& operator=(const DBConnectionGuard&) = delete;
	DBConnectionGuard(DBConnectionGuard&& other) noexcept
		: m_pool(other.m_pool), m_index(other.m_index), m_conn(other.m_conn) {
		other.m_pool = nullptr;
		other.m_index = -1;
		other.m_conn = nullptr;
	}
	DBConnectionGuard& operator=(DBConnectionGuard&& other) noexcept {
		if (this != &other) {
			Release();
			m_pool = other.m_pool;
			m_index = other.m_index;
			m_conn = other.m_conn;
			other.m_pool = nullptr;
			other.m_index = -1;
			other.m_conn = nullptr;
		}
		return *this;
	}

	/// Check if this guard holds a valid connection
	explicit operator bool() const { return m_conn != nullptr; }

	/// Access the underlying DBConnection
	DBConnection* operator->() const { return m_conn; }
	DBConnection& operator*() const { return *m_conn; }

	/// Convenience accessors for table objects
	TBLAccounts*   tblaccounts()  const { return m_conn->tblaccounts; }
	TBLCharacters* tblcharaters() const { return m_conn->tblcharaters; }
	TBLFriends*    tblfriends()   const { return m_conn->tblfriends; }
	TBLMaster*     tblmaster()    const { return m_conn->tblmaster; }
	TBLGuilds*     tblguilds()    const { return m_conn->tblguilds; }
	TBLParam*      tblparam()     const { return m_conn->tblparam; }
	friend_tbl*    tblX1()        const { return m_conn->tblX1; }

	/// Access raw cfl_db (for begin_tran/commit_tran/rollback)
	cfl_db&        db()           const { return *m_conn->db; }

	/// Manually release early (before scope exit)
	void Release();

private:
	DBPool*       m_pool;
	int           m_index;
	DBConnection* m_conn;
};


/// Connection pool for GroupServer database access.
/// Replaces the single m_cfg_db + m_mtxDB with N independent connections.
/// Each connection has its own ODBC handle and table objects.
class DBPool {
	friend class DBConnectionGuard;
public:
	DBPool() : m_poolSize(0), m_initialized(false) {}
	~DBPool();

	/// Initialize the pool with N connections to the database.
	/// Returns true if all connections were established successfully.
	bool Initialize(int poolSize, const char* ip, const char* db,
	                const char* login, const char* passwd);

	/// Get a connection from the pool. Blocks if all connections are in use.
	/// Returns an RAII guard that auto-releases on scope exit.
	/// @param timeoutMs Maximum time to wait in ms (0 = wait forever)
	/// @return Valid guard on success, invalid guard (bool==false) on timeout
	DBConnectionGuard GetConnection(unsigned long timeoutMs = 10000);

	/// Check if pool is initialized
	bool IsInitialized() const { return m_initialized; }

	/// Get pool size
	int GetPoolSize() const { return m_poolSize; }

	/// Get a specific connection by index (for initialization tasks only - NOT thread safe)
	DBConnection* GetConnectionDirect(int index) {
		if (index >= 0 && index < m_poolSize) return &m_connections[index];
		return nullptr;
	}

private:
	void ReleaseConnection(int index);

	std::vector<DBConnection> m_connections;
	std::mutex                m_poolMutex;
	std::condition_variable   m_poolCV;
	int                       m_poolSize;
	bool                      m_initialized;
};

#endif // DBPOOL_H
