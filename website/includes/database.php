<?php
/**
 * PKO Website Database Connection Manager
 * 
 * Provides secure database connections using PDO with SQL Server
 * All queries use parameterized statements to prevent SQL injection
 */

declare(strict_types=1);

class Database
{
    private static ?PDO $accountDb = null;
    private static ?PDO $gameDb = null;
    private static ?PDO $webDb = null;
    
    /**
     * Get connection options for PDO
     */
    private static function getConnectionOptions(): array
    {
        return [
            PDO::ATTR_ERRMODE => PDO::ERRMODE_EXCEPTION,
            PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
            PDO::ATTR_EMULATE_PREPARES => false,
            PDO::SQLSRV_ATTR_ENCODING => PDO::SQLSRV_ENCODING_UTF8,
        ];
    }
    
    /**
     * Create DSN string for SQL Server
     */
    private static function createDsn(string $host, string $database): string
    {
        return "sqlsrv:Server={$host};Database={$database};TrustServerCertificate=1;LoginTimeout=3;ConnectionPooling=0";
    }
    
    /**
     * Connect to AccountServer database
     */
    public static function getAccountDb(): PDO
    {
        if (self::$accountDb === null) {
            try {
                // Use Windows Authentication when password is empty, SQL auth otherwise
                $useTrusted = empty(DB_ACCOUNT_PASS);
                self::$accountDb = new PDO(
                    self::createDsn(DB_ACCOUNT_HOST, DB_ACCOUNT_NAME),
                    $useTrusted ? null : DB_ACCOUNT_USER,
                    $useTrusted ? null : DB_ACCOUNT_PASS,
                    self::getConnectionOptions()
                );
            } catch (PDOException $e) {
                error_log("AccountServer DB connection failed: " . $e->getMessage());
                throw new Exception("Database connection failed. Please try again later.");
            }
        }
        return self::$accountDb;
    }
    
    /**
     * Connect to GameDB database
     */
    public static function getGameDb(): PDO
    {
        if (self::$gameDb === null) {
            try {
                // Use Windows Authentication when password is empty, SQL auth otherwise
                $useTrusted = empty(DB_GAME_PASS);
                self::$gameDb = new PDO(
                    self::createDsn(DB_GAME_HOST, DB_GAME_NAME),
                    $useTrusted ? null : DB_GAME_USER,
                    $useTrusted ? null : DB_GAME_PASS,
                    self::getConnectionOptions()
                );
            } catch (PDOException $e) {
                error_log("GameDB connection failed: " . $e->getMessage());
                throw new Exception("Database connection failed. Please try again later.");
            }
        }
        return self::$gameDb;
    }
    
    /**
     * Connect to WebsiteDB database
     */
    public static function getWebDb(): PDO
    {
        if (self::$webDb === null) {
            try {
                // Use Windows Authentication when password is empty, SQL auth otherwise
                $useTrusted = empty(DB_WEB_PASS);
                self::$webDb = new PDO(
                    self::createDsn(DB_WEB_HOST, DB_WEB_NAME),
                    $useTrusted ? null : DB_WEB_USER,
                    $useTrusted ? null : DB_WEB_PASS,
                    self::getConnectionOptions()
                );
            } catch (PDOException $e) {
                error_log("WebsiteDB connection failed: " . $e->getMessage());
                throw new Exception("Database connection failed. Please try again later.");
            }
        }
        return self::$webDb;
    }
    
    /**
     * Close all database connections
     */
    public static function closeAll(): void
    {
        self::$accountDb = null;
        self::$gameDb = null;
        self::$webDb = null;
    }
}

/**
 * Account Model - Handles account-related database operations
 */
