
#二期内容推荐架构

## 1. 线上推荐引擎

对外是http的接口，本质上是一个thin layer，做query转换，然后调用我们的thrift服务。

recommendation engine是基于thrift的服务。它有两个接口，一个是文档添加接口，一个是推荐结果接口，分别是两个thrift server。所需数据有两个，一个是item profile，一个是user profile。item profile采用pushed to server的方式，由另外的service从存储系统（HBase）中拉取出数据然后推送给server；user profile采用pulled by server的方式，从存储系统（redis/mc）中拉取。

### 1.1 service接口：

   * 文档接口
      * SetDocProfile(item_id, article_profile {topics, named_entities})
         * 添加文档的profile信息到In-Process Doc DB，topics是文档的ldatopic向量，named_entities是提取的重要的entity
      * SetDocMetrics(item_id, article_visits{covisited_articles, clicks_by_uclusters} )
         * 添加文档的访问信息，主要是同时被访问的文章有哪些，被哪些用户聚类下的用户访问过，次数分别是多少
      * 如果in-process doc DB不支持并发读写，那么文档接口使用TNonblockingServer，否则使用TThreadedSelectorServer

   * 推荐结果
      * GetRecommendation(user_id, ip, etc..)  根据user_id获取用户的user profile；然后遍历in-process article db，选取综合相关性和多样性的推荐结果。
      * 具体过程是，首先Algo Selection模块确定当前这次serving需要应用那几个算法，并访问存储获取当前访问者的user profile
      * 然后依次调用selection, scoring和result插件；Selection相当于做一次粗筛，选取了与用户某一方面相关的item；然后scoring做分数计算；result插件选取top N，生成最终结果，返回多个结果时，多样性可以在这里来处理。

### 1.2 内部模块：

   * In-process Doc DB
      * 使用leveldb来实现，需要两组接口，一个是添加文档的接口，对应service中的SetDocProfile和SetDocMetrics；另一个是查询/遍历接口。使用leveldb的原因是其本身就做了persistence，而且in-process效率很高。
   * Algo Selection
预先编写好的流量分配策略，根据uid/ip或随机的方式来进行算法插件的选择
      * 获取当前用户的user profile，这部分也要有in-memory的LRU cache，以降低对UP存储的压力
   * Algo plugins
      * ItemSelection plugin
         * 根据给定的user profile和item list，应用一些策略（topic matching, named entity matching, etc）来做初步的选取，返回结果为当前user profile, 粗筛过的item list，以及一些中间分数
      * ItemScoring plugin
         * 根据user profile，selection过程中的中间分数以及items，对每一个item计算一组分数
      * Result plugin
         * 根据scoring的分数，按照一定的规则（top K, budgeted maximum coverage）来选取最终的推荐结果

### 2. 文档profile生成模块

   * 抓取文章，来自sina.cn web页面或wapcms数据库
   * 分词，提取named entities
   * 使用分词结果计算lda topic infer
   * 将这部分profile通过推荐引擎的SetDocProfile接口推送给推荐引擎

### 3. 用户profile和聚类挖掘模块

   * 从用户浏览日志中生成用户访问历史列表
   * 生成用户的topic和named entity兴趣profile
   * 使用Hadoop计算用户的MinHash聚类和PLSI聚类
   * 将结果推送到HBase服务（凌晨执行，覆盖之前的结果，保证时效性）

### 4. 实时流日志处理与结果推送

   * 数据中心通过Scribe将新浪新闻客户端请求日志推送到实时流系统中
   * 实时流系统首先解析日志，提取user/dev id信息，以及item的it等
   * 更新user和item的实时信息到HBase服务
      * user实时数据
         * 用户session访问历史
      * item实时数据
         * 被同时访问/like/评论的文章列表（co-engagement）
         * 被每一用户聚类访问/like/评论的次数
   * 将结果以increment的方式推送到HBase服务

### 5. HBase存储与线上机房本地cache
   * HBase使用数据中心的实例
   * 线上cache需要再部署redis或mc


