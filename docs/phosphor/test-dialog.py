#!/usr/bin/env python3
"""Render the real auth UI on private Xvfb; never submit a real credential."""
import ctypes as C
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import time

daemon_mode = '--daemon' in sys.argv
if daemon_mode: sys.argv.remove('--daemon')
binary = Path(sys.argv[1] if len(sys.argv) > 1 else
              '/usr/local/libexec/xscreensaver/xscreensaver-auth')
for composite in (False, True):
    with tempfile.TemporaryDirectory(prefix='phosphor-test-') as tmp:
        home = Path(tmp)
        (home / '.xscreensaver').write_text('fade: False\nunfade: False\ndialogTheme: phosphor\npasswdTimeout: 0:00:30\nlock: True\nlockTimeout: 0:00:00\nmode: one\nselected: 0\nprograms: /usr/local/libexec/xscreensaver/xmatrix -root -no-trace\n')
        rd, wr = os.pipe()
        server = subprocess.Popen(['Xvfb', '-displayfd', str(wr), '-screen', '0',
                                   '1024x768x24', '-nolisten', 'tcp'], pass_fds=(wr,),
                                  stderr=subprocess.DEVNULL)
        os.close(wr)
        with os.fdopen(rd) as pipe:
            display = ':' + pipe.readline().strip()
        assert display != os.environ.get('DISPLAY')
        env = dict(os.environ, DISPLAY=display, HOME=tmp, LC_ALL='C',
                   DBUS_SESSION_BUS_ADDRESS='unix:path=/nonexistent-phosphor-test')
        children = []
        log_path = Path('/tmp/phosphor-composite.log' if composite else '/tmp/phosphor-opaque.log')
        with log_path.open('w') as log:
            try:
                if composite:
                    children.append(subprocess.Popen(['xcompmgr', '-n'], env=env, stdout=log, stderr=log))
                    time.sleep(.3)
                children.append(subprocess.Popen(['/usr/local/libexec/xscreensaver/xmatrix', '-geometry', '1024x768+0+0', '-no-trace'], env=env, stdout=log, stderr=log))
                app = subprocess.Popen([str(binary), '--display', display, '--verbose', '--sync'] + (['--no-splash'] if daemon_mode else []),
                                       env=env, stdout=log, stderr=log)
                children.append(app)
                time.sleep(1)
                assert app.poll() is None, log_path.read_text()
                x = C.CDLL('libX11.so.6')
                xt = C.CDLL('libXtst.so.6')
                x.XOpenDisplay.argtypes = [C.c_char_p]; x.XOpenDisplay.restype = C.c_void_p
                x.XStringToKeysym.argtypes = [C.c_char_p]; x.XStringToKeysym.restype = C.c_ulong
                x.XKeysymToKeycode.argtypes = [C.c_void_p, C.c_ulong]; x.XKeysymToKeycode.restype = C.c_ubyte
                x.XFlush.argtypes = [C.c_void_p]; x.XCloseDisplay.argtypes = [C.c_void_p]
                xt.XTestFakeKeyEvent.argtypes = [C.c_void_p,C.c_uint,C.c_int,C.c_ulong]
                d = x.XOpenDisplay(display.encode())
                def key(name, delay=.1):
                    code = x.XKeysymToKeycode(d, x.XStringToKeysym(name.encode()))
                    for down in (1,0): xt.XTestFakeKeyEvent(d, code, down, 0)
                    x.XFlush(d)
                    time.sleep(delay)
                if daemon_mode:
                    subprocess.run(['/usr/local/bin/xscreensaver-command', '--lock'], env=env,check=True,capture_output=True)
                    time.sleep(2.3)
                    key('Shift_L')
                    time.sleep(1)
                    assert 'theme: phosphor' in log_path.read_text()
                for ch in 'abcdef': key(ch)
                key('BackSpace')
                screenshot = '/tmp/phosphor-composite.png' if composite else '/tmp/phosphor-opaque.png'
                capture = """import gi
gi.require_version('Gdk','3.0')
from gi.repository import Gdk
w=Gdk.get_default_root_window()
Gdk.pixbuf_get_from_window(w,0,0,1024,768).savev(%r,'png',[],[])
""" % screenshot
                subprocess.run(['/usr/bin/python3','-c',capture], env=env,check=True)
                # Overflow keeps a centered suffix and caret without changing input.
                for ch in 'abcdefghijklmnopqrstuvwxyz0123456789': key(ch, .01)
                time.sleep(.2)
                longshot = screenshot.replace('.png', '-long.png')
                subprocess.run(['/usr/bin/python3','-c',capture.replace(screenshot,longshot)], env=env,check=True)
                for _ in range(42): key('BackSpace', .01)
                time.sleep(.2)
                emptyshot = screenshot.replace('.png', '-empty.png')
                subprocess.run(['/usr/bin/python3','-c',capture.replace(screenshot,emptyshot)], env=env,check=True)
                blink = """import gi,time
gi.require_version('Gdk','3.0')
from gi.repository import Gdk
w=Gdk.get_default_root_window()
levels=[]
for i in range(22):
 p=Gdk.pixbuf_get_from_window(w,504,372,16,24)
 data=p.get_pixels();stride=p.get_rowstride();channels=p.get_n_channels()
 levels.append(sum(data[y*stride+x*channels+1] for y in range(24) for x in range(16))/(16*24))
 time.sleep(.05)
assert min(levels)<110 and max(levels)>150,(min(levels),max(levels))
print('CRT caret brightness range:',round(min(levels)),round(max(levels)))
"""
                subprocess.run(['/usr/bin/python3','-c',blink],env=env,check=True)
                key('Escape')
                if daemon_mode:
                    time.sleep(.4)
                    state = subprocess.check_output(['/usr/local/bin/xscreensaver-command', '--time'], env=env,text=True)
                    assert ': screen locked since ' in state, state
                    # A crashed auth renderer must also leave the daemon locked.
                    time.sleep(1.1)
                    key('Shift_L')
                    time.sleep(.6)
                    auth_pid = None
                    for proc in Path('/proc').iterdir():
                        if not proc.name.isdigit(): continue
                        try:
                            status = (proc/'status').read_text()
                            args = (proc/'cmdline').read_bytes().split(b'\0')
                            if ('PPid:\t%d\n' % app.pid) in status and args and b'xscreensaver-auth' in args[0]:
                                auth_pid = int(proc.name)
                        except (OSError,ProcessLookupError): pass
                    assert auth_pid, 'No auth child found'
                    os.kill(auth_pid, 9)
                    time.sleep(.4)
                    state = subprocess.check_output(['/usr/local/bin/xscreensaver-command', '--time'], env=env,text=True)
                    assert ': screen locked since ' in state, state
                else:
                    app.wait(timeout=8)
                    assert app.returncode != 200, 'Escape must not authenticate'
                assert 'abcdef' not in log_path.read_text(), 'Input must not be logged'
                assert 'X Error' not in log_path.read_text(), log_path.read_text()
                x.XCloseDisplay(d)
                print('PASS:', 'composited' if composite else 'opaque', screenshot)
            finally:
                server.terminate(); server.wait(timeout=5)
                for child in children:
                    if child.poll() is None: child.terminate()
                    child.wait(timeout=5)