class AccountModel
{
    /**
     * Find account by username (using parameterized query)
     */
    public static function findByUsername(string $username): ?array
    {
        $db = Database::getAccountDb();
        $stmt = $db->prepare("
            SELECT id, name, password, email, login_status, ban, 
                   last_login_time, last_login_ip, total_live_time,
                   enable_login_time
            FROM account_login 
            WHERE name = ?
        ");
        $stmt->execute([$username]);
        $result = $stmt->fetch();
        return $result ?: null;
    }
    
    /**
     * Find account by ID
     */
    public static function findById(int $id): ?array
    {
        $db = Database::getAccountDb();
        $stmt = $db->prepare("
            SELECT id, name, email, login_status, ban, 
                   last_login_time, last_login_ip, total_live_time
            FROM account_login 
            WHERE id = ?
        ");
        $stmt->execute([$id]);
        $result = $stmt->fetch();
        return $result ?: null;
    }
    
    /**
     * Check if username exists
     */
    public static function usernameExists(string $username): bool
    {
        $db = Database::getAccountDb();
        $stmt = $db->prepare("SELECT 1 FROM account_login WHERE name = ?");
        $stmt->execute([$username]);
        return $stmt->fetch() !== false;
    }
    
    /**
     * Check if email exists
     */
    public static function emailExists(string $email): bool
    {
        $db = Database::getAccountDb();
        $stmt = $db->prepare("SELECT 1 FROM account_login WHERE email = ?");
        $stmt->execute([$email]);
        return $stmt->fetch() !== false;
    }
    
    /**
     * Create new account using stored procedure
     */
    public static function create(string $username, string $password, string $email): ?int
    {
        $db = Database::getAccountDb();
        
        // Hash password with bcrypt
        $hashedPassword = password_hash($password, PASSWORD_BCRYPT, ['cost' => BCRYPT_COST]);
        
        try {
            $stmt = $db->prepare("{CALL dbo.InsertNewUser(?, ?, ?)}");
            $stmt->execute([$username, $hashedPassword, $email]);
            $result = $stmt->fetch();
            return $result ? (int)$result['NewAccountId'] : null;
        } catch (PDOException $e) {
            error_log("Account creation failed: " . $e->getMessage());
            return null;
        }
    }
    
    /**
     * Verify password using bcrypt
     */
    public static function verifyPassword(array $account, string $password): bool
    {
        return password_verify($password, $account['password']);
    }
    
    /**
     * Verify password by username (fetches password from DB)
     */
    public static function verifyPasswordByUsername(string $username, string $password): bool
    {
        $db = Database::getAccountDb();
        $stmt = $db->prepare("SELECT password FROM account_login WHERE name = ?");
        $stmt->execute([$username]);
        $result = $stmt->fetch();
        
        if (!$result || empty($result['password'])) {
            return false;
        }
        
        return password_verify($password, $result['password']);
    }
    
    /**
     * Update last login info
     */
    public static function updateLoginInfo(int $accountId, string $ip): void
    {
        $db = Database::getAccountDb();
        try {
            $stmt = $db->prepare("{CALL dbo.InsertUserLogin(?, ?, ?)}");
            $stmt->execute([(string)$accountId, '', $ip]);
        } catch (PDOException $e) {
            error_log("Login info update failed: " . $e->getMessage());
        }
    }
    
    /**
     * Update password
     */
    public static function updatePassword(string $username, string $newPassword): bool
    {
        $db = Database::getAccountDb();
        $hashedPassword = password_hash($newPassword, PASSWORD_BCRYPT, ['cost' => BCRYPT_COST]);
        
        try {
            $stmt = $db->prepare("{CALL dbo.UpdateUserPassword(?, ?)}");
            $stmt->execute([$username, $hashedPassword]);
            return true;
        } catch (PDOException $e) {
            error_log("Password update failed: " . $e->getMessage());
            return false;
        }
    }
    
    /**
     * Get account details (VIP status, credit, etc.)
     */
    public static function getDetails(int $accountId): ?array
    {
        $db = Database::getAccountDb();
        $stmt = $db->prepare("
            SELECT acc_id, vipstatus, credit, create_time
            FROM account_details 
            WHERE acc_id = ?
        ");
        $stmt->execute([$accountId]);
        $result = $stmt->fetch();
        return $result ?: null;
    }
    
    /**
     * Get total online players
     */
    public static function getOnlineCount(): int
    {
        $db = Database::getAccountDb();
        $stmt = $db->query("SELECT COUNT(*) as count FROM account_login WHERE login_status = 1");
        $result = $stmt->fetch();
        return (int)($result['count'] ?? 0);
    }
    
    /**
     * Get GM level from GameDB
     */
    public static function getGmLevel(int $accountId): int
    {
        $db = Database::getGameDb();
        $stmt = $db->prepare("SELECT gm FROM account WHERE act_id = ?");
        $stmt->execute([$accountId]);
        $result = $stmt->fetch();
        return (int)($result['gm'] ?? 0);
    }

    /**
     * Get total registered accounts
     */
    public static function getTotalAccounts(): int
    {
        $db = Database::getAccountDb();
        $stmt = $db->query("SELECT COUNT(*) as count FROM account_login");
        $result = $stmt->fetch();
        return (int)($result['count'] ?? 0);
    }
}

/**
 * Character Model - Handles character-related database operations
 */
class CharacterModel
{
    /**
     * Get characters for an account
     */
    public static function getByAccountId(int $accountId): array
    {
        $db = Database::getGameDb();
        $stmt = $db->prepare("
            SELECT cha_id, cha_name, job, degree, exp, hp, sp, 
                   [str], dex, agi, con, sta, luk, gd, map,
                   guild_id, look, credit, IMP, battle_power
            FROM character 
            WHERE act_id = ? AND delflag = 0
            ORDER BY degree DESC
        ");
        $stmt->execute([$accountId]);
        return $stmt->fetchAll();
    }
    
    /**
     * Get character by ID
     */
    public static function findById(int $chaId): ?array
    {
        $db = Database::getGameDb();
        $stmt = $db->prepare("
            SELECT cha_id, cha_name, motto, job, degree, exp, hp, sp, ap, tp, gd,
                   [str], dex, agi, con, sta, luk, map, map_x, map_y,
                   guild_id, guild_stat, look, credit, IMP, battle_power,
                   sail_lv, live_lv, act_id
            FROM character 
            WHERE cha_id = ? AND delflag = 0
        ");
        $stmt->execute([$chaId]);
        $result = $stmt->fetch();
        return $result ?: null;
    }
    
    /**
     * Get character by name
     */
    public static function findByName(string $name): ?array
    {
        $db = Database::getGameDb();
        $stmt = $db->prepare("
            SELECT cha_id, cha_name, job, degree, exp, 
                   guild_id, look, battle_power, act_id
            FROM character 
            WHERE cha_name = ? AND delflag = 0
        ");
        $stmt->execute([$name]);
        $result = $stmt->fetch();
        return $result ?: null;
    }
    
    /**
     * Get top characters by level
     */
    public static function getTopByLevel(int $limit = 10): array
    {
        $db = Database::getGameDb();
        $limit = (int)$limit; // Ensure integer
        $stmt = $db->query("
            SELECT TOP {$limit} c.cha_id, c.cha_name, c.job, c.degree, c.exp,
                   c.battle_power, c.guild_id, g.guild_name
            FROM character c
            LEFT JOIN guild g ON c.guild_id = g.guild_id
            WHERE c.delflag = 0
            ORDER BY c.degree DESC, c.exp DESC
        ");
        return $stmt->fetchAll();
    }
    
    /**
     * Get top characters by gold
     */
    public static function getTopByGold(int $limit = 10): array
    {
        $db = Database::getGameDb();
        $limit = (int)$limit; // Ensure integer
        $stmt = $db->query("
            SELECT TOP {$limit} c.cha_id, c.cha_name, c.job, c.degree, c.gd,
                   c.guild_id, g.guild_name
            FROM character c
            LEFT JOIN guild g ON c.guild_id = g.guild_id
            WHERE c.delflag = 0
            ORDER BY c.gd DESC
        ");
        return $stmt->fetchAll();
    }
    
    /**
     * Get total characters
     */
    public static function getTotalCharacters(): int
    {
        $db = Database::getGameDb();
        $stmt = $db->query("SELECT COUNT(*) as count FROM character WHERE delflag = 0");
        $result = $stmt->fetch();
        return (int)($result['count'] ?? 0);
    }
    
    /**
     * Search characters by name
     */
    public static function search(string $query, int $limit = 20): array
    {
        $db = Database::getGameDb();
        $limit = (int)$limit; // Ensure integer
        $stmt = $db->prepare("
            SELECT TOP {$limit} cha_id, cha_name, job, degree, guild_id
            FROM character 
            WHERE cha_name LIKE ? AND delflag = 0
            ORDER BY degree DESC
        ");
        $stmt->execute(["%{$query}%"]);
        return $stmt->fetchAll();
    }
}

/**
 * Guild Model - Handles guild-related database operations
 */
class GuildModel
{
    /**
     * Get guild by ID
     */
    public static function findById(int $guildId): ?array
    {
        $db = Database::getGameDb();
        $stmt = $db->prepare("
            SELECT g.guild_id, g.guild_name, g.motto, g.leader_id, g.exp, g.level,
                   g.member_total, c.cha_name as leader_name
            FROM guild g
            LEFT JOIN character c ON g.leader_id = c.cha_id
            WHERE g.guild_id = ?
        ");
        $stmt->execute([$guildId]);
        $result = $stmt->fetch();
        return $result ?: null;
    }
    
    /**
     * Get guild members
     */
    public static function getMembers(int $guildId): array
    {
        $db = Database::getGameDb();
        $stmt = $db->prepare("
            SELECT cha_id, cha_name, job, degree, guild_stat
            FROM character 
            WHERE guild_id = ? AND delflag = 0
            ORDER BY guild_stat DESC, degree DESC
        ");
        $stmt->execute([$guildId]);
        return $stmt->fetchAll();
    }
    
    /**
     * Get top guilds by member count
     */
    public static function getTopByMembers(int $limit = 10): array
    {
        $db = Database::getGameDb();
        $limit = (int)$limit; // Ensure integer
        $stmt = $db->query("
            SELECT TOP {$limit} g.guild_id, g.guild_name, g.motto, g.level, g.exp,
                   g.member_total, c.cha_name as leader_name
            FROM guild g
            LEFT JOIN character c ON g.leader_id = c.cha_id
            WHERE g.guild_name NOT LIKE 'Pirate Guild %'
            ORDER BY g.member_total DESC, g.level DESC
        ");
        return $stmt->fetchAll();
    }
    
    /**
     * Get total guilds (excluding placeholders)
     */
    public static function getTotalGuilds(): int
    {
        $db = Database::getGameDb();
        $stmt = $db->query("SELECT COUNT(*) as count FROM guild WHERE guild_name NOT LIKE 'Pirate Guild %'");
        $result = $stmt->fetch();
        return (int)($result['count'] ?? 0);
    }
}

/**
 * Vote Model - Handles voting system
 */
class VoteModel
{
    /**
     * Get all vote sites
     */
    public static function getSites(): array
    {
        $db = Database::getWebDb();
        $stmt = $db->query("SELECT id, name, prize, link, image FROM vote");
        return $stmt->fetchAll();
    }
    
    /**
     * Check if user can vote on a specific site
     */
    public static function canVote(int $accountId, int $siteId, string $ip): bool
    {
        $db = Database::getWebDb();
        
        // Check cooldown by account
        $stmt = $db->prepare("
            SELECT TOP 1 [date] 
            FROM vote_log 
            WHERE act_id = ? AND id = ?
            ORDER BY [date] DESC
        ");
        $stmt->execute([$accountId, (string)$siteId]);
        $lastVote = $stmt->fetch();
        
        if ($lastVote) {
            $lastVoteTime = strtotime($lastVote['date']);
            $cooldownEnd = $lastVoteTime + (VOTE_COOLDOWN_HOURS * 3600);
            if (time() < $cooldownEnd) {
                return false;
            }
        }
        
        return true;
    }
    
    /**
     * Record a vote
     */
    public static function recordVote(int $accountId, string $username, int $siteId, string $ip): bool
    {
        $db = Database::getWebDb();
        
        try {
            $stmt = $db->prepare("
                INSERT INTO vote_log (ip, act_id, account, id, [date])
                VALUES (?, ?, ?, ?, GETDATE())
            ");
            $stmt->execute([$ip, $accountId, $username, (string)$siteId]);
            
            // Add credit to account
            $site = self::getSiteById($siteId);
            if ($site) {
                $gameDb = Database::getGameDb();
                $creditStmt = $gameDb->prepare("
                    UPDATE account 
                    SET total_votes = total_votes + 1, credit = credit + ?
                    WHERE act_id = ?
                ");
                $creditStmt->execute([$site['prize'], $accountId]);
            }
            
            return true;
        } catch (PDOException $e) {
            error_log("Vote recording failed: " . $e->getMessage());
            return false;
        }
    }
    
    /**
     * Get specific vote site
     */
    public static function getSiteById(int $siteId): ?array
    {
        $db = Database::getWebDb();
        $stmt = $db->prepare("SELECT id, name, prize, link, image FROM vote WHERE id = ?");
        $stmt->execute([$siteId]);
        $result = $stmt->fetch();
        return $result ?: null;
    }
}

/**
 * Item Shop Model - Handles in-game shop
 */
class ShopModel
{
    /**
     * Get all shop items
     */
    public static function getItems(?string $category = null): array
    {
        $db = Database::getWebDb();
        
        if ($category) {
            $stmt = $db->prepare("
                SELECT TavaraListaID, TuoNimi, TavaraHinta, TavaraTeksti, 
                       Quota, Icon, MyyniTyyppi, TavaraID, MontaTavaraa, category, bought
                FROM LarryEdit 
                WHERE category = ? AND Quota > bought
                ORDER BY TavaraListaID
            ");
            $stmt->execute([$category]);
        } else {
            $stmt = $db->query("
                SELECT TavaraListaID, TuoNimi, TavaraHinta, TavaraTeksti, 
                       Quota, Icon, MyyniTyyppi, TavaraID, MontaTavaraa, category, bought
                FROM LarryEdit 
                WHERE Quota > bought
                ORDER BY category, TavaraListaID
            ");
        }
        
        return $stmt->fetchAll();
    }
    
    /**
     * Get shop categories
     */
    public static function getCategories(): array
    {
        $db = Database::getWebDb();
        $stmt = $db->query("SELECT DISTINCT TRIM(category) as category FROM LarryEdit ORDER BY category");
        return $stmt->fetchAll(PDO::FETCH_COLUMN);
    }
    
    /**
     * Purchase an item
     */
    public static function purchase(int $accountId, int $itemId, int $chaId): array
    {
        $db = Database::getWebDb();
        
        // Get item details
        $stmt = $db->prepare("
            SELECT TavaraListaID, TuoNimi, TavaraHinta, TavaraID, MontaTavaraa, 
                   Icon, Quota, bought
            FROM LarryEdit 
            WHERE TavaraListaID = ?
        ");
        $stmt->execute([$itemId]);
        $item = $stmt->fetch();
        
        if (!$item) {
            return ['success' => false, 'error' => 'Item not found'];
        }
        
        if ($item['Quota'] <= $item['bought']) {
            return ['success' => false, 'error' => 'Item out of stock'];
        }
        
        // Get account credit
        $gameDb = Database::getGameDb();
        $creditStmt = $gameDb->prepare("SELECT credit FROM account WHERE act_id = ?");
        $creditStmt->execute([$accountId]);
        $account = $creditStmt->fetch();
        
        if (!$account || $account['credit'] < $item['TavaraHinta']) {
            return ['success' => false, 'error' => 'Insufficient credits'];
        }
        
        // Get character info
        $chaStmt = $gameDb->prepare("SELECT cha_name FROM character WHERE cha_id = ? AND act_id = ?");
        $chaStmt->execute([$chaId, $accountId]);
        $character = $chaStmt->fetch();
        
        if (!$character) {
            return ['success' => false, 'error' => 'Character not found'];
        }
        
        try {
            // Start transaction
            $gameDb->beginTransaction();
            $db->beginTransaction();
            
            // Deduct credits
            $deductStmt = $gameDb->prepare("UPDATE account SET credit = credit - ? WHERE act_id = ?");
            $deductStmt->execute([$item['TavaraHinta'], $accountId]);
            
            // Update bought count
            $updateStmt = $db->prepare("UPDATE LarryEdit SET bought = bought + 1 WHERE TavaraListaID = ?");
            $updateStmt->execute([$itemId]);
            
            // Add to delivery box
            $deliveryStmt = $db->prepare("
                INSERT INTO LarryLaatikko (act_id, item_id, assigned, assigned_char, assigned_date, Icon, TuoNimi, MontaTavaraa)
                VALUES (?, ?, 0, ?, GETDATE(), ?, ?, ?)
            ");
            $deliveryStmt->execute([
                $accountId,
                (string)$item['TavaraID'],
                $character['cha_name'],
                $item['Icon'],
                $item['TuoNimi'],
                $item['MontaTavaraa']
            ]);
            
            $gameDb->commit();
            $db->commit();
            
            return ['success' => true, 'message' => 'Purchase successful! Check your in-game delivery box.'];
        } catch (PDOException $e) {
            $gameDb->rollBack();
            $db->rollBack();
            error_log("Purchase failed: " . $e->getMessage());
            return ['success' => false, 'error' => 'Transaction failed. Please try again.'];
        }
    }
}

/**
 * Economy Model - Handles economy/market data analysis
 */
class EconomyModel
{
    /**
     * Get server economy statistics
     */
    public static function getServerStats(): array
    {
        $db = Database::getGameDb();
        
        // Total gold in circulation
        $goldStmt = $db->query("
            SELECT 
                SUM(CAST(gd AS BIGINT)) as total_gold,
                AVG(CAST(gd AS BIGINT)) as avg_gold,
                MAX(gd) as max_gold,
                COUNT(*) as player_count
            FROM character 
            WHERE delflag = 0
        ");
        $goldStats = $goldStmt->fetch();
        
        // Guild treasury totals
        $guildStmt = $db->query("
            SELECT 
                SUM(CAST(gold AS BIGINT)) as total_guild_gold,
                COUNT(*) as guild_count
            FROM guild 
            WHERE guild_name NOT LIKE 'Pirate Guild %'
        ");
        $guildStats = $guildStmt->fetch();
        
        // Wealth distribution (by level brackets)
        $distributionStmt = $db->query("
            SELECT 
                CASE 
                    WHEN degree < 20 THEN '1-19'
                    WHEN degree < 40 THEN '20-39'
                    WHEN degree < 60 THEN '40-59'
                    WHEN degree < 80 THEN '60-79'
                    ELSE '80+'
                END as level_bracket,
                COUNT(*) as player_count,
                SUM(CAST(gd AS BIGINT)) as total_gold,
                AVG(CAST(gd AS BIGINT)) as avg_gold
            FROM character 
            WHERE delflag = 0
            GROUP BY 
                CASE 
                    WHEN degree < 20 THEN '1-19'
                    WHEN degree < 40 THEN '20-39'
                    WHEN degree < 60 THEN '40-59'
                    WHEN degree < 80 THEN '60-79'
                    ELSE '80+'
                END
            ORDER BY MIN(degree)
        ");
        $distribution = $distributionStmt->fetchAll();
        
        return [
            'total_gold' => (int)($goldStats['total_gold'] ?? 0),
            'average_gold' => (int)($goldStats['avg_gold'] ?? 0),
            'max_gold' => (int)($goldStats['max_gold'] ?? 0),
            'player_count' => (int)($goldStats['player_count'] ?? 0),
            'guild_treasury' => (int)($guildStats['total_guild_gold'] ?? 0),
            'guild_count' => (int)($guildStats['guild_count'] ?? 0),
            'wealth_distribution' => $distribution,
        ];
    }
    
    /**
     * Get active offline stalls (market listings)
     */
    public static function getActiveStalls(int $limit = 50): array
    {
        $db = Database::getGameDb();
        
        try {
            $stmt = $db->prepare("
                SELECT TOP (?) 
                    stall_id, cha_name, stall_title, map_name, 
                    pos_x, pos_y, item_count, created_time, expire_time
                FROM offline_stalls 
                WHERE is_active = 1 AND expire_time > GETDATE()
                ORDER BY created_time DESC
            ");
            $stmt->execute([$limit]);
            return $stmt->fetchAll();
        } catch (PDOException $e) {
            // Table might not exist yet
            error_log("Offline stalls query failed: " . $e->getMessage());
            return [];
        }
    }
    
    /**
     * Get stall count by map
     */
    public static function getStallsByMap(): array
    {
        $db = Database::getGameDb();
        
        try {
            $stmt = $db->query("
                SELECT map_name, COUNT(*) as stall_count
                FROM offline_stalls 
                WHERE is_active = 1 AND expire_time > GETDATE()
                GROUP BY map_name
                ORDER BY stall_count DESC
            ");
            return $stmt->fetchAll();
        } catch (PDOException $e) {
            return [];
        }
    }
    
    /**
     * Get top richest players (for economy analysis)
     */
    public static function getTopRichest(int $limit = 20): array
    {
        $db = Database::getGameDb();
        $limit = (int)$limit;
        
        $stmt = $db->query("
            SELECT TOP {$limit} 
                c.cha_id, c.cha_name, c.job, c.degree, c.gd,
                g.guild_name
            FROM character c
            LEFT JOIN guild g ON c.guild_id = g.guild_id
            WHERE c.delflag = 0
            ORDER BY c.gd DESC
        ");
        return $stmt->fetchAll();
    }
    
    /**
     * Get class distribution
     */
    public static function getClassDistribution(): array
    {
        $db = Database::getGameDb();
        
        $stmt = $db->query("
            SELECT 
                CASE 
                    WHEN job = '' OR job IS NULL THEN 'Newbie'
                    ELSE job 
                END as job_class,
                COUNT(*) as count,
                AVG(degree) as avg_level,
                AVG(CAST(gd AS BIGINT)) as avg_gold
            FROM character 
            WHERE delflag = 0
            GROUP BY job
            ORDER BY count DESC
        ");
        return $stmt->fetchAll();
    }
    
    /**
     * Get shop purchase statistics (from web shop)
     */
    public static function getShopStats(): array
    {
        try {
            $db = Database::getWebDb();
            
            // Most purchased items
            $topItems = $db->query("
                SELECT TOP 10 
                    TuoNimi as item_name,
                    Icon as icon,
                    bought as purchase_count,
                    TavaraHinta as price
                FROM LarryEdit 
                WHERE bought > 0
                ORDER BY bought DESC
            ");
            
            // Total purchases
            $totalStmt = $db->query("SELECT SUM(bought) as total FROM LarryEdit");
            $total = $totalStmt->fetch();
            
            return [
                'top_items' => $topItems->fetchAll(),
                'total_purchases' => (int)($total['total'] ?? 0),
            ];
        } catch (PDOException $e) {
            error_log("Shop stats query failed: " . $e->getMessage());
            return ['top_items' => [], 'total_purchases' => 0];
        }
    }
    
    /**
     * Get recent character activity (new characters by day)
     */
    public static function getRecentActivity(int $days = 7): array
    {
        $db = Database::getGameDb();
        
        $stmt = $db->prepare("
            SELECT 
                CAST(operdate AS DATE) as date,
                COUNT(*) as new_characters
            FROM character 
            WHERE operdate >= DATEADD(DAY, -?, GETDATE()) AND delflag = 0
            GROUP BY CAST(operdate AS DATE)
            ORDER BY date DESC
        ");
        $stmt->execute([$days]);
        return $stmt->fetchAll();
    }
    
    /**
     * Get level distribution for charts
     */
    public static function getLevelDistribution(): array
    {
        $db = Database::getGameDb();
        
        $stmt = $db->query("
            SELECT 
                CASE 
                    WHEN degree BETWEEN 1 AND 10 THEN '1-10'
                    WHEN degree BETWEEN 11 AND 20 THEN '11-20'
                    WHEN degree BETWEEN 21 AND 30 THEN '21-30'
                    WHEN degree BETWEEN 31 AND 40 THEN '31-40'
                    WHEN degree BETWEEN 41 AND 50 THEN '41-50'
                    WHEN degree BETWEEN 51 AND 60 THEN '51-60'
                    WHEN degree BETWEEN 61 AND 70 THEN '61-70'
                    WHEN degree BETWEEN 71 AND 80 THEN '71-80'
                    WHEN degree BETWEEN 81 AND 90 THEN '81-90'
                    ELSE '91+'
                END as level_range,
                COUNT(*) as count
            FROM character 
            WHERE delflag = 0
            GROUP BY 
                CASE 
                    WHEN degree BETWEEN 1 AND 10 THEN '1-10'
                    WHEN degree BETWEEN 11 AND 20 THEN '11-20'
                    WHEN degree BETWEEN 21 AND 30 THEN '21-30'
                    WHEN degree BETWEEN 31 AND 40 THEN '31-40'
                    WHEN degree BETWEEN 41 AND 50 THEN '41-50'
                    WHEN degree BETWEEN 51 AND 60 THEN '51-60'
                    WHEN degree BETWEEN 61 AND 70 THEN '61-70'
                    WHEN degree BETWEEN 71 AND 80 THEN '71-80'
                    WHEN degree BETWEEN 81 AND 90 THEN '81-90'
                    ELSE '91+'
                END
            ORDER BY MIN(degree)
        ");
        return $stmt->fetchAll();
    }
    
    /**
     * Get raw kitbag data for a character
     */
    public static function getCharacterKitbagRaw(int $chaId): ?string
    {
        $db = Database::getGameDb();
        
        $stmt = $db->prepare("
            SELECT content 
            FROM Resource 
            WHERE cha_id = ? AND type_id = 1
        ");
        $stmt->execute([$chaId]);
        $result = $stmt->fetch();
        
        return $result['content'] ?? null;
    }
    
    /**
     * Get all kitbag data for inventory analysis
     * Returns character ID and raw content for bulk parsing
     */
    public static function getAllKitbagData(): array
    {
        $db = Database::getGameDb();
        
        $stmt = $db->query("
            SELECT r.cha_id, r.content, c.cha_name, c.degree, c.job
            FROM Resource r
            INNER JOIN character c ON r.cha_id = c.cha_id
            WHERE r.type_id = 1 AND c.delflag = 0
        ");
        
        return $stmt->fetchAll();
    }
    
    /**
     * Get item distribution across all players (most common items)
     */
    public static function getItemDistribution(int $limit = 50): array
    {
        // This uses the helper function which does the heavy lifting
        return getServerItemDistribution($limit);
    }
    
    /**
     * Get rarest items in the game
     */
    public static function getRarestItems(int $limit = 20): array
    {
        return getRarestItems($limit, 1);
    }
    
    /**
     * Get economy snapshot history for charts
     */
    public static function getEconomyHistory(int $days = 30): array
    {
        try {
            $db = Database::getGameDb();
            
            $stmt = $db->prepare("
                SELECT 
                    snapshot_id,
                    snapshot_time,
                    total_gold,
                    total_guild_gold,
                    avg_player_gold,
                    total_characters,
                    active_stalls,
                    total_stall_items,
                    gini_coefficient
                FROM economy_snapshots
                WHERE snapshot_time >= DATEADD(DAY, -?, GETDATE())
                ORDER BY snapshot_time ASC
            ");
            $stmt->execute([$days]);
            return $stmt->fetchAll();
        } catch (PDOException $e) {
            // Table might not exist yet
            error_log("Economy history query failed: " . $e->getMessage());
            return [];
        }
    }
    
    /**
     * Get item supply/demand trends over time
     */
    public static function getItemTrends(int $itemId, int $days = 30): array
    {
        try {
            $db = Database::getGameDb();
            
            $stmt = $db->prepare("
                SELECT 
                    s.snapshot_time,
                    i.total_supply,
                    i.player_count,
                    i.listed_for_sale,
                    i.avg_asking_price,
                    i.scarcity_index,
                    i.supply_demand_ratio
                FROM economy_item_snapshots i
                INNER JOIN economy_snapshots s ON i.snapshot_id = s.snapshot_id
                WHERE i.item_id = ?
                  AND s.snapshot_time >= DATEADD(DAY, -?, GETDATE())
                ORDER BY s.snapshot_time ASC
            ");
            $stmt->execute([$itemId, $days]);
            return $stmt->fetchAll();
        } catch (PDOException $e) {
            error_log("Item trends query failed: " . $e->getMessage());
            return [];
        }
    }
    
    /**
     * Get latest item snapshot data with calculated metrics
     */
    public static function getLatestItemMetrics(int $limit = 100): array
    {
        try {
            $db = Database::getGameDb();
            
            // Get the latest snapshot
            $latestStmt = $db->query("
                SELECT TOP 1 snapshot_id 
                FROM economy_snapshots 
                ORDER BY snapshot_time DESC
            ");
            $latest = $latestStmt->fetch();
            
            if (!$latest) {
                return [];
            }
            
            $stmt = $db->prepare("
                SELECT TOP (?)
                    item_id,
                    total_supply,
                    player_count,
                    avg_per_player,
                    listed_for_sale,
                    scarcity_index,
                    supply_demand_ratio
                FROM economy_item_snapshots
                WHERE snapshot_id = ?
                ORDER BY total_supply DESC
            ");
            $stmt->execute([$limit, $latest['snapshot_id']]);
            return $stmt->fetchAll();
        } catch (PDOException $e) {
            error_log("Item metrics query failed: " . $e->getMessage());
            return [];
        }
    }
    
    /**
     * Get supply vs demand summary for all tracked items
     */
    public static function getSupplyDemandSummary(): array
    {
        try {
            $db = Database::getGameDb();
            
            // Get latest snapshot
            $stmt = $db->query("
                SELECT TOP 1 
                    s.snapshot_id,
                    s.snapshot_time,
                    s.total_gold,
                    s.active_stalls,
                    s.total_stall_items,
                    s.gini_coefficient
                FROM economy_snapshots s
                ORDER BY s.snapshot_time DESC
            ");
            $snapshot = $stmt->fetch();
            
            if (!$snapshot) {
                return [
                    'snapshot' => null,
                    'oversupply' => [],
                    'undersupply' => [],
                    'balanced' => []
                ];
            }
            
            // Categorize items by supply/demand ratio
            $itemsStmt = $db->prepare("
                SELECT 
                    item_id, total_supply, player_count, listed_for_sale,
                    scarcity_index, supply_demand_ratio
                FROM economy_item_snapshots
                WHERE snapshot_id = ? AND total_supply > 0
                ORDER BY total_supply DESC
            ");
            $itemsStmt->execute([$snapshot['snapshot_id']]);
            $items = $itemsStmt->fetchAll();
            
            $oversupply = [];   // ratio > 2 (lots of supply, low demand)
            $undersupply = [];  // ratio < 0.5 (scarce, high demand)
            $balanced = [];     // ratio 0.5-2
            
            $itemDb = getItemDatabase();
            
            foreach ($items as $item) {
                $itemInfo = $itemDb[$item['item_id']] ?? null;
                $item['name'] = $itemInfo['name'] ?? 'Unknown #' . $item['item_id'];
                $item['type_name'] = getItemTypeName($itemInfo['type'] ?? 0);
                
                $ratio = $item['supply_demand_ratio'];
                if ($ratio === null) {
                    $balanced[] = $item;
                } elseif ($ratio > 2) {
                    $oversupply[] = $item;
                } elseif ($ratio < 0.5) {
                    $undersupply[] = $item;
                } else {
                    $balanced[] = $item;
                }
            }
            
            return [
                'snapshot' => $snapshot,
                'oversupply' => array_slice($oversupply, 0, 20),
                'undersupply' => array_slice($undersupply, 0, 20),
                'balanced' => array_slice($balanced, 0, 20)
            ];
        } catch (PDOException $e) {
            error_log("Supply/demand summary failed: " . $e->getMessage());
            return [
                'snapshot' => null,
                'oversupply' => [],
                'undersupply' => [],
                'balanced' => []
            ];
        }
    }
}

/**
 * News Model - Handles news/announcements database operations
 */
class NewsModel
{
    /**
     * Get all news with pagination and optional category filter
     */
    public static function getAll(int $page = 1, int $perPage = 10, bool $publishedOnly = true, ?string $category = null): array
    {
        try {
            $db = Database::getWebDb();
            $offset = ($page - 1) * $perPage;
            
            $conditions = [];
            $params = [];
            
            if ($publishedOnly) {
                $conditions[] = "is_published = 1";
            }
            
            if ($category !== null && !empty($category)) {
                $conditions[] = "category = ?";
                $params[] = $category;
            }
            
            $whereClause = !empty($conditions) ? "WHERE " . implode(" AND ", $conditions) : "";
            
            $stmt = $db->prepare("
                SELECT id, title, slug, summary, image, category, author, 
                       is_published, is_pinned, views, created_at, updated_at
                FROM news
                {$whereClause}
                ORDER BY is_pinned DESC, created_at DESC
                OFFSET {$offset} ROWS FETCH NEXT {$perPage} ROWS ONLY
            ");
            $stmt->execute($params);
            $news = $stmt->fetchAll();
            
            // Get total count
            $countParams = $category !== null && !empty($category) ? [$category] : [];
            $countStmt = $db->prepare("SELECT COUNT(*) as total FROM news {$whereClause}");
            $countStmt->execute($countParams);
            $total = $countStmt->fetch()['total'];
            
            return [
                'news' => $news,
                'total' => $total,
                'page' => $page,
                'perPage' => $perPage,
                'totalPages' => ceil($total / $perPage)
            ];
        } catch (PDOException $e) {
            error_log("News getAll failed: " . $e->getMessage());
            return ['news' => [], 'total' => 0, 'page' => 1, 'perPage' => $perPage, 'totalPages' => 0];
        }
    }
    
    /**
     * Get latest news for homepage
     */
    public static function getLatest(int $limit = 5): array
    {
        try {
            $db = Database::getWebDb();
            $limit = (int)$limit; // Ensure integer
            $stmt = $db->prepare("
                SELECT TOP ({$limit}) id, title, slug, summary, image, category, author, 
                       is_pinned, views, created_at
                FROM news
                WHERE is_published = 1
                ORDER BY is_pinned DESC, created_at DESC
            ");
            $stmt->execute([]);
            return $stmt->fetchAll();
        } catch (PDOException $e) {
            error_log("News getLatest failed: " . $e->getMessage());
            return [];
        }
    }
    
    /**
     * Get news by ID
     */
    public static function getById(int $id): ?array
    {
        try {
            $db = Database::getWebDb();
            $stmt = $db->prepare("SELECT * FROM news WHERE id = ?");
            $stmt->execute([$id]);
            $result = $stmt->fetch();
            return $result ?: null;
        } catch (PDOException $e) {
            error_log("News getById failed: " . $e->getMessage());
            return null;
        }
    }
    
    /**
     * Get news by slug
     */
    public static function getBySlug(string $slug): ?array
    {
        try {
            $db = Database::getWebDb();
            $stmt = $db->prepare("SELECT * FROM news WHERE slug = ?");
            $stmt->execute([$slug]);
            $result = $stmt->fetch();
            return $result ?: null;
        } catch (PDOException $e) {
            error_log("News getBySlug failed: " . $e->getMessage());
            return null;
        }
    }
    
    /**
     * Create new news article
     */
    public static function create(array $data): ?int
    {
        try {
            $db = Database::getWebDb();
            $stmt = $db->prepare("
                INSERT INTO news (title, slug, summary, content, image, category, author, author_id, is_published, is_pinned)
                VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)
            ");
            $stmt->execute([
                $data['title'],
                $data['slug'],
                $data['summary'] ?? null,
                $data['content'],
                $data['image'] ?? null,
                $data['category'] ?? 'update',
                $data['author'],
                $data['author_id'],
                $data['is_published'] ?? 1,
                $data['is_pinned'] ?? 0
            ]);
            
            // Get the inserted ID
            $stmt = $db->query("SELECT SCOPE_IDENTITY() as id");
            $result = $stmt->fetch();
            return $result ? (int)$result['id'] : null;
        } catch (PDOException $e) {
            error_log("News create failed: " . $e->getMessage());
            return null;
        }
    }
    
    /**
     * Update news article
     */
    public static function update(int $id, array $data): bool
    {
        try {
            $db = Database::getWebDb();
            
            $fields = [];
            $values = [];
            
            $allowedFields = ['title', 'slug', 'summary', 'content', 'image', 'category', 'is_published', 'is_pinned'];
            
            foreach ($allowedFields as $field) {
                if (isset($data[$field])) {
                    $fields[] = "$field = ?";
                    $values[] = $data[$field];
                }
            }
            
            if (empty($fields)) {
                return false;
            }
            
            $fields[] = "updated_at = GETDATE()";
            $values[] = $id;
            
            $sql = "UPDATE news SET " . implode(', ', $fields) . " WHERE id = ?";
            $stmt = $db->prepare($sql);
            $stmt->execute($values);
            
            return $stmt->rowCount() > 0;
        } catch (PDOException $e) {
            error_log("News update failed: " . $e->getMessage());
            return false;
        }
    }
    
    /**
     * Delete news article
     */
    public static function delete(int $id): bool
    {
        try {
            $db = Database::getWebDb();
            $stmt = $db->prepare("DELETE FROM news WHERE id = ?");
            $stmt->execute([$id]);
            return $stmt->rowCount() > 0;
        } catch (PDOException $e) {
            error_log("News delete failed: " . $e->getMessage());
            return false;
        }
    }
    
    /**
     * Increment view count
     */
    public static function incrementViews(int $id): void
    {
        try {
            $db = Database::getWebDb();
            $stmt = $db->prepare("UPDATE news SET views = views + 1 WHERE id = ?");
            $stmt->execute([$id]);
        } catch (PDOException $e) {
            error_log("News incrementViews failed: " . $e->getMessage());
        }
    }
    
    /**
     * Generate unique slug from title
     */
    public static function generateSlug(string $title, ?int $excludeId = null): string
    {
        // Convert to lowercase, replace spaces with hyphens, remove special chars
        $slug = strtolower(trim($title));
        $slug = preg_replace('/[^a-z0-9\s-]/', '', $slug);
        $slug = preg_replace('/[\s-]+/', '-', $slug);
        $slug = trim($slug, '-');
        
        // Check if slug exists
        $baseSlug = $slug;
        $counter = 1;
        
        while (self::slugExists($slug, $excludeId)) {
            $slug = $baseSlug . '-' . $counter;
            $counter++;
        }
        
        return $slug;
    }
    
    /**
     * Check if slug exists
     */
    public static function slugExists(string $slug, ?int $excludeId = null): bool
    {
        try {
            $db = Database::getWebDb();
            
            if ($excludeId) {
                $stmt = $db->prepare("SELECT 1 FROM news WHERE slug = ? AND id != ?");
                $stmt->execute([$slug, $excludeId]);
            } else {
                $stmt = $db->prepare("SELECT 1 FROM news WHERE slug = ?");
                $stmt->execute([$slug]);
            }
            
            return $stmt->fetch() !== false;
        } catch (PDOException $e) {
            return false;
        }
    }
    
    /**
     * Get news categories
     */
    public static function getCategories(): array
    {
        return [
            'announcement' => ['name' => 'Announcement', 'icon' => '📢', 'color' => '#FFD700'],
            'update' => ['name' => 'Update', 'icon' => '🔄', 'color' => '#4CAF50'],
            'event' => ['name' => 'Event', 'icon' => '🎉', 'color' => '#E91E63'],
            'maintenance' => ['name' => 'Maintenance', 'icon' => '🔧', 'color' => '#FF9800'],
            'guide' => ['name' => 'Guide', 'icon' => '📖', 'color' => '#2196F3'],
            'community' => ['name' => 'Community', 'icon' => '👥', 'color' => '#9C27B0']
        ];
    }
}
