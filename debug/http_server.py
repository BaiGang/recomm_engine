#!/usr/local/bin/python
"""
 Copyright 2013 Sina Inc. All rights reserved.
 Author: yanbing3@staff.sina.com.cn (Yan-Bing Bai)
 Description: HTTP Server for debugging recomm engine
 Simply read data from redis and mysql
"""
import api
import db_reader
import BaseHTTPServer 
import sys
import SocketServer
import redis
import json
import mysql.connector
from SimpleHTTPServer import SimpleHTTPRequestHandler
host=""
port=int(sys.argv[1])
#load word mapping
word_dict_file = open("tmp_dict", "r")
word_dict = {}
for line in word_dict_file:
	segs = line.split('\t')
	word_dict[segs[0].strip()] = segs[1].strip()
print len(word_dict)


# Http Server handler
class RecommEnegineDebugHttpServerHandler(BaseHTTPServer.BaseHTTPRequestHandler):
	def do_head(self):
		pass
	# Process post request
	def do_POST(self):
		# len=int(self.headers['Content-Length'])
		# if self.rfile:
		#   print self.rfile.read(len)
		pass
	# Parse args from request url as k-v pairs
	def get_arg(self):
		ret = {} 
		str = self.path
		para_idx = str.find('?');
		if para_idx < 0:
			return ret
		print "para_index=",para_idx
		str = str[para_idx+1:len(str)]
		segs = str.split('&')
		for seg in segs:
			print seg
			kv_segs = seg.split('=')
			if (len(kv_segs) < 2):
				print "error kv format ", seg
			else:
				ret[kv_segs[0]] = kv_segs[1]
		return ret
		 
	# Process get request
	def do_GET(self):
		ret = {}
		ret['users'] = []
		ret['devices'] = []
		args = self.get_arg()
		if (args.has_key('did')):
			did = args['did']

			# process users 
			user_id_list = api.getWeiboUserID(did)
			for uid in user_id_list:
					user = {}
					user['weibo_interests'] = api.getWeiboInterest(uid)
					ret['users'].append(user)
					pass

			# process did
			device = {}
			device['did'] = did
			device['history'] = []
			device['reason'] = {}

			doclist = db_reader.get_basic_doc_info_from_redis(did)
			device['recomm_docs'] = db_reader.get_docs_from_mysql(doclist)
			device['history'] = db_reader.get_history(did)
			device['reason'] = db_reader.get_recomm_reason_from_redis(did, word_dict)
			ret['devices'].append(device)
		self.respond(ret)
		
	def respond(self, user_info):
		self.send_response(200, message=None)
		self.send_header('Content-type', 'application/json')
		self.end_headers();
		res = json.dumps(user_info)
		res = "getUserRecommInfoOnline(%s)" % (res)
		self.wfile.write(res.encode(encoding='utf-8'))

# start http server
if __name__ == "__main__":
	handler = RecommEnegineDebugHttpServerHandler; 
	print "serving with %s %d" % (host, port)
	httpd = SocketServer.TCPServer((host, port), handler);
	httpd.serve_forever()
