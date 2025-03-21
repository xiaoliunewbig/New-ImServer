#include "server/db/mysql_connection.h"
#include <stdexcept>
#include <sstream>
#include "server/utils/logger.h"

namespace im {
namespace db {

MySQLConnection::MySQLConnection(
    const std::string& host,
    int port,
    const std::string& user,
    const std::string& password,
    const std::string& database
) : host_(host),
    port_(port),
    user_(user),
    password_(password),
    database_(database),
    mysql_(nullptr),
    connected_(false) {
}

MySQLConnection::~MySQLConnection() {
    Disconnect();
}

bool MySQLConnection::Connect() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (connected_) {
        return true;
    }

    mysql_ = mysql_init(nullptr);
    if (!mysql_) {
        last_error_ = "Failed to initialize MySQL connection";
        LogError("Failed to initialize MySQL connection");
        return false;
    }

    // 设置自动重连
    bool reconnect = 1;
    mysql_options(mysql_, MYSQL_OPT_RECONNECT, &reconnect);

    // 设置连接超时
    int timeout = 5;
    mysql_options(mysql_, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);

    // 连接数据库
    if (!mysql_real_connect(mysql_, host_.c_str(), user_.c_str(), password_.c_str(), 
                          database_.c_str(), port_, nullptr, 0)) {
        last_error_ = mysql_error(mysql_);
        LogError("Failed to connect to MySQL: " + last_error_);
        mysql_close(mysql_);
        mysql_ = nullptr;
        return false;
    }

    // 设置字符集
    mysql_set_character_set(mysql_, "utf8mb4");

    connected_ = true;
    LogDebug("Connected to MySQL database: " + database_);
    return true;
}

void MySQLConnection::Disconnect() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (mysql_) {
        mysql_close(mysql_);
        mysql_ = nullptr;
    }
    
    connected_ = false;
}

bool MySQLConnection::IsConnected() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!mysql_ || !connected_) {
        return false;
    }
    
    // 使用mysql_ping检查连接是否有效
    return mysql_ping(mysql_) == 0;
}

MySQLConnection::ResultSet MySQLConnection::ExecuteQuery(
    const std::string& sql, 
    const std::vector<std::string>& params
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!CheckAndReconnect()) {
        throw std::runtime_error("Failed to connect to database: " + last_error_);
    }
    
    try {
        MYSQL_STMT* stmt = PrepareStatement(sql, params);
        if (!stmt) {
            throw std::runtime_error("Failed to prepare statement: " + last_error_);
        }
        
        // 执行语句
        if (mysql_stmt_execute(stmt) != 0) {
            last_error_ = mysql_stmt_error(stmt);
            LogError("Failed to execute query: " + last_error_);
            mysql_stmt_close(stmt);
            throw std::runtime_error("Failed to execute query: " + last_error_);
        }
        
        // 获取结果
        ResultSet results = FetchResults(stmt);
        
        // 关闭语句
        mysql_stmt_close(stmt);
        
        return results;
    } catch (const std::exception& e) {
        LogError("ExecuteQuery exception: " + std::string(e.what()));
        throw;
    }
}

bool MySQLConnection::ExecuteUpdate(
    const std::string& sql, 
    const std::vector<std::string>& params
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!CheckAndReconnect()) {
        LogError("Failed to connect to database: " + last_error_);
        return false;
    }
    
    try {
        MYSQL_STMT* stmt = PrepareStatement(sql, params);
        if (!stmt) {
            LogError("Failed to prepare statement: " + last_error_);
            return false;
        }
        
        // 执行语句
        if (mysql_stmt_execute(stmt) != 0) {
            last_error_ = mysql_stmt_error(stmt);
            LogError("Failed to execute update: " + last_error_);
            mysql_stmt_close(stmt);
            return false;
        }
        
        // 获取影响的行数
        my_ulonglong affected_rows = mysql_stmt_affected_rows(stmt);
        
        // 关闭语句
        mysql_stmt_close(stmt);
        
        return affected_rows > 0;
    } catch (const std::exception& e) {
        LogError("ExecuteUpdate exception: " + std::string(e.what()));
        return false;
    }
}

