#!/usr/local/bin/python
import json
import urllib
import hashlib
import sys

def getWeiboInterest(uid):
	url = "http://api.ds.weibo.com/1/user_feature/get_common_by_names.json?uid=%s&feature_names=main_interest" % uid
	print "interest url:",url
	try:
		stream = urllib.urlopen(url)
		lines = stream.readlines()
		jsons = ""
		for line in lines:
			jsons = jsons + line
		obj = json.loads(jsons)
		print obj
		return obj
	except Exception as e:
		print "error when get weibo interest:", e
	obj = {}
	obj['message']="fail"
	return obj

# input: device id
# output: user id list
def getWeiboUserID(did):
	ret = []
	entry = "adrecommend"
	pin="304d5fd7b828e7143576e667a45eaa6a"
	url = "http://ilogin.sina.com.cn/api/deviceid/info"
	succ_code = 20000000
	m = hashlib.md5("%s%s"%(did,pin)).hexdigest()
	para = {}
	para["id"] = did
	para["type"] = "did"
	para["entry"] = entry
	para["m"] = m
	para["extra"] = "1"
	
	encoded_str = urllib.urlencode(para)
	requrl = "%s?%s"%(url,encoded_str)
	print "request url:", requrl
	s = urllib.urlopen(requrl)
	lines = s.readlines()
	jsons = ""
	for line in lines:
		jsons = jsons + line
	print "ret json:",jsons
	obj = json.loads(jsons)
	print obj
	if obj["retcode"] == succ_code and len(obj['data']) != 0:
		print "succ"
		keys =  obj['data'][did].keys()
		for key in keys:
			ret.append(key)
	return ret
