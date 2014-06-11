#ifndef _REDIS_PLUSPLUS_H_
#define _REDIS_PLUSPLUS_H_

#include <string>
#include <vector>
#include <map>
#include "common.h"

// needed to use redis C API
struct redisContext;

namespace redis {

class Client {
 public:
  // 构造时不建立连接, 需要调用成员方法 Connect() 来建立连接.
  Client(const std::string &ip, int port);
  // 析构时, 会释放连接
  ~Client();

  // 获取最近一次错误
  const std::string &last_error() const {
    return last_error_;
  }

  // Connect(), Disconnect(), Reconnect(), Ping()
  //
  // return true if success
  // return false if failed, and call last_error() to get the error reason;
  //
  bool Connect() WARN_UNUSED_RESULT;
  bool Disconnect();
  bool Reconnect() WARN_UNUSED_RESULT;
  bool IsConnected() WARN_UNUSED_RESULT;  // 检查连接是否异常断开
  bool Ping() WARN_UNUSED_RESULT;

  // redis 读写超时， 单位毫秒
  // 必须在调用 Connect() 以后调用该函数， 否则返回 false
  bool SetReadWriteTimeOut(int timeout_in_ms);  // redis 读写超时， 单位毫秒

  //==========================================================================
  // 下面的命令用于读写 Key-Value, 对应 redis 的 string data structure; 包括
  // Get(), Set(), MultiGet(), MultiSet(), 及其 Ex 版本
  //==========================================================================

  // 设置 key-value
  //
  // 如果 life_in_seconds > 0, 则设置该 key 的过期自动删除时间
  bool SetEx(const char *key, int key_len,
             const char *value, int value_len,
             int life_in_seconds) WARN_UNUSED_RESULT;
  bool Set(const char *key, int key_len, const char *value, int value_len)  WARN_UNUSED_RESULT {
    return SetEx(key, key_len, value, value_len, -1);
  }

  // return 0 if success
  // return 1 if the |key| dosen't exist
  // return -1 for other errors, and call last_error() to get the error reason;
  int Get(const char *key, int key_len, std::string *value) WARN_UNUSED_RESULT;

  // return 0 if success
  // return 1 if the |key| dosen't exist
  // return -1 for other errors, and call last_error() to get the error reason;
  int Delete(const char *key, int key_len) WARN_UNUSED_RESULT;

  // 如果 life_in_seconds > 0, 则设置该 key 的过期自动删除时间
  bool MultiSetEx(const std::vector<std::string> &keys,
                  const std::vector<std::string> &values,
                  int life_in_seconds) WARN_UNUSED_RESULT;
  bool MultiSetEx(const std::vector<std::string> &keys,
                  const std::vector<std::string> &values,
                  const std::vector<int> &life_in_seconds) WARN_UNUSED_RESULT;
  bool MultiSet(const std::vector<std::string> &keys,
                const std::vector<std::string> &values) WARN_UNUSED_RESULT {
    return MultiSetEx(keys, values, -1);
  }

  // 相当于调用多次 Get(), 但是性能更好.
  //
  // 返回值:
  // 如果所有 Get() 都返回 0, 则返回 true
  // 只要有一个 Get() 不返回 0, 则返回 false,
  // 且 results 中记录了每个 Get() 的返回值, 具体含义见 Get() 的注释.
  //
  // NOTE: 函数返回后, values->size(), ret_codes->size() 跟 keys.size() 相等
  bool MultiGet(const std::vector<std::string> &keys,
                std::vector<std::string> *values,
                std::vector<int> *ret_codes) WARN_UNUSED_RESULT;

  //==========================================================================
  // 下面的命令用于读写 key-field-value, 对应 redis 的 hash data structure;
  // 包括 HashGet(), HashGetAllFields(), HashSet(), MultiHashGet(),
  // MultiHashSet(), MultiHashGetAll() 及其 Ex 版本 同一个 key 里还可以有多个
  // field, 每个 field 一个 value
  //==========================================================================

  // 设置 key-field-value
  //
  // 如果 life_in_seconds > 0, 则设置该 key 的过期自动删除时间
  //
  // NOTE: 超时是作用在 key 上的, 所以虽然只修改了某个 key-field 对应的 value,
  // 但是如果制定了大于 0 的 life_in_seconds, 会修改整个 key 的超时时间.
  bool HashSetEx(const char *key, int key_len,
                 const char *field, int field_len,
                 const char *value, int value_len,
                 int life_in_seconds) WARN_UNUSED_RESULT;
  bool HashSet(const char *key, int key_len,
               const char *field, int field_len,
               const char *value, int value_len) WARN_UNUSED_RESULT {
    return HashSetEx(key, key_len, field, field_len, value, value_len, -1);
  }

  // 获取 key-field 的 value
  //
  // return 0 if success
  // return 1 if the |key| or |field| dosen't exist
  // return -1 for other errors, and call last_error() to get the error reason;
  int HashGet(const char *key, int key_len,
              const char *field, int field_len,
              std::string *value) WARN_UNUSED_RESULT;

