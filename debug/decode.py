#!/usr/local/bin/python
import redis
import json
import mysql.connector
import urllib
import hashlib
import sys
print urllib.unquote(sys.argv[1].encode('utf-8'))
