import os
import re
import subprocess
import sys

def cmain(name, file_name = '/etc/passwd'):
    user = check_user(name, file_name)
    if user == {}:
        return "finger: %s: no such user." % name
    return display(user)

def check_user(name, file_name = '/etc/passwd'):
    user = {}

    p_file = file(file_name)
    u_len = len(name)
    match = None

    for line in p_file:
        if line[0:u_len+1] == name + ':':
            match = line
    if not match:
        return user

    match = match.split(':')

    user['name'] = match[0]
    user['uid'] = int(match[2])
    user['gid'] = int(match[3])
    user['comment'] = match[4].split(',')
    user['home'] = match[5]
    user['shell'] = match[6].replace("\n", '')
    user['len'] = u_len

    return user

def check_nofinger(user):
    nofinger_file = user['home'] + '/.nofinger'
    try:
        fp = open(nofinger_file)
    except IOError as e:
        return True
    else:
        fp.close()
        return False

def check_who(user):
    who = subprocess.Popen(["who", "-u"], stdout=subprocess.PIPE).communicate()[0].split("\n")
    spaces = re.compile(r"[ ]+")
    matches = []
    for line in who:
        if line[0:user['len']+1] == user['name'] + ' ':
            out = re.split(spaces, line)
            if len(out) > 5:
                who = {}
                who['tty'] = out[1]
                wfrom = out[7][1:]
                who['from'] = wfrom[:-1]
                who['time'] = "%s %s %s" % (out[2], out[3], out[4])
                matches.append(who)
    if len(matches) > 0:
        return matches
    return False

def check_mail(user):
    return "No mail." + "\n"

def open_plan(user):
    file_name = user['home'] + '/.plan'
    return open_file(file_name)

def open_project(user):
    file_name = user['home'] + '/.project'
    return open_file(file_name)

def display_plan(user):
    plan = open_plan(user)
    ret = ''
    if plan:
        ret = ret + "Plan:" + "\n"
        ret = ret + plan
    return ret

def display_project(user):
    proj = open_project(user)
    ret = ''
    if proj:
        ret = ret + "Project:" + "\n"
        ret = ret + proj
    return ret

def display_who(user):
    users = check_who(user)
    ret = ''
    if users:
        for who in users:
            ret = ret + "On since %s on %s from %s" % (who['time'], who['tty'], who['from']) + "\n"
    return ret

def display(user):
    ret = ''
    if user['name']:
        ret = ret + "Login: "
        ret = ret + user['name']
    if user['comment'][0]:
        ret = ret + "\t\t\t\tName: "
        ret = ret + user['comment'][0] + "\n"
    else:
        ret = ret + "\n"

    if user['home']:
        ret = ret + "Directory: "
        ret = ret + user['home']
    if user['shell']:
        ret = ret + "\t\tShell: "
        ret = ret + user['shell'] + "\n"
    else:
        ret = ret + "\n"

    ret = ret + display_who(user)
    ret = ret + check_mail(user)

    ret = ret + display_project(user)
    ret = ret + display_plan(user)

    return ret

def open_file(file_name):
    try:
        fp = open(file_name)
    except IOError as e:
        return False
    else:
        with fp:
            return fp.read()

if __name__ == '__main__':
    print cmain("florian")
    #cmain("florian2")