  // 获取一个 key 下的所有 field-value;
  // 将结果填充到 |field_values| 中 (类型为 std::map, 是 field -> value 的映射).
  //
  // return 0 if success
  // return 1 if the |key| dosen't exist
  // return -1 for other errors, and call last_error() to get the error reason;
  int HashGetAllFields(const char *key, int key_len,
                       std::map<std::string, std::string> *field_values) WARN_UNUSED_RESULT;

  // 获取多个 key 的 所有 field-value
  //
  // 设置多个 key-field 的 value
  // 如果 life_in_seconds > 0, 则设置该 key 的过期自动删除时间
  bool MultiHashSetEx(const std::vector<std::string> &keys,
                      const std::vector<std::string> &fields,
                      const std::vector<std::string> &values,
                      int life_in_seconds) WARN_UNUSED_RESULT;
  bool MultiHashSet(const std::vector<std::string> &keys,
                    const std::vector<std::string> &fields,
                    const std::vector<std::string> &values) WARN_UNUSED_RESULT {
    return MultiHashSetEx(keys, fields, values, -1);
  }

  // 获取多个 key-field 下的 value
  // 相当于调用多次 HashGet(), 但是性能更好.
  //
  // 返回值:
  // 如果所有 HashGet() 都返回 0, 则返回 true
  // 只要有一个 Get() 不返回 0, 则返回 false,
  // 且 results 中记录了每个 Get() 的返回值, 具体含义见 Get() 的注释.
  //
  // NOTE: 函数返回后, values->size(), ret_codes->size() 跟 keys.size() 相等
  bool MultiHashGet(const std::vector<std::string> &keys,
                    const std::vector<std::string> &fields,
                    std::vector<std::string> *values,
                    std::vector<int> *ret_codes) WARN_UNUSED_RESULT;

  // 获取多个 key 下的所有 field-value
  // 相当于调用多次 HashGetAllFields(), 但是性能更好.
  //
  // 返回值:
  // 如果所有 HashGetAllFields() 都返回 0, 则返回 true
  // 只要有一个 HashGetAllFields() 不返回 0, 则返回 false,
  // 且 ret_codes 中记录了每个 HashGetAllFields() 的返回值, 具体含义见 HashGetAllFields() 的注释.
  //
  // NOTE: 函数返回后, field_values, ret_codes 的内容, 跟 keys 的内容一一对应
  bool MultiHashGetAllFields(const std::vector<std::string> &keys,
                             std::vector<std::map<std::string, std::string> > *field_values,
                             std::vector<int> *ret_codes) WARN_UNUSED_RESULT;

  //==========================================================================
  // ListPushLeft(), ListPopLeft(), ListPushRight(),
  // ListPopRight(), MultiListPopLeft(), MultiListPopRight()
  //==========================================================================
  //
  // PushLeft 操作, 返回操作成功或失败
  // 如果成功 且 list_length 不为 NULL， 那么 *list_length 为 Push 操作后链表的长度
  // 如果失败 不修改 *list_length 的值
  bool ListPushLeft(const char *key, int key_len,
                    const char *value, int value_len, int32 *list_length) WARN_UNUSED_RESULT;

  // return 0 if success
  // return 1 if the |key| dosen't exist
  // return -1 for other errors, and call last_error() to get the error reason;
  int ListPopLeft(const char *key, int key_len, std::string *value) WARN_UNUSED_RESULT;

  // PushRight 操作, 返回操作成功或失败
  // 如果成功 且 list_length 不为 NULL， 那么 *list_length 为 Push 操作后链表的长度
  // 如果失败 不修改 *list_length 的值
  bool ListPushRight(const char *key, int key_len,
                     const char *value, int value_len, int32 *list_length) WARN_UNUSED_RESULT;

  // return 0 if success
  // return 1 if the |key| dosen't exist
  // return -1 for other errors, and call last_error() to get the error reason;
  int ListPopRight(const char *key, int key_len, std::string *value) WARN_UNUSED_RESULT;

  // 返回值:
  // 如果所有 LPOP() 都返回 0, 则返回 true
  // 只要有一个 LPOP() 不返回 0, 则返回 false,
  // 且 results 中记录了每个 LPOP() 的返回值, 具体含义见 LPOP() 的注释.
  //
  // NOTE: 函数返回后, values->size(), ret_codes->size() 跟 keys.size() 相等
  bool MultiListPopLeft(const std::vector<std::string> &keys,
                        std::vector<std::string> *values,
                        std::vector<int> *ret_codes) WARN_UNUSED_RESULT;

  // 返回值:
  // 如果所有 RPOP() 都返回 0, 则返回 true
  // 只要有一个 RPOP() 不返回 0, 则返回 false,
  // 且 results 中记录了每个 RPOP() 的返回值, 具体含义见 RPOP() 的注释.
  //
  // NOTE: 函数返回后, values->size(), ret_codes->size() 跟 keys.size() 相等
  bool MultiListPopRight(const std::vector<std::string> &keys,
                         std::vector<std::string> *values,
                         std::vector<int> *ret_codes) WARN_UNUSED_RESULT;
 private:
  std::string ip_;
  int port_;

  redisContext *ctx_;
  std::string last_error_;
  DISALLOW_COPY_AND_ASSIGN(Client);
};

}  // namespace redis

#endif // _REDIS_PLUSPLUS_H_

