import numpy as np

class Stage:
    def __init__(self, fn='positions.csv'):
        with open(fn, 'r') as f:
            csv = f.read()

        lines = csv.split('\n')
        lines = [l.strip() for l in lines if len(l.strip())>0]
        lines = [l for l in lines if (not l.startswith('#')) and len([n for n in l.split(',') if len(n)>0])>=6]

        # print(lines)
        print('read {} valid line(s) from {}...'.format(
            len(lines), fn))

        self.lines = lines
        self.index = 0

    def decode(self, line):
        l = line
        items = l.split(',')
        items = items[0:6]
        assert len(items)==6
        return items

    def decode_all(self):
        return np.array([self.decode(l) for l in self.lines],dtype='float32')

stage = Stage()

# print(s.decode_all())

all_points = stage.decode_all()
print(all_points)

from scipy.interpolate import interp1d

def interp(p1, p2):
    print('interp()', p1, p2)

    mmax =np.amax(np.abs(p1 - p2) * np.array([0.5,1, 1,0.1,0.1,0.1]))
    t = max(0.2, mmax * 0.013) # avoid t=0 singularity

    diff = p2-p1

    interpolator = interp1d([-.1, 0., t, t+.1],[p1, p1, p2, p2], axis=0, kind='cubic')

    time_res = 0.02 # 50Hz

    steps = int(t / time_res)
    steptime = t/steps
    totaltime = steptime*steps

    def cubic(x):
        return -2 * x**3 + 3 * x**2

    def interpolator(a):
        return np.array([p1 + diff * i  for i in cubic(a / totaltime)], dtype='float32')

    return interpolator(np.arange(0,steps+1)*steptime), time_res

    # print(mmax)

# interp(ap[0], ap[1])
