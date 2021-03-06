
import serial,time,math,random

def choose_serial_connection():
    import serial.tools.list_ports as stlp

    l = stlp.comports()
    if len(l) == 0:
        raise Exception('No available serial devices found')

    print('Please choose from the following serial ports.')
    for i,s in enumerate(l):
        print('({}) {}'.format(i,s))

    k = input('Please enter a number [{}-{}]'.format(0, len(l)-1))
    if len(k)==0: k=0
    return l[int(k)].device

ser = serial.Serial(choose_serial_connection(), 115200 ,timeout=0.2)

jindex = 0 # joint setting selected index
jsettings = [0]*6 # joint settings
jreadings = [-1]*6 # joint actual readings

def process_serial_incoming_lines(bstring):
    global jreadings
    try:
        s = bstring.decode('ascii')
    except:
        print('serial bytestring decode err', bstring)
        return

    # print('got', s)
    s = s.replace('_', ' ')
    s = s.split('j')
    s = [i.strip().split(' ') for i in s if len(i)>=2]
    # print(s)
    if len(s)==6:
        jreadings = [int(i[1]) for i in s]
    else:
        print('not six')

# limit travel of each axis
def clip(a):
    return max(min(a,1000),20)

# read the positions list from file
from stage import interp, all_points, np

# index of position list we're on
pind = -1

# go interpolated from current location to all_points[idx].
def slide_to_point(idx):
    print('going to point #{}/{}'.format(idx+1, len(all_points)))

    p = all_points[idx]
    now = np.array(jreadings, dtype='float32')

    plan, time_res = interp(now, p)
    for i, p in enumerate(plan):
        time.sleep(time_res)
        # print(p.astype('int32'))

        for j in range(6):
            jsettings[j] = int(
                plan[i][j]
                + 0.5
                # + (random.random()-0.5)*1.3
            )
        print(jsettings)

        apply_joint_settings()

    print('done #{}/{}'.format(idx+1, len(all_points)))

def process_char(c):
    global jindex,jsettings, pind
    if type(c)==type(b''):
        c = c.decode('ascii')[0] # windows hack

    if  c=='[': # left
        jindex = (jindex-1)%6
    elif c==']':
        jindex = (jindex+1)%6
    elif c=='=':
        jsettings[jindex] = clip(jsettings[jindex]+1)
    elif c=='-':
        jsettings[jindex] = clip(jsettings[jindex]-1)
    elif c=='+':
        jsettings[jindex] = clip(jsettings[jindex]+20)
    elif c=='_':
        jsettings[jindex] = clip(jsettings[jindex]-20)
    elif c=='q':
        exit()
    elif c==',':
        # prev point
        pind = (pind-1) % len(all_points)
        slide_to_point(pind)

    elif c=='.':
        # next point
        pind = (pind+1) % len(all_points)
        slide_to_point(pind)
    elif c=='r':
        reading_to_setting()
    else:
        pass

def display_joint_info():
    print(' '.join([
        '{}j{:1d}={:4d}({:4d})'.format(
            '>'if i==jindex else ' ',
            i+1, jsettings[i], jreadings[i]
        )
    for i in range(6)]))

def serial_loop():
    b = b''
    while 1:
        c = ser.read()
        if c == b'\n' or c == b'\r':
            if len(b)>0:
                process_serial_incoming_lines(b)
                b = b''
        else:
            b = b+c

import threading as th
t = th.Thread(target=serial_loop, daemon=True)
t.start()

while jreadings[5] == -1:
    print('waiting for the bot to come up...')
    time.sleep(.2)

def reading_to_setting():
    for idx, k in enumerate(jreadings):
        jsettings[idx] = k
reading_to_setting()

import readchar
print('press q to quit, [] to select, +- to adjust, shift to accelerate')
print('press <> to goto prev/next waypoint')
print('press r to set current position as commanded position')

display_joint_info()

def apply_joint_settings():
    ser.write(''.join(['j{} {} '.format(i+1, k) for i,k in enumerate(jsettings)]).encode())
    ser.flush()

while 1:

    c = readchar.readchar()
    process_char(c)
    display_joint_info()
    apply_joint_settings()
                                             
