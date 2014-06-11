#ifndef _REDIS_CLUSTER_H_
#define _REDIS_CLUSTER_H_

#include <map>
#include <string>
#include <fstream>
#include <pthread.h>
#include <boost/unordered_map.hpp>
#include <boost/program_options.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/tokenizer.hpp>
#include "city.h"
#include "redispp.h"


namespace redis_cluster {

struct CityHash {
  uint32 operator()(const char *str) const {
    CityHash32(str, strlen(str));
    return CityHash32(str, strlen(str));
  }
};

template <class Hash>
class HashRing {
  public:
    typedef struct Node_t {
      std::string node_name;
      unsigned int replicas;
    } Node;

    typedef std::map<uint32, Node> NodeMap;

    HashRing(): hash_(CityHash()) {}

    HashRing(const Hash& hash): hash_(hash) {}

    uint32 AddNode(const std::string& node_name, const unsigned int replicas);
    void RemoveNode(const std::string& node_name);
    const std::string& GetNode(const std::string& key) const;

  private:
    NodeMap ring_;
    Hash hash_;

    std::string ToString(unsigned int r);

    DISALLOW_COPY_AND_ASSIGN(HashRing);
};


class RedisCluster{
  public:
    typedef struct {
      redis::Client *master;
      redis::Client *slave;
      pthread_mutex_t mutex;
    } RedisPair;
    typedef std::map<std::string, RedisPair> ServerMap;

    RedisCluster();
    virtual ~RedisCluster();

    bool Initialize(const std::string& redis_config_file);

    bool GetRedisNode(const std::string& key,
        redis::Client **client,
        pthread_mutex_t **mutex);

    // just for test
    const std::string GetRedisNode(const std::string& key);

  private:
    std::string config_file_;
    int is_open_slave_;
    HashRing<CityHash> hash_ring_;
    ServerMap servers_;

    bool LoadRedisesMaster();
    bool LoadRedisesSlave();

    DISALLOW_COPY_AND_ASSIGN(RedisCluster);
};

}


#endif // _REDIS_CLUSTER_H_