int64_t MySQLConnection::ExecuteInsert(
    const std::string& sql, 
    const std::vector<std::string>& params
) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!CheckAndReconnect()) {
        LogError("Failed to connect to database: " + last_error_);
        return 0;
    }
    
    try {
        MYSQL_STMT* stmt = PrepareStatement(sql, params);
        if (!stmt) {
            LogError("Failed to prepare statement: " + last_error_);
            return 0;
        }
        
        // 执行语句
        if (mysql_stmt_execute(stmt) != 0) {
            last_error_ = mysql_stmt_error(stmt);
            LogError("Failed to execute insert: " + last_error_);
            mysql_stmt_close(stmt);
            return 0;
        }
        
        // 获取自增ID
        my_ulonglong insert_id = mysql_stmt_insert_id(stmt);
        
        // 关闭语句
        mysql_stmt_close(stmt);
        
        return static_cast<int64_t>(insert_id);
    } catch (const std::exception& e) {
        LogError("ExecuteInsert exception: " + std::string(e.what()));
        return 0;
    }
}

bool MySQLConnection::BeginTransaction() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!CheckAndReconnect()) {
        LogError("Failed to connect to database: " + last_error_);
        return false;
    }
    
    if (mysql_query(mysql_, "START TRANSACTION") != 0) {
        last_error_ = mysql_error(mysql_);
        LogError("Failed to begin transaction: " + last_error_);
        return false;
    }
    
    return true;
}

bool MySQLConnection::CommitTransaction() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!CheckAndReconnect()) {
        LogError("Failed to connect to database: " + last_error_);
        return false;
    }
    
    if (mysql_query(mysql_, "COMMIT") != 0) {
        last_error_ = mysql_error(mysql_);
        LogError("Failed to commit transaction: " + last_error_);
        return false;
    }
    
    return true;
}

bool MySQLConnection::RollbackTransaction() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!CheckAndReconnect()) {
        LogError("Failed to connect to database: " + last_error_);
        return false;
    }
    
    if (mysql_query(mysql_, "ROLLBACK") != 0) {
        last_error_ = mysql_error(mysql_);
        LogError("Failed to rollback transaction: " + last_error_);
        return false;
    }
    
    return true;
}

std::string MySQLConnection::GetLastError() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return last_error_;
}

MYSQL_STMT* MySQLConnection::PrepareStatement(
    const std::string& sql, 
    const std::vector<std::string>& params
) {
    MYSQL_STMT* stmt = mysql_stmt_init(mysql_);
    if (!stmt) {
        last_error_ = mysql_error(mysql_);
        LogError("Failed to initialize statement: " + last_error_);
        return nullptr;
    }
    
    if (mysql_stmt_prepare(stmt, sql.c_str(), sql.length()) != 0) {
        last_error_ = mysql_stmt_error(stmt);
        LogError("Failed to prepare statement: " + last_error_);
        mysql_stmt_close(stmt);
        return nullptr;
    }
    
    // 检查参数数量是否匹配
    unsigned long param_count = mysql_stmt_param_count(stmt);
    if (param_count != params.size()) {
        std::ostringstream oss;
        oss << "Parameter count mismatch: expected " << param_count << ", got " << params.size();
        last_error_ = oss.str();
        LogError(last_error_);
        mysql_stmt_close(stmt);
        return nullptr;
    }
    
    // 如果有参数，绑定它们
    if (param_count > 0) {
        if (!BindParams(stmt, params)) {
            mysql_stmt_close(stmt);
            return nullptr;
        }
    }
    
    return stmt;
}

bool MySQLConnection::BindParams(
    MYSQL_STMT* stmt, 
    const std::vector<std::string>& params
) {
    unsigned long param_count = mysql_stmt_param_count(stmt);
    
    // 准备绑定数组
    std::vector<MYSQL_BIND> binds(param_count);
    std::vector<unsigned long> lengths(param_count);
    
    // 初始化绑定数组
    for (unsigned long i = 0; i < param_count; i++) {
        const std::string& param = params[i];
        lengths[i] = param.length();
        
        memset(&binds[i], 0, sizeof(MYSQL_BIND));
        binds[i].buffer_type = MYSQL_TYPE_STRING;
        binds[i].buffer = const_cast<char*>(param.c_str());
        binds[i].buffer_length = lengths[i];
        binds[i].length = &lengths[i];
        binds[i].is_null = 0;
    }
    
    // 绑定参数
    if (mysql_stmt_bind_param(stmt, binds.data()) != 0) {
        last_error_ = mysql_stmt_error(stmt);
        LogError("Failed to bind parameters: " + last_error_);
        return false;
    }
    
    return true;
}

