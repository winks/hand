import socket
import sys
import finger

HOST = None
PORT = 79
s = None

while 1:
    for res in socket.getaddrinfo(HOST, PORT, socket.AF_UNSPEC, socket.SOCK_STREAM, 0, socket.AI_PASSIVE):
        af, socktype, proto, canonname, sa = res
        try:
            s = socket.socket(af, socktype, proto)
        except socket.error, msg:
            s = None
            continue
        try:
            s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            s.bind(sa)
            s.listen(1)
        except socket.error, msg:
            s.close()
            s = None
            continue
        break;
    if s is None:
        print 'could not open socket'
        sys.exit(1)
    if HOST is None:
        PHOST = '*'
    else:
        PHOST = HOST
    print "Listening on %s:%d..." % (PHOST, PORT)
    conn, addr = s.accept()
    print "Connected by ", addr
    while 1:
        data = conn.recv(1024)
        print data + "\n"
        if not data: break
        name = data.strip()
        ret = finger.cmain(name)
        conn.send(ret)
        conn.close()
	break
