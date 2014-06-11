#!/usr/local/bin/python
"""
 Copyright 2013 Sina Inc. All rights reserved.
 Author: yanbing3@staff.sina.com.cn (Yan-Bing Bai)
 Description: HTTP Server for debugging recomm engine
 Simply read data from redis and mysql
"""
import BaseHTTPServer 
import sys
import urllib
import SocketServer
import redis
import json
import mysql.connector
from SimpleHTTPServer import SimpleHTTPRequestHandler

# return history
def get_history(did):
	docs = []
	url="http://10.79.96.44:9999?did=%s" % did
	mysql_host="172.16.193.108"
	mysql_port=30300
	mysql_user="wapcms_r"
	mysql_password = "VGBknKtQG7W"
	mysql_dbname = "wapcms_11"
	print "get history url ", url
	try:
		conn = mysql.connector.connect(
		       host= mysql_host, port = mysql_port, user= mysql_user, password=mysql_password, database=mysql_dbname);
		stream = urllib.urlopen(url)
		lines = stream.readlines()
		jsons = ""
		for line in lines:
			jsons = jsons + line
		doclist = json.loads(jsons)
		for str in doclist:
			segs = str.split('-')
			if len(segs) < 2:
				print "bad format doc ", str
				continue
			sql = "SELECT title, wap_title, web_url, channel  FROM %s where id = %s;" % ("dt_"+segs[0], segs[1])
			doc = {}
			try:
				cur = conn.cursor()
				cur.execute(sql)
				res_db = cur.fetchone();
				if(res_db == None):
					continue
				if len(res_db) < 4:
					continue
				doc['title'] = res_db[0]
				doc['wap_title'] = res_db[1]
				doc['web_url'] = res_db[2]
				doc['channel'] = res_db[3]
				docs.append(doc)
				cur.close()
			except Exception:
				pass
		conn.close()
	except Exception as e:
		print "exception when getting url", url, e
	return docs
# return a reason obj
def get_recomm_reason_from_redis(did, word_dict):
	reason = {}
	reason['keywords'] = {}
	host = "10.79.96.42"
	port = 10086
	db_idx = 0
	key = "muep_%s"%did
	try:
		redis_conn = redis.StrictRedis(host=host, port=port, db=db_idx)
		value_str = redis_conn.get(key)
		tmp = json.loads(value_str)
		obj = {}
		for (k,v) in tmp.items():
				if word_dict.has_key(k):
						obj[word_dict[k]] = v
				else:
						obj[k] = v
		reason['keywords'] = obj
	except Exception as e:
		print "error when reading redis,",e
	return reason
# get from redis
def get_basic_doc_info_from_redis(did):
	MYSQL_TABLE_PREFIX = "dt_"
	REDIS_KEY_PREFIX = "p_"
	redis_host='10.79.96.52'
	redis_port=7000
	redis_db_idx = 0
	ret = []
	recomm_doc_list = []
	redis_key = REDIS_KEY_PREFIX + did 
	try:
		redis_conn = redis.StrictRedis(host=redis_host, port=redis_port, db=redis_db_idx)
		redis_value_str = redis_conn.get(redis_key)
		if (redis_value_str != None):
			recomm_doc_list = json.loads(redis_value_str)
			for info in recomm_doc_list:
				doc = {}
				doc['table'] = MYSQL_TABLE_PREFIX + info[0]
				doc['docid'] = info[1]
				doc['score'] = info[2]
				doc['timestamp'] = info[3]
				ret.append(doc)
	except Exception as e:
		print "error occured when reading redis ", e
	return ret
# return a list of docs
def get_docs_from_mysql(doc_list):
	mysql_host="172.16.193.108"
	mysql_port=30300
	mysql_user="wapcms_r"
	mysql_password = "VGBknKtQG7W"
	mysql_dbname = "wapcms_11"
	ret = []
	try:
		conn = mysql.connector.connect(
		       host= mysql_host, port = mysql_port, user= mysql_user, password=mysql_password, database=mysql_dbname);
		for doc in doc_list:
			sql = "SELECT title, wap_title, web_url, content, channel  FROM %s where id = %s;" % (doc['table'], doc['docid'])
			cur = conn.cursor()
			cur.execute(sql)
			res_db = cur.fetchone();
			if (res_db == None):
				print "no result from db with %s %s" % (docs[0], doc[1])
				cur.close();
				continue
			if len(res_db) < 5:
				print "no ok result from db ", res_db
				cur.close();
				continue
			doc['title'] = res_db[0]
			doc['wap_title'] = res_db[1]
			doc['web_url'] = res_db[2]
			# doc['content'] = res_db[3]
			doc['content'] = ""
			doc['channel'] = res_db[4]
			ret.append(doc)
			cur.close()
		conn.close()
	except Exception as e:
		print "error when operating mysql ", e
	return ret
