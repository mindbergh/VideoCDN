import sys, select, time
from subprocess import Popen, PIPE, STDOUT
import requests

def run_bg(args):
    Popen(args, shell=True)

def get(url):
        return get_curl(url)

def get_requests(url):
    return requests.get(url).content

def get_curl(url):
    return check_both("curl -f -s %s" % url, shouldPrint=False)[0][0]


def check_both(args, shouldPrint=True, check=True):
    out = ""
    p = Popen(args,shell=True,stdout=PIPE,stderr=STDOUT)
    poll_obj = select.poll()
    poll_obj.register(p.stdout, select.POLLIN)
    t = time.time()
    while (time.time() - t) < 3:
        poll_result = poll_obj.poll(0)
        if poll_result:
            line = p.stdout.readline()
            if not line:
                break
            if shouldPrint: sys.stdout.write(line)
            out += line
            t = time.time()
    rc = p.wait()
    out = (out,"")
    out = (out, rc)
    if check and rc is not 0:
        #print "Error processes output: %s" % (out,)
        raise Exception("subprocess.CalledProcessError: Command '%s'" \
                            "returned non-zero exit status %s" % (args, rc))
    return out


def do():
    ip = "1.0.0.1"
    port = "9999"
    num_gets = 5
    content = get('http://%s:%s/vod/big_buck_bunny.f4m' % (ip, port))
    
    for i in xrange(num_gets):
        content = get('http://%s:%s/vod/1000Seg2-Frag7' %(ip, port))


    

if __name__ == "__main__":
    do()