#include "include/server/db/redis_client.h"
#include "include/server/utils/logger.h"
#include <hiredis/hiredis.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <cstring>

namespace im {
namespace db {

RedisClient::RedisClient(const std::string& host, int port, const std::string& password)
    : host_(host), port_(port), password_(password), connected_(false), context_(nullptr) {
}

RedisClient::~RedisClient() {
    if (connected_) {
        Disconnect();
    }
}

bool RedisClient::Connect() {
    try {
        LOG_INFO("Connecting to Redis at {}:{}", host_, port_);
        
        // 连接到Redis服务器
        context_ = redisConnect(host_.c_str(), port_);
        if (!context_ || context_->err) {
            if (context_) {
                LOG_ERROR("Redis connection error: {}", context_->errstr);
                redisFree(context_);
                context_ = nullptr;
            } else {
                LOG_ERROR("Failed to allocate Redis context");
            }
            return false;
        }
        
        // 如果有密码，进行身份验证
        if (!password_.empty()) {
            redisReply* reply = (redisReply*)redisCommand(context_, "AUTH %s", password_.c_str());
            if (!reply) {
                LOG_ERROR("Redis authentication error: connection lost");
                Disconnect();
                return false;
            }
            
            if (reply->type == REDIS_REPLY_ERROR) {
                LOG_ERROR("Redis authentication failed: {}", reply->str);
                freeReplyObject(reply);
                Disconnect();
                return false;
            }
            
            freeReplyObject(reply);
        }
        
        connected_ = true;
        LOG_INFO("Successfully connected to Redis");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to connect to Redis: {}", e.what());
        return false;
    }
}

void RedisClient::Disconnect() {
    if (context_) {
        redisFree(context_);
        context_ = nullptr;
    }
    connected_ = false;
}

bool RedisClient::IsConnected() const {
    return connected_ && context_ && !context_->err;
}

bool RedisClient::SetValue(const std::string& key, const std::string& value, int expire_seconds) {
    if (!IsConnected()) {
        return false;
    }
    
    try {
        redisReply* reply = (redisReply*)redisCommand(context_, "SET %s %s", key.c_str(), value.c_str());
        if (!reply) {
            LOG_ERROR("Redis SET command failed: connection lost");
            return false;
        }
        
        bool success = (reply->type != REDIS_REPLY_ERROR);
        freeReplyObject(reply);
        
        if (success && expire_seconds > 0) {
            reply = (redisReply*)redisCommand(context_, "EXPIRE %s %d", key.c_str(), expire_seconds);
            if (!reply) {
                LOG_ERROR("Redis EXPIRE command failed: connection lost");
                return false;
            }
            
            success = (reply->type != REDIS_REPLY_ERROR);
            freeReplyObject(reply);
        }
        
        return success;
    } catch (const std::exception& e) {
        LOG_ERROR("Error setting value in Redis: {}", e.what());
        return false;
    }
}

std::string RedisClient::GetValue(const std::string& key) {
    if (!IsConnected()) {
        return "";
    }
    
    try {
        redisReply* reply = (redisReply*)redisCommand(context_, "GET %s", key.c_str());
        if (!reply) {
            LOG_ERROR("Redis GET command failed: connection lost");
            return "";
        }
        
        std::string value;
        if (reply->type == REDIS_REPLY_STRING) {
            value = std::string(reply->str, reply->len);
        } else if (reply->type == REDIS_REPLY_NIL) {
            // 键不存在
            value = "";
        } else {
            LOG_ERROR("Redis GET command returned unexpected type: {}", reply->type);
            value = "";
        }
        
        freeReplyObject(reply);
        return value;
    } catch (const std::exception& e) {
        LOG_ERROR("Error getting value from Redis: {}", e.what());
        return "";
    }
}

bool RedisClient::DeleteKey(const std::string& key) {
    if (!IsConnected()) {
        return false;
    }
    
    try {
        redisReply* reply = (redisReply*)redisCommand(context_, "DEL %s", key.c_str());
        if (!reply) {
            LOG_ERROR("Redis DEL command failed: connection lost");
            return false;
        }
        
        bool success = (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0);
        freeReplyObject(reply);
        return success;
    } catch (const std::exception& e) {
        LOG_ERROR("Error deleting key from Redis: {}", e.what());
        return false;
    }
}

bool RedisClient::KeyExists(const std::string& key) {
    if (!IsConnected()) {
        return false;
    }
    
    try {
        redisReply* reply = (redisReply*)redisCommand(context_, "EXISTS %s", key.c_str());
        if (!reply) {
            LOG_ERROR("Redis EXISTS command failed: connection lost");
            return false;
        }
        
        bool exists = (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0);
        freeReplyObject(reply);
        return exists;
    } catch (const std::exception& e) {
        LOG_ERROR("Error checking key existence in Redis: {}", e.what());
        return false;
    }
}

bool RedisClient::Expire(const std::string& key, int seconds) {
    if (!IsConnected()) {
        return false;
    }
    
    try {
        redisReply* reply = (redisReply*)redisCommand(context_, "EXPIRE %s %d", key.c_str(), seconds);
        if (!reply) {
            LOG_ERROR("Redis EXPIRE command failed: connection lost");
            return false;
        }
        
        bool success = (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0);
        freeReplyObject(reply);
        return success;
    } catch (const std::exception& e) {
        LOG_ERROR("Error setting expiration in Redis: {}", e.what());
        return false;
    }
}

int RedisClient::GetTTL(const std::string& key) {
    if (!IsConnected()) {
        return -2;
    }
    
    try {
        redisReply* reply = (redisReply*)redisCommand(context_, "TTL %s", key.c_str());
        if (!reply) {
            LOG_ERROR("Redis TTL command failed: connection lost");
            return -2;
        }
        
        int ttl = -2;
        if (reply->type == REDIS_REPLY_INTEGER) {
            ttl = static_cast<int>(reply->integer);
        }
        
        freeReplyObject(reply);
        return ttl;
    } catch (const std::exception& e) {
        LOG_ERROR("Error getting TTL from Redis: {}", e.what());
        return -2;
    }
}

int64_t RedisClient::PushBack(const std::string& key, const std::string& value) {
    if (!IsConnected()) {
        return -1;
    }
    
    try {
        redisReply* reply = (redisReply*)redisCommand(context_, "RPUSH %s %s", key.c_str(), value.c_str());
        if (!reply) {
            LOG_ERROR("Redis RPUSH command failed: connection lost");
            return -1;
        }
        
        int64_t length = -1;
        if (reply->type == REDIS_REPLY_INTEGER) {
            length = reply->integer;
        }
        
        freeReplyObject(reply);
        return length;
    } catch (const std::exception& e) {
        LOG_ERROR("Error pushing to list in Redis: {}", e.what());
        return -1;
    }
}

int64_t RedisClient::PushFront(const std::string& key, const std::string& value) {
    if (!IsConnected()) {
        return -1;
    }
    
    try {
        redisReply* reply = (redisReply*)redisCommand(context_, "LPUSH %s %s", key.c_str(), value.c_str());
        if (!reply) {
            LOG_ERROR("Redis LPUSH command failed: connection lost");
            return -1;
        }
        
        int64_t length = -1;
        if (reply->type == REDIS_REPLY_INTEGER) {
            length = reply->integer;
        }
        
        freeReplyObject(reply);
        return length;
    } catch (const std::exception& e) {
        LOG_ERROR("Error pushing to list in Redis: {}", e.what());
        return -1;
    }
}

std::vector<std::string> RedisClient::GetList(const std::string& key, int start, int end) {
    if (!IsConnected()) {
        return {};
    }
    
    try {
        redisReply* reply = (redisReply*)redisCommand(context_, "LRANGE %s %d %d", key.c_str(), start, end);
        if (!reply) {
            LOG_ERROR("Redis LRANGE command failed: connection lost");
            return {};
        }
        
        std::vector<std::string> result;
        if (reply->type == REDIS_REPLY_ARRAY) {
            for (size_t i = 0; i < reply->elements; i++) {
                if (reply->element[i]->type == REDIS_REPLY_STRING) {
                    result.emplace_back(reply->element[i]->str, reply->element[i]->len);
                }
            }
        }
        
        freeReplyObject(reply);
        return result;
    } catch (const std::exception& e) {
        LOG_ERROR("Error getting list from Redis: {}", e.what());
        return {};
    }
}

int64_t RedisClient::ListLength(const std::string& key) {
    if (!IsConnected()) {
        return 0;
    }
    
    try {
        redisReply* reply = (redisReply*)redisCommand(context_, "LLEN %s", key.c_str());
        if (!reply) {
            LOG_ERROR("Redis LLEN command failed: connection lost");
            return 0;
        }
        
        int64_t length = 0;
        if (reply->type == REDIS_REPLY_INTEGER) {
            length = reply->integer;
        }
        
        freeReplyObject(reply);
        return length;
    } catch (const std::exception& e) {
        LOG_ERROR("Error getting list length from Redis: {}", e.what());
        return 0;
    }
}

bool RedisClient::ListTrim(const std::string& key, int start, int end) {
    if (!IsConnected()) {
        return false;
    }
    
    try {
        redisReply* reply = (redisReply*)redisCommand(context_, "LTRIM %s %d %d", key.c_str(), start, end);
        if (!reply) {
            LOG_ERROR("Redis LTRIM command failed: connection lost");
            return false;
        }
        
        bool success = (reply->type == REDIS_REPLY_STATUS && strcmp(reply->str, "OK") == 0);
        freeReplyObject(reply);
        return success;
    } catch (const std::exception& e) {
        LOG_ERROR("Error trimming list in Redis: {}", e.what());
        return false;
    }
}

int64_t RedisClient::SetAdd(const std::string& key, const std::string& value) {
    if (!IsConnected()) {
        return 0;
    }
    
    try {
        redisReply* reply = (redisReply*)redisCommand(context_, "SADD %s %s", key.c_str(), value.c_str());
        if (!reply) {
            LOG_ERROR("Redis SADD command failed: connection lost");
            return 0;
        }
        
        int64_t count = 0;
        if (reply->type == REDIS_REPLY_INTEGER) {
            count = reply->integer;
        }
        
        freeReplyObject(reply);
        return count;
    } catch (const std::exception& e) {
        LOG_ERROR("Error adding to set in Redis: {}", e.what());
        return 0;
    }
}

int64_t RedisClient::SetRemove(const std::string& key, const std::string& value) {
    if (!IsConnected()) {
        return 0;
    }
    
    try {
        redisReply* reply = (redisReply*)redisCommand(context_, "SREM %s %s", key.c_str(), value.c_str());
        if (!reply) {
            LOG_ERROR("Redis SREM command failed: connection lost");
            return 0;
        }
        
        int64_t count = 0;
        if (reply->type == REDIS_REPLY_INTEGER) {
            count = reply->integer;
        }
        
        freeReplyObject(reply);
        return count;
    } catch (const std::exception& e) {
        LOG_ERROR("Error removing from set in Redis: {}", e.what());
        return 0;
    }
}

std::vector<std::string> RedisClient::SetMembers(const std::string& key) {
    if (!IsConnected()) {
        return {};
    }
    
    try {
        redisReply* reply = (redisReply*)redisCommand(context_, "SMEMBERS %s", key.c_str());
        if (!reply) {
            LOG_ERROR("Redis SMEMBERS command failed: connection lost");
            return {};
        }
        
        std::vector<std::string> result;
        if (reply->type == REDIS_REPLY_ARRAY) {
            for (size_t i = 0; i < reply->elements; i++) {
                if (reply->element[i]->type == REDIS_REPLY_STRING) {
                    result.emplace_back(reply->element[i]->str, reply->element[i]->len);
                }
            }
        }
        
        freeReplyObject(reply);
        return result;
    } catch (const std::exception& e) {
        LOG_ERROR("Error getting set members from Redis: {}", e.what());
        return {};
    }
}

bool RedisClient::Publish(const std::string& channel, const std::string& message) {
    if (!IsConnected()) {
        return false;
    }
    
    try {
        redisReply* reply = (redisReply*)redisCommand(context_, "PUBLISH %s %s", channel.c_str(), message.c_str());
        if (!reply) {
            LOG_ERROR("Redis PUBLISH command failed: connection lost");
            return false;
        }
        
        bool success = (reply->type == REDIS_REPLY_INTEGER);
        freeReplyObject(reply);
        return success;
    } catch (const std::exception& e) {
        LOG_ERROR("Error publishing message in Redis: {}", e.what());
        return false;
    }
}

bool RedisClient::SetHashField(const std::string& key, const std::string& field, const std::string& value) {
    if (!IsConnected()) {
        return false;
    }
    
    try {
        redisReply* reply = (redisReply*)redisCommand(context_, "HSET %s %s %s", key.c_str(), field.c_str(), value.c_str());
        if (!reply) {
            LOG_ERROR("Redis HSET command failed: connection lost");
            return false;
        }
        
        bool success = (reply->type == REDIS_REPLY_INTEGER);
        freeReplyObject(reply);
        return success;
    } catch (const std::exception& e) {
        LOG_ERROR("Error setting hash field in Redis: {}", e.what());
        return false;
    }
}

std::string RedisClient::GetHashField(const std::string& key, const std::string& field) {
    if (!IsConnected()) {
        return "";
    }
    
    try {
        redisReply* reply = (redisReply*)redisCommand(context_, "HGET %s %s", key.c_str(), field.c_str());
        if (!reply) {
            LOG_ERROR("Redis HGET command failed: connection lost");
            return "";
        }
        
        std::string value;
        if (reply->type == REDIS_REPLY_STRING) {
            value = std::string(reply->str, reply->len);
        } else if (reply->type == REDIS_REPLY_NIL) {
            // 字段不存在
            value = "";
        } else {
            LOG_ERROR("Redis HGET command returned unexpected type: {}", reply->type);
            value = "";
        }
        
        freeReplyObject(reply);
        return value;
    } catch (const std::exception& e) {
        LOG_ERROR("Error getting hash field from Redis: {}", e.what());
        return "";
    }
}

bool RedisClient::DeleteHashField(const std::string& key, const std::string& field) {
    if (!IsConnected()) {
        return false;
    }
    
    try {
        redisReply* reply = (redisReply*)redisCommand(context_, "HDEL %s %s", key.c_str(), field.c_str());
        if (!reply) {
            LOG_ERROR("Redis HDEL command failed: connection lost");
            return false;
        }
        
        bool success = (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0);
        freeReplyObject(reply);
        return success;
    } catch (const std::exception& e) {
        LOG_ERROR("Error deleting hash field from Redis: {}", e.what());
        return false;
    }
}

bool RedisClient::SetHashValues(
    const std::string& key,
    const std::map<std::string, std::string>& fields,
    int expire_seconds) {
    if (!IsConnected() || fields.empty()) {
        return false;
    }
    
    try {
        // 构建HMSET命令
        std::stringstream cmd;
        cmd << "HMSET " << key;
        for (const auto& pair : fields) {
            cmd << " " << pair.first << " " << pair.second;
        }
        
        redisReply* reply = (redisReply*)redisCommand(context_, cmd.str().c_str());
        if (!reply) {
            LOG_ERROR("Redis HMSET command failed: connection lost");
            return false;
        }
        
        bool success = (reply->type == REDIS_REPLY_STATUS && strcmp(reply->str, "OK") == 0);
        freeReplyObject(reply);
        
        if (success && expire_seconds > 0) {
            reply = (redisReply*)redisCommand(context_, "EXPIRE %s %d", key.c_str(), expire_seconds);
            if (!reply) {
                LOG_ERROR("Redis EXPIRE command failed: connection lost");
                return false;
            }
            
            success = (reply->type == REDIS_REPLY_INTEGER && reply->integer > 0);
            freeReplyObject(reply);
        }
        
        return success;
    } catch (const std::exception& e) {
        LOG_ERROR("Error setting hash values in Redis: {}", e.what());
        return false;
    }
}

std::map<std::string, std::string> RedisClient::GetHashAll(const std::string& key) {
    if (!IsConnected()) {
        return {};
    }
    
    try {
        redisReply* reply = (redisReply*)redisCommand(context_, "HGETALL %s", key.c_str());
        if (!reply) {
            LOG_ERROR("Redis HGETALL command failed: connection lost");
            return {};
        }
        
        std::map<std::string, std::string> result;
        if (reply->type == REDIS_REPLY_ARRAY) {
            for (size_t i = 0; i < reply->elements; i += 2) {
                if (i + 1 < reply->elements) {
                    std::string field(reply->element[i]->str, reply->element[i]->len);
                    std::string value(reply->element[i + 1]->str, reply->element[i + 1]->len);
                    result[field] = value;
                }
            }
        }
        
        freeReplyObject(reply);
        return result;
    } catch (const std::exception& e) {
        LOG_ERROR("Error getting hash all from Redis: {}", e.what());
        return {};
    }
}

int64_t RedisClient::Increment(const std::string& key, int64_t increment) {
    if (!IsConnected()) {
        return -1;
    }
    
    try {
        redisReply* reply = (redisReply*)redisCommand(context_, "INCRBY %s %lld", key.c_str(), increment);
        if (!reply) {
            LOG_ERROR("Redis INCRBY command failed: connection lost");
            return -1;
        }
        
        int64_t value = -1;
        if (reply->type == REDIS_REPLY_INTEGER) {
            value = reply->integer;
        }
        
        freeReplyObject(reply);
        return value;
    } catch (const std::exception& e) {
        LOG_ERROR("Error incrementing value in Redis: {}", e.what());
        return -1;
    }
}

int64_t RedisClient::Decrement(const std::string& key, int64_t decrement) {
    if (!IsConnected()) {
        return -1;
    }
    
    try {
        redisReply* reply = (redisReply*)redisCommand(context_, "DECRBY %s %lld", key.c_str(), decrement);
        if (!reply) {
            LOG_ERROR("Redis DECRBY command failed: connection lost");
            return -1;
        }
        
        int64_t value = -1;
        if (reply->type == REDIS_REPLY_INTEGER) {
            value = reply->integer;
        }
        
        freeReplyObject(reply);
        return value;
    } catch (const std::exception& e) {
        LOG_ERROR("Error decrementing value in Redis: {}", e.what());
        return -1;
    }
}

} // namespace db
} // namespace im 

