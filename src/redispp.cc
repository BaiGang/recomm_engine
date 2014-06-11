#include "redispp.h"

#include <time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <boost/format.hpp>
#include "hiredis/hiredis.h"

namespace redis {

Client::Client(const std::string &ip, int port) {
  ip_ = ip;
  port_ = port;
  ctx_ = NULL;
}

Client::~Client() {
  if (ctx_ != NULL) {
    Disconnect();
  }
}

bool Client::Connect() {
  last_error_.clear();

  if (ctx_ != NULL) {
    last_error_ = "redis is already connected.";
    return false;
  }

  timeval t;
  t.tv_sec = 5;
  t.tv_usec = 0;

  ctx_ = redisConnectWithTimeout(ip_.c_str(), port_, t);
  if (ctx_->err == 0) {
    if (redisSetTimeout(ctx_, t) == REDIS_ERR) {
      LOG(WARNING) << "failed to set redis timeout";
    }
    return true;
  } else {
    last_error_ = ctx_->errstr;
    redisFree(ctx_);
    ctx_ = NULL;
    return false;
  }
}

bool Client::Disconnect() {
  last_error_.clear();

  if (ctx_ == NULL) {
    last_error_ = "redis not connected";
    return false;
  }
  redisFree(ctx_);
  ctx_ = NULL;
  return true;
}

bool Client::Reconnect() {
  last_error_.clear();

  Disconnect();
  return Connect();
}

bool Client::Ping() {
  last_error_.clear();

  if (ctx_ == NULL) {
    last_error_ = "redis not connected";
    return false;
  }
  redisReply *reply = reinterpret_cast<redisReply *>(redisCommand(ctx_, "PING"));
  if (reply == NULL) {
    last_error_ = ctx_->errstr;
    return false;
  }

  if (reply->len == (int)strlen("PONG") && strcmp("PONG", reply->str) == 0u) {
    DVLOG(1) << reply->type << ", " << std::string(reply->str, reply->len);
    freeReplyObject(reply);
    return true;
  }

  last_error_ = "unknown reply for PING: ";
  last_error_.append(reply->str, reply->len);

  freeReplyObject(reply);
  return false;
}

bool Client::SetReadWriteTimeOut(int timeout_in_ms) {
  if (ctx_ == NULL) {
    LOG(WARNING) << "Should call SetReadWriteTimeOut after Connect";
    return false;
  }
  int second = timeout_in_ms / 1000;
  int ms = timeout_in_ms % 1000;
  timeval t;
  t.tv_sec = second;
  t.tv_usec = ms * 1000;
  if (redisSetTimeout(ctx_, t) == REDIS_ERR) {
    LOG(WARNING) << "failed to set redis timeout";
    return false;
  }
  return true;
}

bool Client::SetEx(const char *key, int key_len, const char *value, int value_len, int life_in_seconds) {
  last_error_.clear();

  if (ctx_ == NULL) {
    last_error_ = "redis not connected";
    return false;
  }

  redisReply *reply = NULL;
  if (life_in_seconds > 0) {
    reply = reinterpret_cast<redisReply *>(
        redisCommand(ctx_, "SETEX %b %d %b", key, (int64)key_len,
                     (int64)life_in_seconds, value, (int64)value_len));
  } else {
    reply = reinterpret_cast<redisReply *>(
        redisCommand(ctx_, "SET %b %b", key, (int64)key_len, value, (int64)value_len));
  }
  if (reply == NULL) {
    last_error_ = ctx_->errstr;
    return false;
  }
  if (reply->type != REDIS_REPLY_STATUS || strncmp("OK", reply->str, reply->len) != 0) {
      // TODO(suhua): better last error
    last_error_ = str(boost::format("unexpected reply, type: %1%") % reply->type);
    freeReplyObject(reply);
    return false;
  }

  freeReplyObject(reply);
  return true;
}

// TODO(suhua): 每次调用前, 清空 last_error();
int Client::Get(const char *key, int key_len, std::string *value) {
  last_error_.clear();

  value->clear();
  if (ctx_ == NULL) {
    last_error_ = "redis not connected";
    return -1;
  }

  redisReply *reply =
      reinterpret_cast<redisReply *>(redisCommand(ctx_, "GET %b", key, (int64)key_len));
  if (reply == NULL) {
    last_error_ = ctx_->errstr;
    return -1;
  }

  if (reply->type == REDIS_REPLY_ERROR) {
    last_error_.assign(reply->str, reply->len);
    freeReplyObject(reply);
    return -1;
  } else if (reply->type == REDIS_REPLY_STRING) {
    value->assign(reply->str, reply->len);
    freeReplyObject(reply);
    return 0;
  } else if (reply->type == REDIS_REPLY_NIL) {
    last_error_ = "the key doesn't exist";
    freeReplyObject(reply);
    return 1;
  }

  freeReplyObject(reply);
  return -1;
}

int Client::Delete(const char *key, int key_len) {
  last_error_.clear();

  if (ctx_ == NULL) {
    last_error_ = "redis not connected";
    return -1;
  }

  redisReply *reply =
      reinterpret_cast<redisReply *>(redisCommand(ctx_, "DEL %b", key, (int64)key_len));
  if (reply == NULL) {
    last_error_ = ctx_->errstr;
    return -1;
  }

  if (reply->type == REDIS_REPLY_INTEGER) {
    LOG(INFO) << "Deleted " << reply->integer << " keys in redis.";
    freeReplyObject(reply);
    return 0;
  } else if (reply->type == REDIS_REPLY_ERROR) {
    last_error_.assign(reply->str, reply->len);
    freeReplyObject(reply);
    return -1;
  }

  freeReplyObject(reply);
  return -1;
}

bool Client::MultiSetEx(const std::vector<std::string> &keys,
                        const std::vector<std::string> &values,
                        int life_in_seconds) {
  std::vector<int> multi_life_in_seconds(keys.size(), life_in_seconds);
  return MultiSetEx(keys, values, multi_life_in_seconds);
}

bool Client::MultiSetEx(const std::vector<std::string> &keys,
                        const std::vector<std::string> &values,
                        const std::vector<int> &life_in_seconds) {
  last_error_.clear();

  if (ctx_ == NULL) {
    last_error_ = "redis not connected";
    return false;
  }

  if (keys.size() != values.size()) {
    last_error_ = "#keys != #values";
    return false;
  }

  // 启动 transaction
  if (redisAppendCommand(ctx_, "MULTI") != REDIS_OK) {
    last_error_ = ctx_->errstr;
    return false;
  }
  // 多次 transaction set
  for (int i = 0; i < (int)keys.size(); ++i) {
    if (life_in_seconds[i] > 0) {
      if (redisAppendCommand(ctx_, "SETEX %b %d %b",
                             keys[i].data(), keys[i].size(), life_in_seconds[i],
                             values[i].data(), values[i].size()) != REDIS_OK) {
        last_error_ = ctx_->errstr;
        return false;
      }
    } else {
      if (redisAppendCommand(ctx_, "SET %b %b",
                             keys[i].data(), keys[i].size(),
                             values[i].data(), values[i].size()) != REDIS_OK) {
        last_error_ = ctx_->errstr;
        return false;
      }
    }
  }
  // 执行 EXEC, 并取回 MULTI 的 reply
  redisReply *reply = NULL;
  reply = reinterpret_cast<redisReply *>(redisCommand(ctx_, "EXEC"));
  if (reply == NULL) {
    last_error_ = ctx_->errstr;
    return false;
  }
  freeReplyObject(reply);

  // 取多个 SET 的回复
  for (int i = 0; i < (int)keys.size(); ++i) {
    redisReply *reply = NULL;
    redisGetReply(ctx_, reinterpret_cast<void **>(&reply));
    if (reply == NULL) {
      last_error_ = ctx_->errstr;
      return false;
    }
    if (reply->type != REDIS_REPLY_STATUS || strncmp("QUEUED", reply->str, reply->len) != 0) {
      // TODO(suhua): better last error
      last_error_ = str(boost::format("unexpected reply, type: %1%") % reply->type);
      freeReplyObject(reply);
      return false;
    }
    freeReplyObject(reply);
  }

  // 取 EXEC 的回复
  reply = NULL;
  redisGetReply(ctx_, reinterpret_cast<void **>(&reply));
  if (reply == NULL) {
    last_error_ = ctx_->errstr;
    return false;
  }
  if (reply->type != REDIS_REPLY_ARRAY) {
    last_error_ = str(boost::format("unexpected reply, type: %1%") % reply->type);
    freeReplyObject(reply);
    return false;
  }

  last_error_.assign(reply->str, reply->len);
  freeReplyObject(reply);
  return true;
}

bool Client::MultiGet(const std::vector<std::string> &keys,
                      std::vector<std::string> *values,
                      std::vector<int> *ret_codes) {
  last_error_.clear();

  values->clear();
  ret_codes->clear();
  values->resize(keys.size());
  ret_codes->resize(keys.size(), -1);

  if (ctx_ == NULL) {
    last_error_ = "redis not connected";
    return false;
  }

  // 启动 transaction
  if (redisAppendCommand(ctx_, "MULTI") != REDIS_OK) {
    last_error_ = ctx_->errstr;
    return false;
  }
  // 多次 transaction get
  for (int i = 0; i < (int)keys.size(); ++i) {
    if (redisAppendCommand(ctx_, "GET %b", keys[i].data(), keys[i].size()) != REDIS_OK) {
      last_error_ = ctx_->errstr;
      return false;
    }
  }
  // 执行 EXEC, 并取回 MULTI 的 reply
  redisReply *reply = NULL;
  reply = reinterpret_cast<redisReply *>(redisCommand(ctx_, "EXEC"));
  if (reply == NULL) {
    last_error_ = ctx_->errstr;
    return false;
  }
  freeReplyObject(reply);

  // 取多个 GET 的回复
  for (int i = 0; i < (int)keys.size(); ++i) {
    redisReply *reply = NULL;
    redisGetReply(ctx_, reinterpret_cast<void **>(&reply));
    if (reply == NULL) {
      last_error_ = ctx_->errstr;
      return false;
    }
    if (reply->type != REDIS_REPLY_STATUS || strncmp("QUEUED", reply->str, reply->len) != 0) {
      last_error_ = str(boost::format("unexpected reply, type: %1%") % reply->type);
      freeReplyObject(reply);
      return false;
    }
    freeReplyObject(reply);
  }

  // 取 EXEC 的回复
  reply = NULL;
  redisGetReply(ctx_, reinterpret_cast<void **>(&reply));
  if (reply == NULL) {
    last_error_ = ctx_->errstr;
    return false;
  }
  if (reply->type != REDIS_REPLY_ARRAY || reply->elements != keys.size()) {
    last_error_ = str(boost::format("unexpected reply, type: %1%") % reply->type);
    freeReplyObject(reply);
    return false;
  }
  CHECK_EQ(reply->elements, keys.size());

  int ok_num = 0;
  for (int i = 0; i < (int)reply->elements; ++i) {
    redisReply *r = reinterpret_cast<redisReply *>(reply->element[i]);
    if (r->type == REDIS_REPLY_ERROR) {
      values->at(i) = "";
      ret_codes->at(i) = -1;
      last_error_.assign(r->str, r->len);
    } else if (r->type == REDIS_REPLY_STRING) {
      values->at(i).assign(r->str, r->len);
      ret_codes->at(i) = 0;
      ++ok_num;
    } else if (r->type == REDIS_REPLY_NIL) {
      values->at(i) = "";
      ret_codes->at(i) = 1;
      last_error_ = "the key doesn't exist";
    } else {
      // NOT_REACHED();
    }
  }

  last_error_.assign(reply->str, reply->len);
  freeReplyObject(reply);
  return ok_num == (int)keys.size();
}

bool Client::HashSetEx(const char *key, int key_len,
                       const char *field, int field_len,
                       const char *value, int value_len,
                       int life_in_seconds) {
  last_error_.clear();

  // TODO(将超时单独设置)

  if (ctx_ == NULL) {
    last_error_ = "redis not connected";
    return false;
  }

  redisReply *reply = NULL;
  // XXX(suhua): 这里修了一个神奇的 bug, 如果不把 xx_len 强转成 int64, 直接传入
  // int 值, 最后一个 value_len 在库函数里会被读成一个 8 字节整数, 其中一半是
  // value_len, 另一半是后面未定义的 4 个字节.  传入前将 value_len 转换成 8
  // 字节的 int64 解决了该问题. 具体原因仍未查清楚, 需要研究一下变参函数
  // 的调用规范.
  reply = reinterpret_cast<redisReply *>(
      redisCommand(ctx_, "HSET %b %b %b", key, (int64)key_len,
                   field, (int64)field_len, value, (int64)value_len));
  if (reply == NULL) {
    last_error_ = ctx_->errstr;
    return false;
  }
  if (reply->type != REDIS_REPLY_INTEGER || (reply->integer != 0 && reply->integer != 1)) {
    last_error_ = str(boost::format("unexpected reply, type: %1%") % reply->type);
    freeReplyObject(reply);
    return false;
  }
  freeReplyObject(reply);

  if (life_in_seconds <= 0) {
    return true;
  }

  // 修正超时时间
  reply = reinterpret_cast<redisReply *>(
      redisCommand(ctx_, "EXPIRE %b %d", key, (int64)key_len, (int64)life_in_seconds));
  if (reply == NULL) {
    last_error_ = ctx_->errstr;
    return false;
  }
  if (reply->type != REDIS_REPLY_INTEGER || reply->integer != 1) {
    last_error_ = str(boost::format("unexpected reply, type: %1%") % reply->type);
    freeReplyObject(reply);
    return false;
  }

  freeReplyObject(reply);
  return true;
}

int Client::HashGet(const char *key, int key_len,
                    const char *field, int field_len,
                    std::string *value) {
  last_error_.clear();

  value->clear();
  if (ctx_ == NULL) {
    last_error_ = "redis not connected";
    return -1;
  }

  redisReply *reply =
      reinterpret_cast<redisReply *>(
          redisCommand(ctx_, "HGET %b %b", key, (int64)key_len, field, (int64)field_len));
  if (reply == NULL) {
    last_error_ = ctx_->errstr;
    return -1;
  }

  if (reply->type == REDIS_REPLY_ERROR) {
    last_error_.assign(reply->str, reply->len);
    freeReplyObject(reply);
    return -1;
  } else if (reply->type == REDIS_REPLY_STRING) {
    value->assign(reply->str, reply->len);
    freeReplyObject(reply);
    return 0;
  } else if (reply->type == REDIS_REPLY_NIL) {
    last_error_ = "the key-field doesn't exist";
    freeReplyObject(reply);
    return 1;
  }

  // NOT_REACHED() << reply->type;
  freeReplyObject(reply);
  return -1;
}

int Client::HashGetAllFields(const char *key, int key_len, std::map<std::string, std::string> *field_values) {
  last_error_.clear();

  field_values->clear();
  if (ctx_ == NULL) {
    last_error_ = "redis not connected";
    return -1;
  }

  redisReply *reply =
      reinterpret_cast<redisReply *>(redisCommand(ctx_, "HGETALL %b", key, (int64)key_len));
  if (reply == NULL) {
    last_error_ = ctx_->errstr;
    return -1;
  }

  if (reply->type != REDIS_REPLY_ARRAY) {
    last_error_ = str(boost::format("unexpected reply, type: %1%") % reply->type);
    freeReplyObject(reply);
    return -1;
  }

  if (reply->elements == 0) {
    last_error_ = "the key dosen't exist";
    freeReplyObject(reply);
    return 1;
  }

  CHECK_EQ(reply->elements % 2, 0u);
  for (int i = 0; i < (int)reply->elements; i+=2) {
    redisReply *rf = reinterpret_cast<redisReply *>(reply->element[i]);
    redisReply *rv = reinterpret_cast<redisReply *>(reply->element[i+1]);
    CHECK_EQ(rf->type, REDIS_REPLY_STRING);
    CHECK_EQ(rv->type, REDIS_REPLY_STRING);
    (*field_values)[std::string(rf->str, rf->len)] = std::string(rv->str, rv->len);
  }

  freeReplyObject(reply);
  return 0;
}

bool Client::MultiHashSetEx(const std::vector<std::string> &keys,
                            const std::vector<std::string> &fields,
                            const std::vector<std::string> &values,
                            int life_in_seconds) {
  last_error_.clear();

  if (ctx_ == NULL) {
    last_error_ = "redis not connected";
    return false;
  }

  if (keys.size() != values.size()) {
    last_error_ = "#keys != #values";
    return false;
  }

  if (keys.size() != fields.size()) {
    last_error_ = "#keys != #fields";
    return false;
  }

  // 启动 transaction
  if (redisAppendCommand(ctx_, "MULTI") != REDIS_OK) {
    last_error_ = ctx_->errstr;
    return false;
  }
  // 多次 transaction set
  for (int i = 0; i < (int)keys.size(); ++i) {
    if (redisAppendCommand(ctx_, "HSET %b %b %b",
                           keys[i].data(), keys[i].size(),
                           fields[i].data(), fields[i].size(),
                           values[i].data(), values[i].size()) != REDIS_OK) {
      last_error_ = ctx_->errstr;
      return false;
    }

    if (life_in_seconds > 0) {
      if (redisAppendCommand(ctx_, "EXPIRE %b %d",
                             keys[i].data(), keys[i].size(),
                             life_in_seconds) != REDIS_OK) {
        last_error_ = ctx_->errstr;
        return false;
      }
    }
  }
  // 执行 EXEC, 并取回 MULTI 的 reply
  redisReply *reply = NULL;
  reply = reinterpret_cast<redisReply *>(redisCommand(ctx_, "EXEC"));
  if (reply == NULL) {
    last_error_ = ctx_->errstr;
    return false;
  }

  // 取多个 SET 的回复
  for (int i = 0; i < (int)keys.size(); ++i) {
    redisReply *reply = NULL;
    redisGetReply(ctx_, reinterpret_cast<void **>(&reply));
    if (reply == NULL) {
      last_error_ = ctx_->errstr;
      return false;
    }
    if (reply->type != REDIS_REPLY_STATUS || strncmp("QUEUED", reply->str, reply->len) != 0) {
      last_error_ = str(boost::format("unexpected reply, type: %1%") % reply->type);
      freeReplyObject(reply);
      return false;
    }

    // 取回 EXPIRE 的回复
    if (life_in_seconds > 0) {
      redisGetReply(ctx_, reinterpret_cast<void **>(&reply));
      if (reply == NULL) {
        last_error_ = ctx_->errstr;
        return false;
      }
      if (reply->type != REDIS_REPLY_STATUS || strncmp("QUEUED", reply->str, reply->len) != 0) {
        last_error_ = str(boost::format("unexpected reply, type: %1%") % reply->type);
        freeReplyObject(reply);
        return false;
      }
    }
  }

  // 取 EXEC 的回复
  reply = NULL;
  redisGetReply(ctx_, reinterpret_cast<void **>(&reply));
  if (reply == NULL) {
    last_error_ = ctx_->errstr;
    return false;
  }
  if (reply->type != REDIS_REPLY_ARRAY) {
    last_error_ = str(boost::format("unexpected reply, type: %1%") % reply->type);
    freeReplyObject(reply);
    return false;
  }

  // TODO(suhua): 检查 array reply 里的每一个元素

  last_error_.assign(reply->str, reply->len);
  freeReplyObject(reply);
  return true;
}

bool Client::MultiHashGet(const std::vector<std::string> &keys,
                          const std::vector<std::string> &fields,
                          std::vector<std::string> *values,
                          std::vector<int> *ret_codes) {
  last_error_.clear();

  values->clear();
  ret_codes->clear();
  values->resize(keys.size());
  ret_codes->resize(keys.size(), -1);

  if (ctx_ == NULL) {
    last_error_ = "redis not connected";
    return false;
  }

  // 启动 transaction
  if (redisAppendCommand(ctx_, "MULTI") != REDIS_OK) {
    last_error_ = ctx_->errstr;
    return false;
  }
  // 发起多次 hget
  for (int i = 0; i < (int)keys.size(); ++i) {
    if (redisAppendCommand(ctx_, "HGET %b %b",
                           keys[i].data(), keys[i].size(),
                           fields[i].data(), fields[i].size()) != REDIS_OK) {
      last_error_ = ctx_->errstr;
      return false;
    }
  }
  // 执行 EXEC, 并取回 MULTI 的 reply
  redisReply *reply = NULL;
  reply = reinterpret_cast<redisReply *>(redisCommand(ctx_, "EXEC"));
  if (reply == NULL) {
    last_error_ = ctx_->errstr;
    return false;
  }

  // 取多个 HGET 的 reply
  for (int i = 0; i < (int)keys.size(); ++i) {
    redisReply *reply = NULL;
    redisGetReply(ctx_, reinterpret_cast<void **>(&reply));
    if (reply == NULL) {
      last_error_ = ctx_->errstr;
      return false;
    }
    if (reply->type != REDIS_REPLY_STATUS || strncmp("QUEUED", reply->str, reply->len) != 0) {
      last_error_ = str(boost::format("unexpected reply, type: %1%") % reply->type);
      freeReplyObject(reply);
      return false;
    }
  }

  // 取 EXEC 的 reply
  reply = NULL;
  redisGetReply(ctx_, reinterpret_cast<void **>(&reply));
  if (reply == NULL) {
    last_error_ = ctx_->errstr;
    return false;
  }
  if (reply->type != REDIS_REPLY_ARRAY || reply->elements != keys.size()) {
    last_error_ = str(boost::format("unexpected reply, type: %1%") % reply->type);
    freeReplyObject(reply);
    return false;
  }
  CHECK_EQ(reply->elements, keys.size());

  // 从 EXEC 的 reply 中, 取出 values
  int ok_num = 0;
  for (int i = 0; i < (int)reply->elements; ++i) {
    redisReply *r = reinterpret_cast<redisReply *>(reply->element[i]);
    if (r->type == REDIS_REPLY_ERROR) {
      values->at(i) = "";
      ret_codes->at(i) = -1;
      last_error_.assign(r->str, r->len);
    } else if (r->type == REDIS_REPLY_STRING) {
      values->at(i).assign(r->str, r->len);
      ret_codes->at(i) = 0;
      ++ok_num;
    } else if (r->type == REDIS_REPLY_NIL) {
      values->at(i) = "";
      ret_codes->at(i) = 1;
      last_error_ = "the key-field doesn't exist";
    } else {
      // NOT_REACHED();
    }
  }

  last_error_.assign(reply->str, reply->len);
  freeReplyObject(reply);
  return ok_num == (int)keys.size();
}

bool Client::MultiHashGetAllFields(const std::vector<std::string> &keys,
                                   std::vector<std::map<std::string, std::string> > *field_values,
                                   std::vector<int> *ret_codes) {
  last_error_.clear();

  field_values->clear();
  ret_codes->clear();
  field_values->resize(keys.size());
  ret_codes->resize(keys.size(), -1);

  if (ctx_ == NULL) {
    last_error_ = "redis not connected";
    return false;
  }

  // 启动 transaction
  if (redisAppendCommand(ctx_, "MULTI") != REDIS_OK) {
    last_error_ = ctx_->errstr;
    return false;
  }
  // 多次 transaction get
  for (int i = 0; i < (int)keys.size(); ++i) {
    if (redisAppendCommand(ctx_, "HGETALL %b", keys[i].data(), keys[i].size()) != REDIS_OK) {
      last_error_ = ctx_->errstr;
      return false;
    }
  }
  // 执行 EXEC, 并取回 MULTI 的 reply
  redisReply *reply = NULL;
  reply = reinterpret_cast<redisReply *>(redisCommand(ctx_, "EXEC"));
  if (reply == NULL) {
    last_error_ = ctx_->errstr;
    return false;
  }

  // 取多个 HGETALL 的回复
  for (int i = 0; i < (int)keys.size(); ++i) {
    redisReply *reply = NULL;
    redisGetReply(ctx_, reinterpret_cast<void **>(&reply));
    if (reply == NULL) {
      last_error_ = ctx_->errstr;
      return false;
    }
    if (reply->type != REDIS_REPLY_STATUS || strncmp("QUEUED", reply->str, reply->len) != 0) {
      last_error_ = str(boost::format("unexpected reply, type: %1%") % reply->type);
      freeReplyObject(reply);
      return false;
    }
  }

  // 取 EXEC 的回复
  reply = NULL;
  redisGetReply(ctx_, reinterpret_cast<void **>(&reply));
  if (reply == NULL) {
    last_error_ = ctx_->errstr;
    return false;
  }
  if (reply->type != REDIS_REPLY_ARRAY || reply->elements != keys.size()) {
    last_error_ = str(boost::format("unexpected reply, type: %1%") % reply->type);
    freeReplyObject(reply);
    return false;
  }
  CHECK_EQ(reply->elements, keys.size());

  int ok_num = 0;
  for (int i = 0; i < (int)reply->elements; ++i) {
    redisReply *r = reinterpret_cast<redisReply *>(reply->element[i]);
    if (r->type == REDIS_REPLY_ERROR) {
      field_values->at(i).clear();
      ret_codes->at(i) = -1;
      last_error_.assign(r->str, r->len);
    } else if (r->type == REDIS_REPLY_ARRAY) {
      field_values->at(i).clear();
      if (r->elements == 0) {
        ret_codes->at(i) = 1;
        last_error_ = "the key doesn't exist";
      } else {
        ret_codes->at(i) = 0;
        ++ok_num;

        CHECK_EQ(r->elements % 2, 0u);
        for (int j = 0; j < (int)r->elements; j+=2) {
          redisReply *rf = reinterpret_cast<redisReply *>(r->element[j]);
          redisReply *rv = reinterpret_cast<redisReply *>(r->element[j+1]);
          CHECK_EQ(rf->type, REDIS_REPLY_STRING);
          CHECK_EQ(rv->type, REDIS_REPLY_STRING);
          field_values->at(i)[std::string(rf->str, rf->len)].assign(rv->str, rv->len);
        }
      }
    } else if (r->type == REDIS_REPLY_NIL) {
      field_values->at(i).clear();
      ret_codes->at(i) = 1;
      last_error_ = "the key doesn't exist";
    } else {
      // NOT_REACHED();
    }
  }

  last_error_.assign(reply->str, reply->len);
  freeReplyObject(reply);
  return ok_num == (int)keys.size();
}

bool Client::ListPushLeft(const char *key, int key_len,
                          const char *value, int value_len,
                          int32 *list_length) {
  last_error_.clear();

  if (ctx_ == NULL) {
    last_error_ = "redis not connected";
    return false;
  }

  redisReply *reply =
      reinterpret_cast<redisReply *>(redisCommand(ctx_, "LPUSH %b %b", key, (int64)key_len,
                                                  value, (int64)value_len));
  if (reply == NULL) {
    last_error_ = ctx_->errstr;
    return false;
  }

  if (reply->type == REDIS_REPLY_ERROR) {
    last_error_.assign(reply->str, reply->len);
    freeReplyObject(reply);
    return false;
  } else if (reply->type == REDIS_REPLY_INTEGER) {
    if (list_length != NULL) {
      *list_length = reply->integer;
    }
    freeReplyObject(reply);
    return true;
  }

  // NOT_REACHED() << reply->type;
  freeReplyObject(reply);
  return false;
}

int Client::ListPopLeft(const char *key, int key_len, std::string *value) {
  last_error_.clear();

  value->clear();
  if (ctx_ == NULL) {
    last_error_ = "redis not connected";
    return -1;
  }

  redisReply *reply =
      reinterpret_cast<redisReply *>(redisCommand(ctx_, "LPOP %b", key, (int64)key_len));
  if (reply == NULL) {
    last_error_ = ctx_->errstr;
    return -1;
  }

  if (reply->type == REDIS_REPLY_ERROR) {
    last_error_.assign(reply->str, reply->len);
    freeReplyObject(reply);
    return -1;
  } else if (reply->type == REDIS_REPLY_STRING) {
    value->assign(reply->str, reply->len);
    freeReplyObject(reply);
    return 0;
  } else if (reply->type == REDIS_REPLY_NIL) {
    last_error_ = "the key doesn't exist";
    freeReplyObject(reply);
    return 1;
  }

  // NOT_REACHED() << reply->type;
  freeReplyObject(reply);
  return -1;
}

int Client::ListPopRight(const char *key, int key_len, std::string *value) {
  last_error_.clear();

  value->clear();
  if (ctx_ == NULL) {
    last_error_ = "redis not connected";
    return -1;
  }

  redisReply *reply =
      reinterpret_cast<redisReply *>(redisCommand(ctx_, "RPOP %b", key, (int64)key_len));
  if (reply == NULL) {
    last_error_ = ctx_->errstr;
    return -1;
  }

  if (reply->type == REDIS_REPLY_ERROR) {
    last_error_.assign(reply->str, reply->len);
    freeReplyObject(reply);
    return -1;
  } else if (reply->type == REDIS_REPLY_STRING) {
    value->assign(reply->str, reply->len);
    freeReplyObject(reply);
    return 0;
  } else if (reply->type == REDIS_REPLY_NIL) {
    last_error_ = "the key doesn't exist";
    freeReplyObject(reply);
    return 1;
  }

  // NOT_REACHED() << reply->type;
  freeReplyObject(reply);
  return -1;
}

bool Client::ListPushRight(const char *key, int key_len,
                           const char *value, int value_len,
                           int32 *list_length) {
  last_error_.clear();

  if (ctx_ == NULL) {
    last_error_ = "redis not connected";
    return false;
  }

  redisReply *reply =
      reinterpret_cast<redisReply *>(redisCommand(ctx_, "RPUSH %b %b", key, (int64)key_len,
                                                  value, (int64)value_len));
  if (reply == NULL) {
    last_error_ = ctx_->errstr;
    return false;
  }

  if (reply->type == REDIS_REPLY_ERROR) {
    last_error_.assign(reply->str, reply->len);
    freeReplyObject(reply);
    return false;
  } else if (reply->type == REDIS_REPLY_INTEGER) {
    if (list_length != NULL) {
      *list_length = reply->integer;
    }
    freeReplyObject(reply);
    return true;
  }

  // NOT_REACHED() << reply->type;
  freeReplyObject(reply);
  return false;
}

bool Client::MultiListPopLeft(const std::vector<std::string> &keys,
                              std::vector<std::string> *values,
                              std::vector<int> *ret_codes) {
  last_error_.clear();

  values->clear();
  ret_codes->clear();
  values->resize(keys.size());
  ret_codes->resize(keys.size(), -1);

  if (ctx_ == NULL) {
    last_error_ = "redis not connected";
    return false;
  }

  // 启动 transaction
  if (redisAppendCommand(ctx_, "MULTI") != REDIS_OK) {
    last_error_ = ctx_->errstr;
    return false;
  }
  // 多次 transaction get
  for (int i = 0; i < (int)keys.size(); ++i) {
    if (redisAppendCommand(ctx_, "LPOP %b", keys[i].data(), keys[i].size()) != REDIS_OK) {
      last_error_ = ctx_->errstr;
      return false;
    }
  }
  // 执行 EXEC, 并取回 MULTI 的 reply
  redisReply *reply = NULL;
  reply = reinterpret_cast<redisReply *>(redisCommand(ctx_, "EXEC"));
  if (reply == NULL) {
    last_error_ = ctx_->errstr;
    return false;
  }

  // 取多个 LPOP 的回复
  for (int i = 0; i < (int)keys.size(); ++i) {
    redisReply *reply = NULL;
    redisGetReply(ctx_, reinterpret_cast<void **>(&reply));
    if (reply == NULL) {
      last_error_ = ctx_->errstr;
      return false;
    }
    if (reply->type != REDIS_REPLY_STATUS || strncmp("QUEUED", reply->str, reply->len) != 0) {
      last_error_ = str(boost::format("unexpected reply, type: %1%") % reply->type);
      freeReplyObject(reply);
      return false;
    }
  }

  // 取 EXEC 的回复
  reply = NULL;
  redisGetReply(ctx_, reinterpret_cast<void **>(&reply));
  if (reply == NULL) {
    last_error_ = ctx_->errstr;
    return false;
  }
  if (reply->type != REDIS_REPLY_ARRAY || reply->elements != keys.size()) {
    last_error_ = str(boost::format("unexpected reply, type: %1%") % reply->type);
    freeReplyObject(reply);
    return false;
  }
  CHECK_EQ(reply->elements, keys.size());

  int ok_num = 0;
  for (int i = 0; i < (int)reply->elements; ++i) {
    redisReply *r = reinterpret_cast<redisReply *>(reply->element[i]);
    if (r->type == REDIS_REPLY_ERROR) {
      values->at(i) = "";
      ret_codes->at(i) = -1;
      last_error_.assign(r->str, r->len);
    } else if (r->type == REDIS_REPLY_STRING) {
      values->at(i).assign(r->str, r->len);
      ret_codes->at(i) = 0;
      ++ok_num;
    } else if (r->type == REDIS_REPLY_NIL) {
      values->at(i) = "";
      ret_codes->at(i) = 1;
      last_error_ = "the key doesn't exist";
    } else {
      // NOT_REACHED();
    }
  }

  last_error_.assign(reply->str, reply->len);
  freeReplyObject(reply);
  return ok_num == (int)keys.size();
}

bool Client::MultiListPopRight(const std::vector<std::string> &keys,
                               std::vector<std::string> *values,
                               std::vector<int> *ret_codes) {
  last_error_.clear();

  values->clear();
  ret_codes->clear();
  values->resize(keys.size());
  ret_codes->resize(keys.size(), -1);

  if (ctx_ == NULL) {
    last_error_ = "redis not connected";
    return false;
  }

  // 启动 transaction
  if (redisAppendCommand(ctx_, "MULTI") != REDIS_OK) {
    last_error_ = ctx_->errstr;
    return false;
  }
  // 多次 transaction get
  for (int i = 0; i < (int)keys.size(); ++i) {
    if (redisAppendCommand(ctx_, "RPOP %b", keys[i].data(), keys[i].size()) != REDIS_OK) {
      last_error_ = ctx_->errstr;
      return false;
    }
  }
  // 执行 EXEC, 并取回 MULTI 的 reply
  redisReply *reply = NULL;
  reply = reinterpret_cast<redisReply *>(redisCommand(ctx_, "EXEC"));
  if (reply == NULL) {
    last_error_ = ctx_->errstr;
    return false;
  }

  // 取多个 LPOP 的回复
  for (int i = 0; i < (int)keys.size(); ++i) {
    redisReply *reply = NULL;
    redisGetReply(ctx_, reinterpret_cast<void **>(&reply));
    if (reply == NULL) {
      last_error_ = ctx_->errstr;
      return false;
    }
    if (reply->type != REDIS_REPLY_STATUS || strncmp("QUEUED", reply->str, reply->len) != 0) {
      last_error_ = str(boost::format("unexpected reply, type: %1%") % reply->type);
      freeReplyObject(reply);
      return false;
    }
  }

  // 取 EXEC 的回复
  reply = NULL;
  redisGetReply(ctx_, reinterpret_cast<void **>(&reply));
  if (reply == NULL) {
    last_error_ = ctx_->errstr;
    return false;
  }
  if (reply->type != REDIS_REPLY_ARRAY || reply->elements != keys.size()) {
    last_error_ = str(boost::format("unexpected reply, type: %1%") % reply->type);
    freeReplyObject(reply);
    return false;
  }
  CHECK_EQ(reply->elements, keys.size());

  int ok_num = 0;
  for (int i = 0; i < (int)reply->elements; ++i) {
    redisReply *r = reinterpret_cast<redisReply *>(reply->element[i]);
    if (r->type == REDIS_REPLY_ERROR) {
      values->at(i) = "";
      ret_codes->at(i) = -1;
      last_error_.assign(r->str, r->len);
    } else if (r->type == REDIS_REPLY_STRING) {
      values->at(i).assign(r->str, r->len);
      ret_codes->at(i) = 0;
      ++ok_num;
    } else if (r->type == REDIS_REPLY_NIL) {
      values->at(i) = "";
      ret_codes->at(i) = 1;
      last_error_ = "the key doesn't exist";
    } else {
      // NOT_REACHED();
    }
  }

  last_error_.assign(reply->str, reply->len);
  freeReplyObject(reply);
  return ok_num == (int)keys.size();
}

bool Client::IsConnected() {
  // 首先获得描述符
  int fd = ctx_->fd;
  if (fd < 0) {
    return false;
  }
  char buf[1];
  ssize_t ret = HANDLE_EINTR(recv(fd, buf, sizeof(buf), MSG_DONTWAIT));
  if (ret > 0) {
    LOG(WARNING) << "there are dirty data, error occur.";
    return false;
  } else if (ret == 0) {
    LOG(WARNING) << "connection closed by peer.";
    return false;
  } else {
    if (errno == EWOULDBLOCK || errno == EAGAIN) {
      // connection ok
      return true;
    } else {
      LOG(WARNING) << "read error in fd " << fd;
      return false;
    }
  }
  return false;
}

}  // namespace redis