MySQLConnection::ResultSet MySQLConnection::FetchResults(MYSQL_STMT* stmt) {
    ResultSet results;
    
    // 获取结果集元数据
    MYSQL_RES* meta = mysql_stmt_result_metadata(stmt);
    if (!meta) {
        // 可能是一个不返回结果的语句（如INSERT, UPDATE等）
        return results;
    }
    
    // 获取列数
    int num_fields = mysql_num_fields(meta);
    
    // 准备绑定数组
    std::vector<MYSQL_BIND> binds(num_fields);
    std::vector<std::vector<char>> buffers(num_fields);
    std::vector<unsigned long> lengths(num_fields);
    // 使用char数组代替std::vector<bool>以解决地址问题
    std::vector<char> is_nulls(num_fields, 0);
    std::vector<char> errors(num_fields, 0);
    
    // 获取列名
    std::vector<std::string> field_names;
    MYSQL_FIELD* fields = mysql_fetch_fields(meta);
    for (int i = 0; i < num_fields; i++) {
        field_names.push_back(fields[i].name);
        
        // 为每一列分配足够大的缓冲区
        buffers[i].resize(8192);
        
        memset(&binds[i], 0, sizeof(MYSQL_BIND));
        binds[i].buffer_type = MYSQL_TYPE_STRING;
        binds[i].buffer = buffers[i].data();
        binds[i].buffer_length = buffers[i].size() - 1; // 留一个字节给终止符
        binds[i].length = &lengths[i];
        binds[i].is_null = (bool*)&is_nulls[i];
        binds[i].error = (bool*)&errors[i];
    }
    
    // 绑定结果集
    if (mysql_stmt_bind_result(stmt, binds.data()) != 0) {
        last_error_ = mysql_stmt_error(stmt);
        LogError("Failed to bind result: " + last_error_);
        mysql_free_result(meta);
        return results;
    }
    
    // 存储结果集
    if (mysql_stmt_store_result(stmt) != 0) {
        last_error_ = mysql_stmt_error(stmt);
        LogError("Failed to store result: " + last_error_);
        mysql_free_result(meta);
        return results;
    }
    
    // 获取行数
    my_ulonglong num_rows = mysql_stmt_num_rows(stmt);
    
    // 提取结果
    while (mysql_stmt_fetch(stmt) == 0) {
        Row row;
        for (int i = 0; i < num_fields; i++) {
            if (is_nulls[i]) {
                row[field_names[i]] = "NULL";
            } else {
                if (errors[i]) {
                    std::vector<char> large_buffer(lengths[i] + 1);
                    
                    MYSQL_BIND bind;
                    memset(&bind, 0, sizeof(MYSQL_BIND));
                    bind.buffer_type = MYSQL_TYPE_STRING;
                    bind.buffer = large_buffer.data();
                    bind.buffer_length = large_buffer.size() - 1;
                    bind.length = &lengths[i];
                    
                    if (mysql_stmt_fetch_column(stmt, &bind, i, 0) != 0) {
                        last_error_ = mysql_stmt_error(stmt);
                        LogError("Failed to fetch column: " + last_error_);
                        row[field_names[i]] = "";
                    } else {
                        large_buffer[lengths[i]] = '\0';
                        row[field_names[i]] = large_buffer.data();
                    }
                } else {
                    buffers[i][lengths[i]] = '\0';
                    row[field_names[i]] = buffers[i].data();
                }
            }
        }
        results.push_back(row);
    }
    
    mysql_free_result(meta);
    return results;
}

bool MySQLConnection::CheckAndReconnect() {
    if (connected_ && mysql_) {
        if (mysql_ping(mysql_) == 0) {
            return true;
        }
        
        // 连接已经断开，尝试重连
        LogDebug("MySQL connection lost, attempting to reconnect...");
        connected_ = false;
    }
    
    return Connect();
}

void MySQLConnection::LogDebug(const std::string& message) const {
    LOG_DEBUG("[MySQL] {}", message);
}

void MySQLConnection::LogError(const std::string& message) const {
    LOG_ERROR("[MySQL] {}", message);
}

} // namespace db
} // namespace im 