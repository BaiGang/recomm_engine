#/usr/local/bin/python
import os
import re
import datetime
import sys
import BaseHTTPServer
import SocketServer
import json
from SimpleHTTPServer import SimpleHTTPRequestHandler

DATA_DIR="/data1/wapcms/USER_PROFILE_DATA_RSYNC"

def get_history(did):
  hdoclist = []
  now = datetime.datetime.now()
  delta = datetime.timedelta(days=-1)
  now = now + delta
  try:
    dt = now.strftime("%Y%m%d")
    fname = "%s/mbuser_history.%s" % (DATA_DIR, dt)
    out = os.popen("grep %s^A %s"%(did, fname))
    str = ""
    for line in out:
      str =  str + line
    print str
    segs = str.split('^A')
    if len(segs) < 2:
      return hdoclist
    docs = segs[1].strip().split('^B')
    for doc in docs:
      hdoclist.append(doc)
    print "error when greping ", e
  except Exception:
    print "error when greping";
  return hdoclist
class RecommEnegineDebugHttpServerHandler(BaseHTTPServer.BaseHTTPRequestHandler):
  def do_head(self):
    pass

  def get_args(self):
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

  def do_GET(self):
    args = self.get_args()
    ret = []
    if not args.has_key("did"):
      self.respond(ret)
      return
    did = args['did']
    ret = get_history(did)
    self.respond(ret)
    return

  def respond(self, obj):
    self.send_response(200, message=None)
    self.send_header('Content-type', 'application/json')
    self.end_headers();
    res = json.dumps(obj)
    self.wfile.write(res.encode(encoding='utf-8'))
if __name__ == "__main__":
  host=""
  port = int(sys.argv[1])
 	handler = RecommEnegineDebugHttpServerHandler;
  print "serving with %s %d" % (host, port)
  httpd = SocketServer.TCPServer((host, port), handler);
  httpd.serve_forever()


