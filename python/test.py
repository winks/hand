from nose import with_setup
import finger

def setup_func():
    global user1
    user1 = {}
    user1['name'] = 'username1'
    user1['uid'] = 23
    user1['gid'] = 23
    user1['home'] = './_fix/1'
    user1['shell'] = '/bin/bash'

    global user2
    user2 = {}
    user2['name'] = 'username2'
    user2['uid'] = 42
    user2['gid'] = 42
    user2['home'] = './_fix/2'
    user2['shell'] = '/bin/bash'

### check_nofinger
@with_setup(setup_func)
def test_check_nofinger1():
    assert not finger.check_nofinger(user1)

@with_setup(setup_func)
def test_check_nofinger2():
    assert finger.check_nofinger(user2)

### check_user
@with_setup(setup_func)
def test_check_user1():
    user = finger.check_user('username1', './_fix/1/passwd1')
    assert (user['name'] == user1['name'])
    assert (user['uid'] == user1['uid'])
    assert (user['gid'] == user1['gid'])
    assert (user['shell'] == user1['shell'])

@with_setup(setup_func)
def test_check_user2():
    user = finger.check_user('username1', './_fix/1/passwd2')
    assert (user == {})

### open_file
def test_open_file1():
    file_name = './_fix/1/.plan'
    msg = finger.open_file(file_name)
    assert (msg == file(file_name).read())

def test_open_file2():
    file_name = './_fix/1/notafile'
    msg = finger.open_file(file_name)
    assert not msg

def test_open_file3():
    file_name = './_fix/2/.plan'
    msg = finger.open_file(file_name)
    assert not msg

### open_plan
@with_setup(setup_func)
def test_open_plan1():
    plan = finger.open_plan(user1)
    assert (plan == file('./_fix/1/.plan').read())
@with_setup(setup_func)

def test_open_plan2():
    plan = finger.open_plan(user2)
    assert not plan 
