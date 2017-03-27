import math, sys, time
import pp
import time

def mnoz(dane):
	A = dane[0]
	X = dane[1]

	nrows = len(A)
	ncols = len(A[0])
	y = []
	for i in xrange(nrows):
		s = 0
		for c in range(0, ncols):
			s += A[i][c] * X[c][0]
		y.append(s)
	return y


def read(fname):
	f = open(fname, "r")
	nr = int(f.readline())
	nc = int(f.readline())

	A = [[0] * nc for x in xrange(nr)]
	r = 0
	c = 0
	for i in range(0,nr*nc):
		A[r][c] = float(f.readline())
		c += 1
		if c == nc:
			c = 0
			r += 1

	return A

def jobs_creator(jobs_num, A, X):
	rows_num = len(A)
	end = rows_num
	step = rows_num / jobs_num + 1
	jobs = []

	for index in xrange(jobs_num):
    		starti = index*step
    		endi = min((index+1)*step, end)
    		jobs.append(job_server.submit(mnoz, ((A[starti:endi],X),), (), ("math",)))
	return jobs

starttime = time.time()

ncpus = int(sys.argv[1]) if len(sys.argv) > 1 else 2
fnameA = sys.argv[2] if len(sys.argv) > 2 else "A.dat"
fnameX = sys.argv[3] if len(sys.argv) > 3 else "X.dat"


A = read(fnameA)
X = read(fnameX)


# tuple of all parallel python servers to connect with
ppservers = ("10.146.113.118",)

job_server = pp.Server(ncpus, ppservers=ppservers)
tasks = job_server.get_ncpus() * 32
print "Starting pp with ", tasks, " workers"

endtime = time.time()
print (endtime-starttime)
starttime = time.time()

jobs = jobs_creator(tasks, A, X)

endtime = time.time()

print (endtime-starttime)

# Retrieve results of all submited jobs
results = [job() for job in jobs]
job_server.print_stats()
print 'wyniczek'
print results
#compare outputs
#print "Wynik y= ", mnoz([A,X])
