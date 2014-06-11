#include "redis_cluster.h"
#include <iostream>

namespace redis_cluster{

template <class Hash>
uint32 HashRing<Hash>::AddNode(const std::string& node_name, const unsigned int replicas) {
	uint32 hash;
	for (unsigned int r = 0; r < replicas; r++) {
		hash = hash_((node_name + ToString(r)).c_str());
		ring_[hash].node_name = node_name;
    ring_[hash].replicas = replicas;
	}
	return hash;
}

template <class Hash>
void HashRing<Hash>::RemoveNode(const std::string& node_name) {
  uint32 hash = hash_((node_name + ToString(0)).c_str());
  unsigned int replicas = ring_[hash].replicas;
	for (unsigned int r = 0; r < replicas; r++) {
		uint32 hash = hash_((node_name + ToString(r)).c_str());
		ring_.erase(hash);
	}
}

template <class Hash>
const std::string& HashRing<Hash>::GetNode(const std::string& key) const {
	uint32 hash = hash_(key.c_str());
	typename NodeMap::const_iterator it;
	// Look for the first node >= hash
	it = ring_.lower_bound(hash);
	if (it == ring_.end()) {
		// Wrapped around; get the first node
		it = ring_.begin();
	}
	return it->second.node_name;
}

template <class Hash>
std::string HashRing<Hash>::ToString(unsigned int r) {
  char tmp[10];
  snprintf(tmp, sizeof(tmp), "%u", r);
  std::string str(tmp);
  return str;
}

RedisCluster::RedisCluster() {

}

RedisCluster::~RedisCluster() {
  ServerMap::const_iterator it;
  for (it = servers_.begin(); it != servers_.end(); ++it) {
    if (NULL != it->second.master) {
      it->second.master->Disconnect();
    }
    if (NULL !=it->second.slave) {
      it->second.slave->Disconnect();
    }
  }
}

bool RedisCluster::Initialize(const std::string& config_file) {
  config_file_ = config_file;
  if (!LoadRedisesMaster()) {
    LOG(ERROR) << "Initialization redises master failed \
      due to failure to load config file " << config_file;
    return false;
  }

  if (is_open_slave_) {
    if (!LoadRedisesSlave()) {
      LOG(ERROR) << "Initialization redises slave failed \
        due to failure to load config file " << config_file;
      return false;
    }
  }

  return true;
}

bool RedisCluster::LoadRedisesMaster() {
  std::ifstream config_file(config_file_.c_str());
  if (!config_file.good()) {
    LOG(ERROR) << "Failed open config file [" << config_file_ << "]";
    return false;
  }

  boost::program_options::options_description desc("redis configs");
  std::string redises_master;
  std::string redises_slave;
  desc.add_options()
    ("is_open_slave", boost::program_options::value<int>(&is_open_slave_),
        "Is open redis master-slave mode.")
    ("redises_master", boost::program_options::value<std::string>(&redises_master)->multitoken(),
        "A series of redis master specifications.")
    ("redises_slave", boost::program_options::value<std::string>(&redises_slave)->multitoken(),
        "A series of redis slave specifications.")
  ;
  boost::program_options::variables_map vm;
  boost::program_options::store(boost::program_options::parse_config_file(config_file, desc), vm);
  config_file.close();
  boost::program_options::notify(vm);
  boost::char_separator<char> space_sep(" \t");
  boost::char_separator<char> colon_sep(":");

  LOG(INFO) << "Redises master specification is: {" << redises_master << "}";
  
  boost::tokenizer<boost::char_separator<char> > redis_master_items(redises_master, space_sep);
  for (boost::tokenizer<boost::char_separator<char> >::iterator it = redis_master_items.begin();
       it != redis_master_items.end(); ++it) {
    boost::tokenizer<boost::char_separator<char> > fields(*it, colon_sep);
    boost::tokenizer<boost::char_separator<char> >::iterator field = fields.begin();

    // 1. redis master node name
    std::string node_name = (*field);

    // 2. redis master host
    std::string host = *(++field);

    // 3. redis master port
    int port = 0;
    try {
      field ++;
      port = boost::lexical_cast<int>(*field);
    } catch (boost::bad_lexical_cast &) {
      LOG(ERROR) << "Failed reading redis master port on \"" << *it << "\""; 
      return false;
    }

    // 4. redis master node replicas
    unsigned int replicas = 0;
    try {
      field ++;
      replicas = boost::lexical_cast<unsigned int>(*field);
    } catch (boost::bad_lexical_cast &) {
      LOG(ERROR) << "Failed reading redis master node replicas on \"" << *it << "\""; 
      return false;
    }

    // add redis node to hash ring
    hash_ring_.AddNode(node_name, replicas);

    LOG(INFO) << "Loading config for redis master [" << host << "].";

    servers_[node_name].master = new redis::Client(host, port);
    if (!servers_[node_name].master->Connect()) {
      LOG(ERROR) << "Failed connecting redis master [" << host << "].";
      return false;
    }
    servers_[node_name].slave = NULL;
    pthread_mutex_init(&servers_[node_name].mutex, NULL);

    LOG(INFO) << "Done loading for redis master " << "host = " << host \
      << " port = " << port << " replicas = " << replicas;
  }

  return true;
}

bool RedisCluster::LoadRedisesSlave() {
  std::ifstream config_file(config_file_.c_str());
  if (!config_file.good()) {
    LOG(ERROR) << "Failed open config file [" << config_file_ << "]";
    return false;
  }

  boost::program_options::options_description desc("redis configs");
  std::string redises_master;
  std::string redises_slave;
  desc.add_options()
    ("is_open_slave", boost::program_options::value<int>(&is_open_slave_),
        "Is open redis master-slave mode.")
    ("redises_master", boost::program_options::value<std::string>(&redises_master)->multitoken(),
        "A series of redis master specifications.")
    ("redises_slave", boost::program_options::value<std::string>(&redises_slave)->multitoken(),
        "A series of redis slave specifications.")
  ;
  boost::program_options::variables_map vm;
  boost::program_options::store(boost::program_options::parse_config_file(config_file, desc), vm);
  config_file.close();
  boost::program_options::notify(vm);
  boost::char_separator<char> space_sep(" \t");
  boost::char_separator<char> colon_sep(":");
  
  LOG(INFO) << "Redises slave specification is: {" << redises_slave << "}";

  boost::tokenizer<boost::char_separator<char> > redis_slave_items(redises_slave, space_sep);
  for (boost::tokenizer<boost::char_separator<char> >::iterator it = redis_slave_items.begin();
      it != redis_slave_items.end(); ++it) {
    boost::tokenizer<boost::char_separator<char> > fields(*it, colon_sep);
    boost::tokenizer<boost::char_separator<char> >::iterator field = fields.begin();

    // 1. redis slave node name
    std::string node_name = (*field);

    // 2. redis slave host
    std::string host = *(++field);

    // 3. redis slave port
    int port = 0;
    try {
      field ++;
      port = boost::lexical_cast<int>(*field);
    } catch (boost::bad_lexical_cast &) {
      LOG(ERROR) << "Failed reading redis slave port on \"" << *it << "\""; 
      return false;
    }

    servers_[node_name].slave = new redis::Client(host, port);
    if (!servers_[node_name].slave->Connect()) {
      LOG(ERROR) << "Failed connecting redis slave [" << host << "].";
      return false;
    }

    LOG(INFO) << "Done loading for redis slave " << "host = " << host \
      << " port = " << port;
  }

  return true;
}


bool RedisCluster::GetRedisNode(const std::string& key,
    redis::Client **client,
    pthread_mutex_t **mutex) {

  std::string node_name;
  node_name = hash_ring_.GetNode(key);
  *mutex = &servers_[node_name].mutex;
  *client = servers_[node_name].master; 

  pthread_mutex_lock(&servers_[node_name].mutex);
  if ((*client)->Ping()) {
    pthread_mutex_unlock(&servers_[node_name].mutex);
    return true;
  }

  if ((*client)->Reconnect()) {
    pthread_mutex_unlock(&servers_[node_name].mutex);
    return true;
  }

  if (is_open_slave_) {
    *client = servers_[node_name].slave; 
    if ((*client)->Ping()) {
      pthread_mutex_unlock(&servers_[node_name].mutex);
      return true;
    }
  }

  if ((*client)->Reconnect()) {
    pthread_mutex_unlock(&servers_[node_name].mutex);
    return true;
  }

  pthread_mutex_unlock(&servers_[node_name].mutex);

  return false;
}

const std::string RedisCluster::GetRedisNode(const std::string& key) {
  return hash_ring_.GetNode(key);
}

}
