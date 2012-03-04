import socket
import sys
import finger

HOST = None
PORT = 79
s = None

def drop_priv(uid_name='nobody', gid_name='nogroup'):
    import os, pwd, grp

    xuid = os.getuid()
    xgid = os.getgid()

    xname = pwd.getpwuid(xuid).pw_name

    if os.getuid() != 0:
        return

    if xuid == 0:
        run_uid = pwd.getpwnam(uid_name).pw_uid
        run_gid = grp.getgrnam(gid_name).gr_gid

        try:
            os.setgroups([run_gid])
            os.setgid(run_gid)
            os.setregid(run_gid, run_gid)
        except OSError, e:
            print "cannot set gid to %s" % gid_name
        try:
            os.setuid(run_uid)
            os.setreuid(run_uid, run_uid)
        except OSError, e:
            print "cannot set uid to %s" % uid_name

        new_umask = 077
        old_umask = os.umask(new_umask)

    final_uid = os.getuid()
    final_gid = os.getgid()
    print "Running as %s/%s" % \
        (pwd.getpwuid(final_uid).pw_name,
         grp.getgrgid(final_gid).gr_name)


for res in socket.getaddrinfo(HOST, PORT, socket.AF_UNSPEC, socket.SOCK_STREAM, 0, socket.AI_PASSIVE):
    af, socktype, proto, canonname, sa = res
    try:
        s = socket.socket(af, socktype, proto)
    except socket.error, msg:
        s = None
        print msg
        continue
    try:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind(sa)
        s.listen(1)
    except socket.error, msg:
        s.close()
        s = None
        print msg
        continue
    break;
if s is None:
    print '[Error] Could not open socket'
    sys.exit(1)
if HOST is None:
    PHOST = '*'
else:
    PHOST = HOST
print "Listening on %s:%d" % (PHOST, PORT)
drop_priv()

while True:
    conn, addr = s.accept()
    print "Connection from", addr
    while True:
        data = conn.recv(1024)
        print data + "\n"
        if not data: break
        name = data.strip()
        ret = finger.cmain(name)
        conn.send(ret)
        conn.close()
	break
