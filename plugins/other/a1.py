#!/usr/bin/python
import cgi
import os
import sys
from urlparse import parse_qs
import traceback

try:

    for k, v in sorted(os.environ.items()):
        str = "{}={}".format(k, v) + "\n"
        sys.stderr.write(str)

    resp = "this is a1.py\n"

    len = int(os.environ["CONTENT_LENGTH"])

    query = os.environ["QUERY_STRING"]

    #print "CONTENT_LENGTH = %d" % len
    #print "QUERY_STRING = " + query
    #print "HOME_DIR = " + os.environ["HOME_DIR"]
    #print "DOCUMENT_ROOT = " + os.environ["DOCUMENT_ROOT"]
    resp += "CONTENT_LENGTH = %d" % len
    resp += "\nQUERY_STRING = " + query
    resp += "\nHOME_DIR = " + os.environ["HOME_DIR"]

    params = parse_qs(query)

    if 'param' in params:
        resp += "\nparam = " + params["param"][0]

    if len:
        data = sys.stdin.read(len)
        resp += "\ndata = " + data

    print resp

except Exception as ex:
    traceback.print_exc()
    print("Unexpected error: %s" % ex)
    sys.exit(3)
    
sys.exit(0)
